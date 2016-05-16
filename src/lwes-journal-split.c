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

#define MAX_FILE_PARTS 25

static const char help[] =
  "lwes-journal-split [options] <journal>"                             "\n"
  ""                                                                   "\n"
  "  where options are:"                                               "\n"
  ""                                                                   "\n"
  "    -n [one argument]"                                              "\n"
  "       The number of events per split file."                        "\n"
  ""                                                                   "\n"
  "    -h"                                                             "\n"
  "       show this message"                                           "\n"
  ""                                                                   "\n"
  "  arguments are specified as -option <value> or -option<value>"     "\n"
  ""                                                                   "\n";

static bool check_num_and_length (const char *word, const size_t len)
{
  if (strlen (word) != len)
    {
      return false;
    }
  else
    {
      for (unsigned int i = 0 ; i < len; i++)
        {
          if (word[i] < '0' || word[i] > '9')
            {
              return false;
            }
        }
    }
  return true;
}

int main(int argc, char **argv)
{
  char header[22];
  unsigned char buf[65535];
  int ret = 0;

  const char *args = "n:h";
  int number = 10000;    /* (n) number per file */

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
  int current_count = 0;

  unsigned long long start_file_timestamp = 0ULL;
  unsigned long long end_file_timestamp = 0ULL;

  const char *filename = argv[optind];
  const char *tmpfile = "lwes-journal-split-tmp.gz";
  char newfile[PATH_MAX];
  int newfile_idx = 0;
  char *buffer = strdup(filename);
  char *tofree = buffer;
  int file_part_count = 0;
  const char *file_parts[MAX_FILE_PARTS];
  const char *sep = ".";
  const char *empty = "";
  char *part;
  for (int i = 0; i < MAX_FILE_PARTS; i++)
    {
      file_parts[MAX_FILE_PARTS] = empty;
    }
  while ((part = strsep (&buffer, sep)) && file_part_count < MAX_FILE_PARTS)
    {
      file_parts[file_part_count++] = part;
    }
  for (int i = 0 ; i < file_part_count; i++)
    {
      printf ("part[%d] = %s\n",i,file_parts[i]);
    }
  newfile[newfile_idx] = '\0';
  if (strcmp (file_parts[file_part_count-1],"gz") == 0)
    {
      if (check_num_and_length (file_parts[file_part_count-2], 10)
          && check_num_and_length (file_parts[file_part_count-3], 10))
        {
          for (int i = 0; i < (file_part_count-3); i++)
            {
              strcat (newfile, file_parts[i]);
              strcat (newfile, sep);
            }
        }
      else
        {
          fprintf (stderr,
                   "WARNING: journal file name format is non-standard\n");
          for (int i = 0; i < (file_part_count-1); i++)
            {
              strcat (newfile, file_parts[i]);
              strcat (newfile, sep);
            }
        }
    }
  else
    {
      fprintf (stderr,
               "ERROR: journal must be a gzip file with .gz extension\n");
      free (tofree);
      ret = 1;
      goto cleanup;
    }
  free (tofree);

  newfile_idx = strlen (newfile);
  fprintf (stderr, "newfile prefix is %s of length %d\n",newfile, newfile_idx);

  gzFile tmp = gzopen (tmpfile, "wb");
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
      if (gzwrite (tmp, header, 22) != 22)
        {
          fprintf (stderr, "ERROR: failure writing journal\n");
          ret=1;
          break;
        }

      /* read an event from the file */
      if (gzread (file, buf, size) != size)
        {
          fprintf (stderr, "ERROR: failure reading journal\n");
          ret=1;
          break;
        }
      if (gzwrite (tmp, buf, size) != size)
        {
          fprintf (stderr, "ERROR: failure writing journal\n");
          ret=1;
          break;
        }
      current_count++;
      total_count++;
      if (current_count == number) {
        gzclose (tmp);
        fprintf (stderr, "split from %llu to %llu %d\n",
                 start_file_timestamp, end_file_timestamp, current_count);
        char renamedfile[PATH_MAX];
        snprintf (renamedfile, sizeof (renamedfile), "%s%llu.%llu%s",
                  newfile,start_file_timestamp,end_file_timestamp, ".gz");
        if (rename (tmpfile, renamedfile) < 0)
          {
            fprintf (stderr, "ERROR : failed to rename %s to %s\n",
                     tmpfile, renamedfile);
            ret=1;
            break;
          }
        start_file_timestamp = 0ULL;
        current_count = 0;
        tmp = gzopen (tmpfile, "wb");
      }
    }
  gzclose (file);
  gzclose (tmp);
  char renamedfile[PATH_MAX];
  snprintf (renamedfile, sizeof (renamedfile), "%s%llu.%llu%s",
            newfile,start_file_timestamp,end_file_timestamp, ".gz");
  if (rename (tmpfile, renamedfile) < 0)
    {
      fprintf (stderr, "ERROR : failed to rename %s to %s\n",
               tmpfile, renamedfile);
      ret=1;
    }
  fprintf (stderr, "%s has %d events\n", filename, total_count);

cleanup:
  exit (ret);
}
