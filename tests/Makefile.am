# BEGIN: Variables to change.

# any additional includes to add to the compile lines

myincludes =

# any additional files to add to the distribution

myextradist =

# any additional files to clean up with 'make clean'

mycleanfiles =

# any additional files to clean up with 'make maintainer-clean'

mymaintainercleanfiles =

# list of test programs, in dependency order

mytests =

# list of test scripts, in dependency order

myscripttests =

# END: Variables to change
# past here, hopefully, there is no need to edit anything

INCLUDES = -I../src ${myincludes}

check_PROGRAMS = ${mytests}

check_SCRIPTS  = ${myscripttests}

# globally added to all instances of valgrind calls
VALGRIND_OPTS = ${myextravalgrindopts}

# globally added to all memcheck calls
VALGRIND_MEM_OPTS = --tool=memcheck \
                    --num-callers=10 \
                    ${myextravalgrindmemopts}

# globally added to all leakcheck calls
VALGRIND_LEAK_OPTS = --tool=memcheck \
                     --num-callers=10 \
                     --leak-check=yes \
                     --leak-resolution=high \
                     --show-reachable=yes \
                     ${myextravalgrindleakopts}

memcheck: $(check_PROGRAMS)
        @for x in $(check_PROGRAMS);                                     \
          do                                                             \
            $(MAKE) memcheck-$$x;                                        \
          done

leakcheck: $(check_PROGRAMS)
        @for x in $(check_PROGRAMS);                                     \
          do                                                             \
            $(MAKE) leakcheck-$$x;                                       \
          done

memcheck-%: %
        @echo "*****************************************";                \
        echo "MEMCHECK: $<";                                             \
        echo "ARGS: ${VALGRIND_OPTS} ${VALGRIND_MEM_OPTS} ${$<_VALGRIND_MEM_OPTS}";\
        $(LIBTOOL) --mode=execute                                        \
          valgrind                                                       \
            ${VALGRIND_OPTS}                                             \
            ${VALGRIND_MEM_OPTS}                                         \
            ${$<_VALGRIND_MEM_OPTS} ./$<

leakcheck-%: %
        @echo "*****************************************";               \
        echo "LEAKCHECK: $<";                                           \
        echo "ARGS: ${VALGRIND_OPTS} ${VALGRIND_LEAK_OPTS} ${$<_VALGRIND_LEAK_OPTS}";                                           \
        $(LIBTOOL) --mode=execute                                       \
          valgrind                                                      \
            ${VALGRIND_OPTS}                                            \
            ${VALGRIND_LEAK_OPTS}                                       \
            ${$<_VALGRIND_LEAK_OPTS}  ./$<

# NB: TESTS are ordered in dependency order

TESTS = $(patsubst %, testwrapper-%, ${mytests}) ${myscripttests}

testwrapper-%: % test-wrapper.sh test-wrapper.sh.in
        @ln -sf test-wrapper.sh $@
        @chmod +x test-wrapper.sh

EXTRA_DIST =                            \
    ${myextradist}

CLEANFILES =                            \
    testwrapper-*                       \
    *.bb                                \
    *.bbg                               \
    *.da                                \
    *.gcno                              \
    *.gcda                              \
    *.gcov                              \
    test-wrapper.sh                     \
    ${mycleanfiles}

MAINTAINERCLEANFILES =                  \
    Makefile.in                         \
    skeletor-tests.am                   \
    ${mymaintainercleanfiles}

# Tell make to ignore these any files that match these targets.
.PHONY: memcheck leakcheck

# .BEGIN is ignored by GNU make so we can use it as a guard
.BEGIN:
	@echo "error: GNU Make is required, try aliasing gmake to make"
	@exit 1

