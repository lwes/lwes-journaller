/*======================================================================*
 * Copyright (C) 2008 Light Weight Event System                         *
 * All rights reserved.                                                 *
 *                                                                      *
 * This program is free software; you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation; either version 2 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program; if not, write to the Free Software          *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,                   *
 * Boston, MA 02110-1301 USA.                                           *
 *======================================================================*/

#include "config.h"

#include "log.h"
#include "opt.h"
#include "perror.h"
#include "queue_to_journal.h"
#include "sig.h"
#include "xport_to_queue.h"

#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include <netinet/in.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NREADERS_MAX 5

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0
#endif

/* Start a new process via fork() and exec() Return the process ID of
   the new child process. */

static int start_program(const char* program, const char* argv[])
{
  int     i;
  int     pid;
  char*   fn;
  char    prg[PATH_MAX + NAME_MAX + 1];
  char    exec_path[PATH_MAX + 1];

  /* Make sure the path to our executable is within reasonable
     limits. */

  if ( strlen(argv[0]) > PATH_MAX )
    {
      LOG_ER("Program path too long: %s.\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  if ( strlen(program) > NAME_MAX )
    {
      LOG_ER("Program name too long: %s.\n", program);
      exit(EXIT_FAILURE);
    }

  /* Create a child process. */
  if ( (pid = fork()) < 0 )
    {
      PERROR("fork()");
      exit(EXIT_FAILURE);
    }

  /* If we're the parent process, we're done -- the child process
     continues on. */
  if ( pid )
    {
      return pid;
    }

  /* Get the path to our executable, use it to exec() other programs
     from the same directory. */

  if ( 0 != (fn = rindex(argv[0], '/')) )
    {
      strncpy(exec_path, argv[0], fn - argv[0] + 1);
      exec_path[fn - argv[0] + 1] = '\0';
    }
  else
    {
      exec_path[0] = '\0';
    }
  snprintf(prg, sizeof(prg), "%s%s", exec_path, program);

  /* Use new program name.  Make sure to mess with argv[0] after the
     fork() call, so as not to bugger up the calling processes
     argv[0]. */

  argv[0] = prg;

  LOG_PROG("About to execvp(\"%s\",\n", prg);
  for ( i=0; argv[i]; ++i )
    {
      LOG_PROG("  argv[%d] == \"%s\"\n", i, argv[i]);
    }
  LOG_PROG("  argv[%d] == NULL\n\n", i);

  /* Replace our current process image with the new one. */
  execvp(prg, (char** const)argv);

  /* Any return is an error. */
  PERROR("execvp()");
  LOG_ER("The execvp() function returned, exiting.\n");
  exit(EXIT_FAILURE);      /* Should never get to this anyway. */
}


/* Start the external processes, then loop to restart any program
   that dies. */

static void process_model(const char* argv[])
{
  int i;
  int waiting_pid = -1;
  int queue_to_journal_pid = -1;
  int xport_to_queue_pid[NREADERS_MAX];

  for ( i=0; i<NREADERS_MAX; ++i )
    {
      xport_to_queue_pid[i] = -1;
    }

  while ( ! gbl_done )
    {
      int status;

      if ((-1 == queue_to_journal_pid) || (waiting_pid == queue_to_journal_pid))
        {
          queue_to_journal_pid = start_program("queue_to_journal", argv);
        }

      for ( i=0; i<arg_nreaders; ++i )
        {
          if ((-1 == xport_to_queue_pid[i]) ||
              (waiting_pid == xport_to_queue_pid[i]))
            {
              xport_to_queue_pid[i] = start_program("xport_to_queue", argv);
            }
        }

      /* Wait and restart any program that exits. */
      waiting_pid = wait(&status);

      /* If a signal interrupts the wait, we loop. */
      if ( (waiting_pid < 0) && (errno == EINTR) )
        {
          /* If we get a rotate, send it along to the process
           * that writes journals. */
          if ( gbl_rotate )
            {
              if ( -1 != queue_to_journal_pid )
                {
                  LOG_PROG("Sending SIGHUP to queue_to_journal[%d] to "
                           "trigger log rotate.\n", queue_to_journal_pid);
                  kill(queue_to_journal_pid, SIGHUP);
                }
              gbl_rotate = 0;
            }
          continue;
        }

      if ( WIFEXITED(status) )
        {
          break;
        }

      if ( WIFSIGNALED(status) )
        {
          /* If any child process exits abnormally, log it and restart. */
          const char* program = "unknown";

          if ( waiting_pid == queue_to_journal_pid )
            {
              program = "queue_to_journal";
            }

          for ( i=0; i<arg_nreaders; ++i )
            {
              if ( waiting_pid == xport_to_queue_pid[i] )
                {
                  program = "xport_to_queue";
                }
            }

          log_msg(LOG_WARNING, __FILE__, __LINE__,
                  "Our %s process exited (pid=%d) with "
                  "signal %d, restarting.\n",
                  program, waiting_pid, WTERMSIG(status));
        }
    }

  LOG_INF("Shutting down.");

  /* We give the children a chance to run before we kill them. */
  sleep(1);

  /* We are done, send a quit signal to our children. */
  kill(0, SIGQUIT);
  while ( wait(0) > 0 )   /* Wait for children. */
    ;
  if ( errno != ECHILD && errno != EINTR )
    {
      PERROR("wait");
    }
}


#if HAVE_PTHREAD_H
static void thread_model()
{
  int i;
  pthread_t queue_to_journal_tid;
  pthread_t xport_to_queue_tid[NREADERS_MAX];

  static pthread_attr_t m_pthread_attr ;
  // Start thread attributes with pthread FIFO policy (default would be OTHER)
  pthread_attr_init(&m_pthread_attr) ;
  pthread_attr_setdetachstate(&m_pthread_attr, PTHREAD_CREATE_JOINABLE);
  if ( arg_rt )
    {
      pthread_attr_setschedpolicy(&m_pthread_attr, SCHED_FIFO) ;
    }

  if ( pthread_create(&queue_to_journal_tid,
                      &m_pthread_attr, queue_to_journal, NULL) )
    {
      PERROR("pthread_create(&queue_to_journal_tid, NULL, queue_to_journal, NULL)");
    }
  LOG_INF("queue_to_journal_tid == %d\n", queue_to_journal_tid);

  for ( i=0; i<arg_nreaders; ++i )
    {
      if ( pthread_create(&xport_to_queue_tid[i], &m_pthread_attr,
                          xport_to_queue, NULL) )
        {
          PERROR("pthread_create(&xport_to_queue_tid[i], NULL, xport_to_queue, NULL)");
        }
        {
          int schedpolicy ;
          pthread_attr_getschedpolicy((pthread_attr_t*)&m_pthread_attr,
                                      &schedpolicy) ;
          switch ( schedpolicy )
            {
            case SCHED_OTHER: LOG_ER("pthread_policy=SCHED_OTHER\n") ; break ;
            case SCHED_FIFO: LOG_ER("pthread_policy=SCHED_FIFO") ; break ;
            case SCHED_RR: LOG_ER("pthread_policy=SCHED_RR") ; break ;
                            //case  SCHED_MIN   = SCHED_OTHER,
                            //case  SCHED_MAX   = SCHED_RR
            default: LOG_ER("pthread_policy=unknown") ; break ;
            }

          LOG_INF("xport_to_queue_tid[%d] == %d\n",
                   i, xport_to_queue_tid[i]);
        }
    }

  while ( ! gbl_done )
    {
      sleep(99);
    }

  LOG_INF("Shutting down.");

  /* All threads share a single gbl_done, so just interrupt them and
     wait for them to finish on their own. */

  for ( i=0; i<arg_nreaders; ++i )
    {
      LOG_PROG("Sending SIGQUIT to xport_to_queue thread %d (%d).\n",
               i, xport_to_queue_tid);
      if ( pthread_kill(xport_to_queue_tid[i], SIGQUIT) )
        {
          PERROR("pthread_kill");
        }
    }

  LOG_PROG("Sending SIGQUIT to queue_to_journal thread (%d).\n",
           queue_to_journal_tid);
  if ( pthread_kill(queue_to_journal_tid, SIGQUIT) )
    {
      switch ( errno )
        {
        default:
          PERROR("pthread_kill");

        case ESRCH:   /* If the thread has already terminated. */
        case ECHILD:
          break;
        }
    }

  for (i=0; i<arg_nreaders; ++i)
    {
      LOG_PROG("Joining xport_to_queue thread %d (%d).\n",
               i, xport_to_queue_tid[i]);
      if ( pthread_join(xport_to_queue_tid[i], NULL) )
        {
          PERROR("pthread_join");
        }
    }

  LOG_PROG("Joining queue_to_journal thread (%d).\n", queue_to_journal_tid);
  if ( pthread_join(queue_to_journal_tid, NULL) )
    {
      PERROR("pthread_join");
    }

  pthread_attr_destroy(&m_pthread_attr);
}
#else
static void thread_model()
{
  LOG_ER("No POSIX thread support on this platform.\n");
}
#endif

/* Return non-zero if addrstr is a multicast address. */

#include <netinet/in.h>
#include <arpa/inet.h>

static int is_multicast(const char* addrstr)
{
  struct in_addr addr;

  if ( 0 == inet_aton(addrstr, &addr) )
    {
    return 0;
    }

  return IN_MULTICAST(ntohl(addr.s_addr));
}

static void do_fork()
{
  switch ( fork() )
    {
    case 0:
      break;

    case -1:
      PERROR("fork()");
      exit(EXIT_FAILURE);

    default:
      exit(EXIT_SUCCESS);
  }
}

static void daemonize()
{
  int fd, fdlimit;
  const char *chdir_root = "/";

  do_fork();
  if ( setsid() < 0 )
    {
    PERROR("setsid()");
    exit(EXIT_FAILURE);
  }
  do_fork();

  umask(0);
  if ( chdir(chdir_root) < 0)
    {
      LOG_ER("Unable to chdir(\"%s\"): %s\n", chdir_root, strerror(errno));
      exit(EXIT_FAILURE);
    }

  /* Close all open files. */
  fdlimit = sysconf (_SC_OPEN_MAX);
  for ( fd=0; fd<fdlimit; ++fd )
    {
    close(fd);
    }

  /* Open 0, 1 and 2. */
  open("/dev/null", O_RDWR);
  if ( dup(0) != 1 )
    {
      PERROR("Unable to dup() to replace stdout: %s\n");
      exit(EXIT_FAILURE);
    }
  if ( dup(0) != 2 )
    {
      PERROR("Unable to dup() to replace stderr: %s\n");
      exit(EXIT_FAILURE);
    }
}


/* Write the process ID of this process into a file so we may be
   easily signaled. */

static void write_pid_file()
{
  /* Write out the PID into the PID file. */
  if ( arg_pid_file )
    {
      FILE* pidfp = fopen(arg_pid_file, "w");
      if ( ! pidfp )
        {
          LOG_ER("Can't open PID file \"%s\"\n", arg_pid_file);
        }
      else
        {
          /* This obnoxiousness is to avoid compiler warnings that we'd get
             compiling on systems with long or int pid_t. */
          pid_t pid = getpid();
          if ( sizeof(pid_t) == sizeof(long) )
            {
              long x = pid;
              fprintf(pidfp, "%ld\n", x);
            }
          else
            {
              int x = pid;
              fprintf(pidfp, "%d\n", x);
            }
          fclose(pidfp);
        }
    }
}


/* Delete the PID file, if one was configured. */

static void delete_pid_file()
{
  if ( arg_pid_file )
    {
      if ( unlink ( arg_pid_file ) < 0 )
        {
          PERROR("Unable to delete pid file");
        }
    }
}


int main(int argc, const char* argv[])
{
  char _buf[100] ; // for log messages
  char _progver[30] ;

  process_options(argc, argv);

  if ( ! arg_nodaemonize )
    {
      daemonize();
    }

  strcpy(_progver, "lwes-journaller-") ;
  strcat(_progver, ""VERSION"") ;

  strcpy(_buf, "Initializing - ") ;
  strcat(_buf, _progver) ;
  strcat(_buf, "\n") ;
  LOG_INF(_buf);

  install_signal_handlers();

  write_pid_file();

  /* Check options. */
  if ( is_multicast(arg_ip) )
    {
      if ( ! arg_join_group )
        {
          LOG_WARN("Using multi-cast address without joining group.\n");
        }
      if ( arg_nreaders > 1 )
        {
          LOG_WARN("Multiple reader threads can't be used with multi-cast "
                   "addresses, will use a single thread instead.\n");
          arg_nreaders = 1;
        }
    }
  else
    {
      if ( arg_join_group )
        {
          LOG_WARN("Can't join group with uni-cast address.\n");
        }
    }
  if ( arg_nreaders > NREADERS_MAX )
    {
      LOG_WARN("Too many reader threads specified (%d) reducing to %d.\n",
               arg_nreaders, NREADERS_MAX);
      arg_nreaders = NREADERS_MAX;
    }
  if ( arg_njournalls > 30 )
    {
      LOG_WARN("suspiciously large (%d) number of journals.",
               arg_njournalls);
    }

  strcpy(_buf, "Starting up - ") ;
  strcat(_buf, _progver) ;
  strcat(_buf, "\n") ;
  LOG_INF(_buf);

  if ( strcmp(arg_proc_type, ARG_PROCESS) == 0 )
    {
      process_model(argv);
    }

  if ( strcmp(arg_proc_type, ARG_THREAD) == 0 )
    {
      thread_model();
    }

  strcpy(_buf, "Normal shutdown complete - ") ;
  strcat(_buf, _progver) ;
  strcat(_buf, "\n") ;
  LOG_INF(_buf);

  delete_pid_file();

  return 0;
}
