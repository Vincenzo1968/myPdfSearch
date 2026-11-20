bin_PROGRAMS = mypdfsearch-cpp

mypdfsearch_cpp_SOURCES = main.cpp mypdfsearch.hpp
mypdfsearch_cpp_CXXFLAGS = -std=c++17 -Wall -W -pedantic
mypdfsearch_cpp_CPPFLAGS = -I$(top_srcdir)/src
if MYPDF_STATIC_ONLY
mypdfsearch_cpp_CPPFLAGS += -DMYPDF_STATIC=1
endif
mypdfsearch_cpp_LDADD = $(top_builddir)/src/libmypdfsearch.la

exampledir = $(docdir)/examples/cpp/console
example_DATA = main.cpp mypdfsearch.hpp
