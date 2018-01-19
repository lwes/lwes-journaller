/*======================================================================*
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

#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <zlib.h>
#include <lwes.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "header.h"
#include "time_utils.h"

/* prototypes */
static void signal_handler(int sig);

/* global variable used to indicate what signal (if any) has been caught */
static volatile bool gbl_sig = false;

static const char help[] =
  "lwes-journal-emitter [options] <journal(s)>"                        "\n"
  ""                                                                   "\n"
  "  where options are:"                                               "\n"
  ""                                                                   "\n"
  "    -o lwes:<iface>:<ip>:<port> | lwes:<iface>:<ip>:<port>:<ttl>"   "\n"
  "       Specify a place to send messages."                           "\n"
  "         lwes - send message via lwes"                              "\n"
  "           iface - ethernet interface to send via, defaults to"     "\n"
  "                   system configured default interface."            "\n"
  "           ip    - ip address to send datagrams to."                "\n"
  "           port  - port to send datagrams to."                      "\n"
  "           ttl   - ttl for multicast datagrams"                     "\n"
  ""                                                                   "\n"
  "           if ip is a multicast ip, then datagrams are sent via"    "\n"
  "           multicast, otherwise they are sent via UDP."             "\n"
  ""                                                                   "\n"
  "    -n [one argument]"                                              "\n"
  "       The number of events from file to emit."                     "\n"
  ""                                                                   "\n"
  "    -r [one argument]"                                              "\n"
  "       The number of events per second to attempt to emit."         "\n"
  "       Smooth delivery is attempted, but not guaranteed"            "\n"
  ""                                                                   "\n"
  "    -d"                                                             "\n"
  "       When finished with journal (or journals), start over at"     "\n"
  "       at the first journal"                                        "\n"
  ""                                                                   "\n"
  "    -t"                                                             "\n"
  "       Attempt to use timings of events from file when emitting."   "\n"
  ""                                                                   "\n"
  "    -h"                                                             "\n"
  "       show this message"                                           "\n"
  ""                                                                   "\n"
  "  arguments are specified as -option <value> or -option<value>"     "\n"
  ""                                                                   "\n";

static void signal_handler(int sig) {
  (void)sig; /* appease compiler */
  gbl_sig = true;
}

static void setup_sig_handler ()
{
  sigset_t fullset;
  struct sigaction act;

  sigfillset (&fullset);
  sigprocmask (SIG_SETMASK, &fullset, NULL);

  memset (&act, 0, sizeof (act));
  act.sa_handler = signal_handler;
  sigfillset (&act.sa_mask);

  sigaction (SIGINT, &act, NULL);
  sigaction (SIGTERM, &act, NULL);
  sigaction (SIGPIPE, &act, NULL);

  sigdelset (&fullset, SIGINT);
  sigdelset (&fullset, SIGTERM);
  sigdelset (&fullset, SIGPIPE);

  sigprocmask (SIG_SETMASK, &fullset, NULL);
}

