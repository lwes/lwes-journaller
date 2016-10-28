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
#include "queue_to_journal.h"
#include "sig.h"
#include "thread_model.h"
#include "time_utils.h"
#include "xport_to_queue.h"

#if HAVE_PTHREAD_H
#define __USE_GNU
#include <pthread.h>
#endif
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#if HAVE_PTHREAD_H

static void *
signal_thread(void *arg)
{
  sigset_t *set = arg;
  int sig;

  int *i = (int *)malloc(sizeof(int));

#ifdef HAVE_PTHREAD_SETNAME_NP_2
  pthread_setname_np (pthread_self(), "signal_handler");
#endif
#ifdef HAVE_PTHREAD_SETNAME_NP_1
  pthread_setname_np ("signal_handler");
#endif

  /* this sets up the itimer to send a SIGALRM to rotate */
  install_interval_rotate_handlers (0);

  while (! gbl_done )
    {
      if (sigwait(set, &sig)) {
        PERROR ("sigwait");
      }
      if (sig == SIGINT || sig == SIGQUIT || sig == SIGTERM) {
        __sync_bool_compare_and_swap(&gbl_done,0,1);
      }
      if (sig == SIGHUP || sig == SIGALRM) {
        __sync_bool_compare_and_swap(&gbl_rotate_enqueue,0,1);
        __sync_bool_compare_and_swap(&gbl_rotate_dequeue,0,1);
      }
      if (sig == SIGUSR1) {
        __sync_bool_compare_and_swap(&gbl_rotate_log,0,1);
      }
   }

  *i = 0;
  pthread_exit((void *)i);
}

static void *
xport_to_queue_thread(void* arg)
{
  (void)arg; /* appease -Wall -Werrorj */

  int *i = (int *)malloc(sizeof(int));
#ifdef HAVE_PTHREAD_SETNAME_NP_2
  pthread_setname_np (pthread_self(), "xport_to_queue");
#endif
#ifdef HAVE_PTHREAD_SETNAME_NP_1
  pthread_setname_np ("xport_to_queue");
#endif
  int r = xport_to_queue();
  *i = r;
  pthread_exit((void *)i);
}

static void *
queue_to_journal_thread(void *arg)
{
  (void)arg; /* appease -Wall -Werrorj */

  int *i = (int *)malloc(sizeof(int));
#ifdef HAVE_PTHREAD_SETNAME_NP_2
  pthread_setname_np (pthread_self(), "queue_to_jrnl");
#endif
#ifdef HAVE_PTHREAD_SETNAME_NP_1
  pthread_setname_np ("queue_to_jrnl");
#endif
  int r = queue_to_journal();
  *i = r;
  pthread_exit((void *)i);
}

void thread_model(void)
{
  pthread_t queue_to_journal_tid;
  pthread_t signal_tid;
  pthread_t xport_to_queue_tid;
  sigset_t set;

  static pthread_attr_t m_pthread_attr ;
  // Start thread attributes with pthread FIFO policy (default would be OTHER)
  pthread_attr_init(&m_pthread_attr) ;
  pthread_attr_setdetachstate(&m_pthread_attr, PTHREAD_CREATE_JOINABLE);
  if ( arg_rt )
    {
      pthread_attr_setschedpolicy(&m_pthread_attr, SCHED_FIFO) ;
    }

  /* Block all signals, all threads created here will inherit this mask */
  sigfillset(&set);
  if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0)
    {
      PERROR("pthread_sigmask");
    }

  /* pass the mask to the thread dealing with signals */
  if ( pthread_create(&signal_tid,
                      &m_pthread_attr, signal_thread, (void *)&set) != 0 )
    {
      PERROR("pthread_create(signal)");
    }

  if ( pthread_create(&queue_to_journal_tid,
                      &m_pthread_attr, queue_to_journal_thread, NULL) != 0 )
    {
      PERROR("pthread_create(queue_to_journal)");
    }

  if ( pthread_create(&xport_to_queue_tid, &m_pthread_attr,
                      xport_to_queue_thread, NULL) != 0 )
    {
      PERROR("pthread_create(xport_to_queue)");
    }

  while ( ! gbl_done )
    {
      sleep(1);
    }

  LOG_INF("Shutting down.\n");

  /* All threads should notice the gbl_done and finish on their own. */

  void *res = NULL;
  if ( pthread_join(xport_to_queue_tid, &res) != 0 )
    {
      PERROR("pthread_join(xport_to_queue_tid[i], NULL)");
    }
  else
    {
      int *i = (int *)res;
      if ((*i) != 0)
        {
          LOG_INF("join of xport_to_queue_tid failed %d\n", (*i));
        }
      free(res);
    }

  res = NULL;
  if ( pthread_join(queue_to_journal_tid, &res) != 0 )
    {
      PERROR("pthread_join(queue_to_journal_tid, NULL)");
    }
  else
    {
      int *i = (int *)res;
      if ((*i) != 0)
        {
          LOG_INF("join of queue_to_journal_tid failed %d\n",(*i));
        }
      free(res);
    }

  res = NULL;
  if ( pthread_join(signal_tid, &res) != 0 )
    {
      PERROR("pthread_join(signal_tid, NULL)");
    }
  else
    {
      int *i = (int *)res;
      if ((*i) != 0)
        {
          LOG_INF("join of signal_tid failed %d\n",(*i));
        }
      free(res);
    }

  pthread_attr_destroy(&m_pthread_attr);
}
#else
static void thread_model()
{
  LOG_ER("No POSIX thread support on this platform.\n");
}
#endif

