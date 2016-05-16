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

int main(int argc, char **argv)
{
  char header[22];
  unsigned char buf[65535];
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

  int total_count = 0;

  unsigned long long start_file_timestamp = 0ULL;
  unsigned long long end_file_timestamp = 0ULL;

  const char *filename = argv[optind];
  gzFile file = gzopen (filename, "rb");

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
          ret=1;
          break;
        }

      total_count++;
    }
  gzclose (file);

  fprintf (stdout, "%s\t%d\n", filename, total_count);

cleanup:
  exit (ret);
}
