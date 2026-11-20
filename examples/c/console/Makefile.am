# Command line program: uses ONLY the public API (mypdfsearch.h).
bin_PROGRAMS = mypdfsearch

mypdfsearch_SOURCES = \
    main.c \
    mypdf_cli.c mypdf_cli.h \
    mytime.c mytime.h

mypdfsearch_CFLAGS = -Wall -W -pedantic

if DEBUG_BUILD
mypdfsearch_CPPFLAGS = -I$(top_srcdir)/src -DMYPDFSEARCH_BUILD_DEBUG=1
else
mypdfsearch_CPPFLAGS = -I$(top_srcdir)/src -DMYPDFSEARCH_BUILD_RELEASE=1
endif

if SHOW_TIME
mypdfsearch_CPPFLAGS += -DMYPDFSEARCH_SHOW_TIME=1
endif

# On Windows a program that links the static library must define MYPDF_STATIC (see mypdfsearch.h).
if MYPDF_STATIC_ONLY
mypdfsearch_CPPFLAGS += -DMYPDF_STATIC=1
endif

mypdfsearch_LDADD = $(top_builddir)/src/libmypdfsearch.la

# The sources are also installed, as a reference for those who use the library.
exampledir = $(docdir)/examples/c/console
example_DATA = main.c mypdf_cli.c mypdf_cli.h mytime.c mytime.h
