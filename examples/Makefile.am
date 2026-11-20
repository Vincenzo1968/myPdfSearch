# Examples of use of libmypdfsearch.
#   c/console   the command line program 'mypdfsearch' (always built and installed)
#   c/gtk4      graphical example, built with --enable-examples if GTK4 is found
#   cpp/console C++17 wrapper + console example, built with --enable-examples
#   python      ctypes binding + script (not built: sources installed)
#   csharp      P/Invoke binding + .NET console project (not built: sources installed)
SUBDIRS = c/console

if BUILD_EXAMPLE_GTK4
SUBDIRS += c/gtk4
endif

if BUILD_EXAMPLE_CPP
SUBDIRS += cpp/console
endif

DIST_SUBDIRS = c/console c/gtk4 cpp/console

exampledir = $(docdir)/examples
nobase_dist_example_DATA = \
    README.md \
    python/mypdfsearch.py \
    python/extract_text.py \
    csharp/MyPdfSearch.cs \
    csharp/Program.cs \
    csharp/MyPdfSearchExample.csproj