#define MAX_WORDS 100
static struct lwes_emitter *
handle_transport_arg (const char *arg)
{
  struct lwes_emitter *emitter = NULL;
  int i;
  const char *sep  = ":";
  const char *empty = "";
  char *word;
  const char *words[MAX_WORDS];
  int count = 0;
  char *buffer;
  char *tofree;

  for (i = 0 ; i < MAX_WORDS; i++)
    {
      words[i] = empty;
    }

  tofree = buffer = strdup (arg);
  if (buffer == NULL)
    {
      return NULL;
    }

  /* get all the words between the ':'s */
  while ((word = strsep (&buffer, sep)) && count < MAX_WORDS)
    {
      words[count++]=word;
    }

  if (strcmp (words[0], "lwes") == 0)
    {
      if (count < 4 || count > 5)
        {
          fprintf (stderr, "ERROR: lwes transport requires 3 or 4 parts\n");
          fprintf (stderr, "       lwes:<iface>:<ip>:<port>\n");
          fprintf (stderr, "       lwes:<iface>:<ip>:<port>:<ttl>\n");
        }
      else
        {
          const char *iface = NULL;
          const char *ip = NULL;
          int port = -1;
          int ttl = 3;
          if (strcmp (words[1], "") != 0)
            {
              iface = words[1];
            }
          if (strcmp (words[2], "") != 0)
            {
              ip = words[2];
              if (strcmp (words[3], "") != 0)
                {
                  port = atoi (words[3]);
                  if (strcmp (words[4], "") != 0)
                    {
                      ttl = atoi (words[4]);
                      if (ttl < 0 || ttl > 32)
                        {
                          fprintf (stderr, "WARNING: ttl must be between 0 "
                                           "and 32, defaulting to 3\n");
                          ttl = 3;
                        }
                    }
                  emitter =
                    lwes_emitter_create_with_ttl ((LWES_SHORT_STRING) ip,
                                                  (LWES_SHORT_STRING) iface,
                                                  (LWES_U_INT_32) port,
                                                  0,
                                                  60,
                                                  ttl);
                }
              else
                {
                  fprintf (stderr,
                           "ERROR: lwes transport requires "
                           "non-empty port\n");
                }
            }
          else
            {
              fprintf (stderr,
                       "ERROR: lwes transport requires non-empty ip\n");
            }
        }
    }
  else
    {
      fprintf (stderr, "ERROR: unrecognized transport %s\n",words[0]);
    }

  free (tofree);

  return emitter;
}




