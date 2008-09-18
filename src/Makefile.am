# BEGIN: Variables to change.

# any additional includes to add to the compile lines

myincludes =

# any additional files to add to the distribution

myextradist =

# any additional files to clean up with 'make clean'

mycleanfiles =

# any additional files to clean up with 'make maintainer-clean'

mymaintainercleanfiles = config.h.in

# list of source files comprising program

mysourcefiles = log.c \
                lwes-journaller.c

bin_PROGRAMS = \
  lwes-journaller

lwes_journaller_SOURCES = ${mysourcefiles}

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
