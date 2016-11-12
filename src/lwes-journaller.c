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
      exit(EXIT_FAILURE);
    }
  do_fork();

  umask(0);
  if ( chdir(chdir_root) < 0)
    {
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
      exit(EXIT_FAILURE);
    }
  if ( dup(0) != 2 )
    {
      exit(EXIT_FAILURE);
    }
}


/* Write the process ID of this process into a file so we may be
 * easily signaled.
 *
 * Function returns 0 if pidfile is wanted and written
 * and a non-zero number otherwise
 */
static int write_pid_file(void)
{
  /* Write out the PID into the PID file. */
  if ( arg_pid_file )
    {
      FILE* pidfp = fopen(arg_pid_file, "w");
      if ( ! pidfp )
        {
          return 1;
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
  return 0;
}


/* Delete the PID file, if one was configured.
 *
 * Function returns 0 if pidfile is there and deleted
 * and a non-zero number otherwise
 */
static int delete_pid_file()
{
  if ( arg_pid_file )
    {
      if ( unlink ( arg_pid_file ) < 0 )
        {
          return errno;
        }
    }
  return 0;
}

int main(int argc, const char* argv[])
{
  FILE *log = get_log (NULL);

  switch (process_options(argc, argv, log))
    {
      case 0:
        break;
      case -1:
        exit(EXIT_SUCCESS);
      default:
        exit(EXIT_FAILURE);
    }

  if ( ! arg_nodaemonize )
    {
      close_log (log); /* close before daemonizing */
      daemonize();
    }

  log = get_log (log); /* reopen */
  LOG_INF(log, "Initializing - lwes-journaller-%s\n", VERSION);

  if (write_pid_file() != 0)
    {
      LOG_ER(log,"Can't open PID file \"%s\"\n", arg_pid_file);
    }

  if ( arg_njournalls > 30 )
    {
      LOG_WARN(log, "suspiciously large (%d) number of journals.",
               arg_njournalls);
    }

  LOG_INF(log, "Starting up - lwes-journaller-%s using %s model\n",
          VERSION, arg_proc_type);

  if ( strcmp(arg_proc_type, ARG_PROCESS) == 0 )
    {
      process_model(argv, log);
    }

  if ( strcmp(arg_proc_type, ARG_THREAD) == 0 )
    {
      thread_model(log);
    }

  if ( strcmp(arg_proc_type, ARG_SERIAL) == 0 )
    {
      serial_model(log);
    }

  int r = 0;
  if ((r = delete_pid_file()) < 0)
    {
      LOG_ER(log,"Unable to delete pid file : %s\n",
             delete_pid_file, strerror(r));
    }
  options_destructor();

  LOG_INF(log, "Normal shutdown complete - lwes-journaller-%s\n", VERSION);

  close_log (log);

  return 0;
}
