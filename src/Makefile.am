# BEGIN: Variables to change.

# any additional includes to add to the compile lines

myincludes = $(LWES_CFLAGS)

# any additional files to add to the distribution

myextradist =

# any additional files to clean up with 'make clean'

mycleanfiles =

# any additional files to clean up with 'make maintainer-clean'

mymaintainercleanfiles = config.h.in

# list of source files comprising program

commonsource = \
								opt.c \
								log.c \
								header.c \
								journal_factory.c \
								journal_file.c \
								journal_gz.c \
								queue_factory.c \
								queue_mqueue.c \
								queue_msg.c \
								queue_to_journal.c \
								sig.c \
								xport.c \
								xport_to_queue.c \
								xport_udp.c \
								stats.c \
								rename_journal.c \
								peer.c

journallersource = \
								lwes-journaller.c

rotatesourcefiles = \
								journaller_rotate_logs.c

queuejournalsources = \
								queue_to_journal_main.c

xportsources = \
								xport_to_queue_main.c

mysourcefiles =

bin_PROGRAMS = \
  lwes-journaller \
	lwes-journaller-rotate \
	queue_to_journal \
	xport_to_queue

lwes_journaller_SOURCES = ${commonsource} ${journallersource}
lwes_journaller_rotate_SOURCES = ${commonsource} ${rotatesourcefiles}
queue_to_journal_SOURCES = ${commonsource} ${queuejournalsources}
xport_to_queue_SOURCES = ${commonsource} ${xportsources}

lwes_journaller_LDADD=$(LWES_LIBS) -lpopt -lltdl
lwes_journaller_rotate_LDADD=$(LWES_LIBS) -lpopt -lltdl
queue_to_journal_LDADD=$(LWES_LIBS) -lpopt -lltdl
xport_to_queue_LDADD=$(LWES_LIBS) -lpopt -lltdl

lwes_journaller_LDFLAGS=-rdynamic
lwes_journaller_rotate_LDFLAGS=
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