int main(int argc, char **argv)
{
  gzFile file;
  const char *filename;
  char header[22];
  unsigned char buf[65535];

  const char *args = "n:o:r:tdh";
  int number = 0;            /* (n) total number to emit */
  struct lwes_emitter *emitter = NULL; /* (o) where to send the events */
  int rate = 0;              /* (r) number per second for emission */
  bool use_timings = false;  /* (t) using timings from file */
  bool repeat = false;       /* (d) rerun journals over and over */

  int ret = 0;
  LWES_INT_64 start  = 0LL;
  LWES_INT_64 stop   = 0LL;

  /* turn off error messages, I'll handle them */
  opterr = 0;
  while (1)
    {
      char c = getopt (argc, argv, args);

      if (c == -1)
        {
          break;
        }
      switch (c)
        {
          case 'n':
            number = atoi(optarg);
            break;

          case 'r':
            rate = atoi(optarg);
            break;

          case 'o':
            emitter = handle_transport_arg (optarg);
            if (emitter == NULL)
              {
                fprintf (stderr, "ERROR: problem with emitter\n");
                ret = 1;
                goto cleanup;
              }
            break;

          case 't':
            use_timings = true;
            break;

          case 'd':
            repeat = true;
            break;

          case 'h':
            fprintf (stderr, "%s", help);
            ret = 1;
            goto cleanup;

          default:
            fprintf (stderr,
                     "WARNING: unrecognized command line option -%c\n",
                     optopt);
        }
    }

  if (optind >= argc)
    {
      fprintf (stderr, "ERROR: journal file is required\n");
      ret = 1;
      goto cleanup;
    }

  /* setup sigint/sigkill/sigpipe handler */
  setup_sig_handler();
  start = currentTimeMillisLongLong ();

  int start_index = optind;
  int num_files = argc - optind;
  int offset = 0;
  int total_count = 0;
  int this_second_count = 0;
  unsigned long long delay_micros = rate > 0 ? 1000000 / rate : 0;

  bool done = false;
  while (! done)
    {
      /* deal with multiple files and repeating */
      filename = argv[start_index + offset];
      if (! repeat && offset == (num_files - 1))
        {
          done = true; /* last file and we are not repeating,
                          so done after it's processed */
        }
      offset = (offset + 1) % num_files; /* skip to next file if available */

      file = gzopen (filename, "rb");
      struct timeval start_emit_time;
      micro_now (&start_emit_time);
      struct timeval current_emit_time = { 0, 0 };
      struct timeval previous_emit_time = { 0, 0 };
      unsigned long long time_between_emit_events = 0ULL;
      unsigned long long start_file_timestamp = 0ULL;
      unsigned long long end_file_timestamp = 0ULL;
      int file_count = 0;
      unsigned long long time_to_sleep = 0ULL;

      /* read a header from the file */
      while (gzread (file, header, 22) == 22)
        {
          unsigned short size = header_payload_length (header);
          unsigned long long cur_file_timestamp =
            (unsigned long long)(header_receipt_time (header));
          if (start_file_timestamp == 0ULL)
            {
              start_file_timestamp = cur_file_timestamp;
            }
          end_file_timestamp = cur_file_timestamp;

          /* read an event from the file */
          if (gzread (file, buf, size) != size)
            {
              fprintf (stderr, "ERROR: failure reading journal\n");
              done = true;
              ret=1;
              break;
            }

          /* keep track of the current emit time */
          micro_now (&current_emit_time);

          /* if we are using timings determine how long to sleep before
           * emitting the next event */
          if (use_timings)
            {
              unsigned long long time_between_file_events = 0ULL;

              time_between_file_events =
                (cur_file_timestamp - start_file_timestamp) * 1000ULL;
              time_between_emit_events =
                micro_timediff (&start_emit_time, &current_emit_time);

              time_to_sleep =
                time_between_file_events > time_between_emit_events
                ? time_between_file_events - time_between_emit_events
                : 0ULL;

              /* sleep some number of microseconds */
              if (time_to_sleep > 0)
                {
                  usleep (time_to_sleep);
                }
            }

          /* if we are trying to meet a rate, determine how long to sleep
           * before next call
           */
          if (rate > 0)
            {
              if (previous_emit_time.tv_sec != 0)
                {
                  /* the difference in micros alway includes the time slept,
                   * so to figure out how much time we should sleep we subtract
                   * the previous slept time from how much time it's actually
                   * been since the last emit.  In other words sleep time plus
                   * processing time should equal delay_ms
                   */

                  unsigned long long diff =
                    micro_timediff (&previous_emit_time, &current_emit_time)
                    - time_to_sleep;
                  time_to_sleep =
                    delay_micros > diff
                    ? delay_micros - diff
                    : 0ULL;
                  if (time_to_sleep > 0)
                    {
                      usleep (time_to_sleep);
                    }
                }
            }

          /* then emit the event */
          if (lwes_emitter_emit_bytes (emitter, buf, size) != size)
            {
              fprintf (stderr, "ERROR: failure emitting\n");
              done = true;
              ret=1;
              break;
            }

          /* keep track of some counts */
          file_count++;
          total_count++;
          this_second_count++;
          previous_emit_time = current_emit_time;

          /* if we are emitting a limited number we will be done if we hit
           * that number
           */
          if (number > 0 && number == total_count)
            {
              done = true;
              ret=2;
              break;
            }
          if (gbl_sig)
            {
              done = true;
              ret=3;
              break;
            }
        }
      gzclose (file);
      fprintf (stderr,
               "emitted %d events from %s representing %lld file time "
               "in %llu milliseconds\n",
               file_count, filename,
               (end_file_timestamp - start_file_timestamp),
               micro_timediff (&start_emit_time, &current_emit_time) / 1000ULL);
    }
  /* if we hit the count limit (2) it is not an error */
  if (ret == 2)
    {
      ret = 0;
    }
  stop = currentTimeMillisLongLong ();
  fprintf (stderr, "emitted %d events in %ld milliseconds\n",
           total_count, (stop - start));
cleanup:
  if (emitter != NULL)
    {
      lwes_emitter_destroy (emitter);
    }
  exit (ret);
}
