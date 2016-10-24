# BEGIN: Variables to change.

# any additional includes to add to the compile lines

myincludes = $(LWES_CFLAGS) $(MONDEMAND_CFLAGS) -I libut/include

# any additional files to clean up with 'make clean'

mycleanfiles =

# any additional files to clean up with 'make maintainer-clean'

mymaintainercleanfiles = config.h.in

# list of source files comprising program

commonsource = \
  opt.c \
  log.c \
  lwes_mondemand.c \
  header.c \
  journal_factory.c \
  journal_file.c \
  journal_gz.c \
  process_model.c \
  queue_factory.c \
  queue_mqueue.c \
  queue_msg.c \
  queue_to_journal.c \
  sig.c \
  xport.c \
  xport_to_queue.c \
  xport_udp.c \
  serial_model.c \
  thread_model.c \
  stats.c \
  time_utils.c \
  rename_journal.c

myheaderfiles =       \
  header.h            \
  journal_file.h      \
  journal_gz.h        \
  journal.h           \
  log.h               \
  lwes_mondemand.h    \
  marshal.h           \
  opt.h               \
  perror.h            \
  process_model.h     \
  queue.h             \
  queue_mqueue.h      \
  queue_msg.h         \
  queue_to_journal.h  \
  rename_journal.h    \
  serial_model.h      \
  thread_model.h      \
  sig.h               \
  stats.h             \
  time_utils.h        \
  xport.h             \
  xport_to_queue.h    \
  xport_udp.h

journallersource = \
  lwes-journaller.c

rotatesourcefiles = \
  lwes-journaller-rotate.c

queuejournalsources = \
  queue_to_journal_main.c

xportsources = \
  xport_to_queue_main.c

bin_PROGRAMS =           \
  lwes-journaller        \
  lwes-journaller-rotate \
  lwes-journal-emitter   \
  lwes-journal-split     \
  lwes-journal-stats     \
  queue_to_journal       \
  xport_to_queue

# any additional files to add to the distribution

myextradist = ${myheaderfiles} libut

lwes_journaller_SOURCES = ${commonsource} ${journallersource}
lwes_journaller_rotate_SOURCES = ${commonsource} ${rotatesourcefiles}
lwes_journal_emitter_SOURCES = \
  lwes_mondemand.c \
  log.c \
  opt.c \
  sig.c \
  header.c \
  time_utils.c \
  lwes-journal-emitter.c
lwes_journal_split_SOURCES = \
  lwes_mondemand.c \
  log.c \
  opt.c \
  sig.c \
  header.c \
  time_utils.c \
  lwes-journal-split.c
lwes_journal_stats_SOURCES = \
  lwes_mondemand.c \
  log.c \
  opt.c \
  sig.c \
  header.c \
  time_utils.c \
  lwes-journal-stats.c
queue_to_journal_SOURCES = ${commonsource} ${queuejournalsources}
xport_to_queue_SOURCES = ${commonsource} ${xportsources}

LDADD = $(LWES_LIBS) $(MONDEMAND_LIBS) $(Z_LIBS) $(THREAD_LIBS) -lpopt

lwes_journaller_LDFLAGS=-rdynamic
lwes_journaller_rotate_LDFLAGS=
lwes_journal_emitter_LDFLAGS=
lwes_journal_split_LDFLAGS=
lwes_journal_stats_LDFLAGS=
queue_to_journal_LDFLAGS=
xport_to_queue_LDFLAGS=

# END: Variables to change
# past here, hopefully, there is no need to edit anything

INCLUDES = ${myincludes}

EXTRA_DIST =                            \
  ${myextradist}

CLEANFILES =                            \
  *.bb                                  \
  *.bbg                                 \
  *.da                                  \
  *.gcno                                \
  *.gcda                                \
  ${mycleanfiles}

MAINTAINERCLEANFILES =                  \
  Makefile.in                           \
  ${mymaintainercleanfiles}

# .BEGIN is ignored by GNU make so we can use it as a guard
.BEGIN:
	@echo "error: GNU Make is required, try aliasing gmake to make"
	@exit 1
