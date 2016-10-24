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
#include "serial_model.h"
#include "thread_model.h"
#include "sig.h"

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

  write_pid_file();

  if ( arg_njournalls > 30 )
    {
      LOG_WARN("suspiciously large (%d) number of journals.",
               arg_njournalls);
    }

  strcpy(_buf, "Starting up - ") ;
  strcat(_buf, _progver) ;
  strcat(_buf, " using ") ;
  strcat(_buf, arg_proc_type);
  strcat(_buf, " model\n");
  LOG_INF(_buf);

  if ( strcmp(arg_proc_type, ARG_PROCESS) == 0 )
    {
      process_model(argv);
    }

  if ( strcmp(arg_proc_type, ARG_THREAD) == 0 )
    {
      thread_model();
    }

  if ( strcmp(arg_proc_type, ARG_SERIAL) == 0 )
    {
      serial_model();
    }

  strcpy(_buf, "Normal shutdown complete - ") ;
  strcat(_buf, _progver) ;
  strcat(_buf, "\n") ;
  LOG_INF(_buf);

  delete_pid_file();

  return 0;
}
