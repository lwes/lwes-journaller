/*======================================================================*
 * Copyright (c) 2008, Yahoo! Inc. All rights reserved.                 *
 * Copyright (c) 2010-2016, OpenX Inc.   All rights reserved.           *
 *                                                                      *
 * Licensed under the New BSD License (the "License"); you may not use  *
 * this file except in compliance with the License.  Unless required    *
 * by applicable law or agreed to in writing, software distributed      *
 * under the License is distributed on an "AS IS" BASIS, WITHOUT        *
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.     *
 * See the License for the specific language governing permissions and  *
 * limitations under the License. See accompanying LICENSE file.        *
 *======================================================================*/

#include "config.h"

#include "log.h"
#include "opt.h"
#include "perror.h"
#include "process_model.h"
#include "queue_to_journal.h"
#include "sig.h"
#include "xport_to_queue.h"

#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

/* Start a new process via fork() and exec() Return the process ID of
   the new child process. */

static int start_program(const char* program, const char* argv[], FILE *log)
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
      LOG_ER(log,"Program path too long: %s.\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  if ( strlen(program) > NAME_MAX )
    {
      LOG_ER(log,"Program name too long: %s.\n", program);
      exit(EXIT_FAILURE);
    }

  /* Create a child process. */
  if ( (pid = fork()) < 0 )
    {
      PERROR(log,"fork()");
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

  LOG_PROG(log,"About to execvp(\"%s\",\n", prg);
  for ( i=0; argv[i]; ++i )
    {
      LOG_PROG(log,"  argv[%d] == \"%s\"\n", i, argv[i]);
    }
  LOG_PROG(log,"  argv[%d] == NULL\n\n", i);

  /* close the log in the child here, it will be reopened by the child */
//  close_log (log);

  /* Replace our current process image with the new one. */
  execvp(prg, (char** const)argv);

  /* Any return is an error. */
  PERROR(NULL,"execvp()");
  LOG_ER(NULL,"The execvp() function returned, exiting.\n");
  exit(EXIT_FAILURE);      /* Should never get to this anyway. */
}

static void signal_pid (int sig, int pid)
{
  if ( -1 != pid )
    {
      kill(pid, sig);
    }
}


/* Start the external processes, then loop to restart any program
   that dies. */
void process_model(const char* argv[], FILE *log)
{
  int waiting_pid = -1;
  int queue_to_journal_pid = -1;
  int xport_to_queue_pid = -1;

  /* this is the main process, so we install all the handlers here */
  install_termination_signal_handlers (log);
  install_rotate_signal_handlers (log);
  install_interval_rotate_handlers (log, 1);
  /* main process rotates on USR1 */
  install_log_rotate_signal_handlers (log, 1, SIGUSR1);

  while ( ! gbl_done )
    {
      int status;

      if ((-1 == queue_to_journal_pid) || (waiting_pid == queue_to_journal_pid))
        {
          queue_to_journal_pid = start_program("queue_to_journal", argv, log);
        }

      if ((-1 == xport_to_queue_pid) || (waiting_pid == xport_to_queue_pid))
        {
          xport_to_queue_pid = start_program("xport_to_queue", argv, log);
        }

      /* Wait and restart any program that exits. */
      waiting_pid = wait(&status);

      /* If a signal interrupts the wait, we loop. */
      if ( (waiting_pid < 0) && (errno == EINTR) )
        {
          /* If we get a rotate, send it along to the process
           * that writes journals. */
          if ( gbl_rotate_dequeue )
            {
              signal_pid (SIGHUP, queue_to_journal_pid);
              CAS_OFF(gbl_rotate_dequeue);
            }
          if ( gbl_rotate_enqueue )
            {
              signal_pid (SIGHUP, xport_to_queue_pid);
              CAS_OFF(gbl_rotate_enqueue);
            }
          if (gbl_rotate_main_log)
            {
              log = get_log (log);
              CAS_OFF (gbl_rotate_main_log);
              /* need to signal both sub-pids */
              signal_pid(SIGUSR1, xport_to_queue_pid);
              signal_pid(SIGUSR2, queue_to_journal_pid);
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

          if ( waiting_pid == xport_to_queue_pid )
            {
              program = "xport_to_queue";
            }

          LOG_WARN(log,
                   "Our %s process exited (pid=%d) with "
                   "signal %d, restarting.\n",
                   program, waiting_pid, WTERMSIG(status));
        }
    }

  /* We give the children a chance to run before we kill them. */
  sleep(1);

  /* We are done, send a quit signal to our children. */
  kill(0, SIGQUIT);
  while ( wait(0) > 0 )   /* Wait for children. */
    ;
  if ( errno != ECHILD && errno != EINTR )
    {
      PERROR(log,"wait");
    }
}

