bin_PROGRAMS = mypdfsearch-gtk4

mypdfsearch_gtk4_SOURCES = main.c
mypdfsearch_gtk4_CFLAGS = -Wall -W $(GTK4_CFLAGS)
mypdfsearch_gtk4_CPPFLAGS = -I$(top_srcdir)/src
if MYPDF_STATIC_ONLY
mypdfsearch_gtk4_CPPFLAGS += -DMYPDF_STATIC=1
endif
mypdfsearch_gtk4_LDADD = $(top_builddir)/src/libmypdfsearch.la $(GTK4_LIBS)

exampledir = $(docdir)/examples/c/gtk4
example_DATA = main.c
