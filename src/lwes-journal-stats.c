#define _GNU_SOURCE
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <zlib.h>
#include <lwes.h>

#include "header.h"
#include "time_utils.h"
#include "uthash.h"

static const char help[] =
  "lwes-journal-stats [options] <journal>"                             "\n"
  ""                                                                   "\n"
  "  where options are:"                                               "\n"
  ""                                                                   "\n"
  "    -h"                                                             "\n"
  "       show this message"                                           "\n"
  ""                                                                   "\n"
  "  arguments are specified as -option <value> or -option<value>"     "\n"
  ""                                                                   "\n";

struct stats_by_event {
  LWES_CHAR name[SHORT_STRING_MAX+1];
  unsigned long long total_events;
  unsigned long long total_bytes;
  unsigned short min_bytes;
  unsigned short max_bytes;
  UT_hash_handle hh;
};

struct stats_by_event *stats = NULL;

static void upsert_stats (LWES_SHORT_STRING name, size_t bytes) {
  struct stats_by_event *s = NULL;

  /* The following fields are often in the header and some tools add them
   * to the end of the event in the lwes serialization format, which gives
   * 
   *   int64   ReceiptTime =    1 (short string length)
   *                         + 11 (length of string)
   *                         +  1 (length of type byte)
   *                         +  8 (length of int64)
   *                       = 21
   *   ip_addr SenderIp    = 1 + 8 + 1 + 4  = 14
   *   uint16  SenderPort  = 1 + 10 + 1 + 2 = 14
   *   uint16  SiteID      = 1 + 6 + 1 + 2  = 10
   *
   * So we end up with 21+14+14+10=59 considered as part of the byte count,
   * so just add 50 to each count here.
   */

  bytes += 59;
  HASH_FIND_STR(stats, name, s);
  if (s == NULL) {
    s = (struct stats_by_event *)malloc(sizeof(struct stats_by_event));
    memset (s, 0, sizeof (struct stats_by_event));
    strncpy (s->name, name, strlen (name));
    s->total_events = 0;
    s->total_bytes = 0;
    s->min_bytes = 65535;
    s->max_bytes = 0;
    HASH_ADD_STR(stats, name, s);
  }
  s->total_bytes += bytes;
  s->total_events++;
  s->min_bytes = bytes < s->min_bytes ? bytes : s->min_bytes;
  s->max_bytes = bytes > s->max_bytes ? bytes : s->max_bytes;
}

static int name_sort (struct stats_by_event *a, struct stats_by_event *b) {
  return strcmp (a->name, b->name);
}

static void print_stats () {
  struct stats_by_event *s, *tmp;
  unsigned long long total_events = 0;
  unsigned long long total_bytes = 0;
  unsigned short min_bytes = 65535;
  unsigned short max_bytes = 0;

  HASH_SORT (stats, name_sort);
  for (s = stats ; s != NULL; s = (struct stats_by_event *)(s->hh.next)) {
    fprintf (stdout,"%-30s %10llu events (bytes: %llu/%u/%u total/min/max)\n",
             s->name,
             s->total_events,
             s->total_bytes,
             s->min_bytes,
             s->max_bytes);
    total_bytes += s->total_bytes;
    total_events += s->total_events;
    min_bytes = s->min_bytes < min_bytes ? s->min_bytes : min_bytes;
    max_bytes = s->max_bytes > max_bytes ? s->max_bytes : max_bytes;
  }
  fprintf (stdout,"%-30s %10llu events (bytes: %llu/%u/%u total/min/max)\n",
           "TOTAL", total_events, total_bytes, min_bytes, max_bytes);
  HASH_ITER(hh, stats, s, tmp) {
    HASH_DEL(stats, s);
    free(s);
  }
}

int main(int argc, char **argv)
{
  char header[22];
  int buflen = 65535;
  unsigned char buf[buflen];
  int ret = 0;

  const char *args = "h";

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

  const char *filename = argv[optind];
  gzFile file = gzopen (filename, "rb");
  if (file == NULL)
    {
      fprintf (stderr, "ERROR: unable to open %s\n", filename);
      ret = 1;
      goto cleanup;
    }
  LWES_CHAR event_name[SHORT_STRING_MAX+1];

  /* read a header from the file */
  while (gzread (file, header, 22) == 22)
    {
      unsigned short size = header_payload_length (header);
      size_t offset = 0;

      /* read an event from the file */
      if (gzread (file, buf, size) != size)
        {
          fprintf (stderr, "ERROR: failure reading journal\n");
          ret=1;
          break;
        }
      if (! unmarshall_SHORT_STRING (event_name, SHORT_STRING_MAX+1, buf, size, &offset))
        {
          fprintf (stderr, "ERROR: failure reading event_name\n");
          ret=1;
          break;
        }
      upsert_stats (event_name, size);
    }
  gzclose (file);

  print_stats ();

cleanup:
  exit (ret);
}
