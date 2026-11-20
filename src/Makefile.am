# ---------------------------------------------------------------------------
# Library: libmypdfsearch (shared and static)
#   Public API: mypdfsearch.h (installed). Everything else is internal.
# ---------------------------------------------------------------------------
lib_LTLIBRARIES = libmypdfsearch.la

include_HEADERS = mypdfsearch.h

libmypdfsearch_la_SOURCES = \
    mypdf_api.c \
    mypdf_internal.h \
    mycontentqueuelist.c mycontentqueuelist.h \
    mypdf_utils.c mypdf_utils.h \
    mycrypt.c mycrypt.h \
    mycrypt_rc4.c mycrypt_rc4.h \
    mydecode.c mydecode.h \
    mydictionaryqueuelist.c mydictionaryqueuelist.h \
    myGenHashTable.c myGenHashTable.h \
    myinitarray.c \
    myInitPredefCMapHT.c myInitPredefCMapHT.h \
    myintqueuelist.c myintqueuelist.h \
    mynumstacklist.c mynumstacklist.h \
    myobjrefqueuelist.c myobjrefqueuelist.h \
    myoctal.c \
    myoctal.h \
    myScopeHashTable.c myScopeHashTable.h \
    mystringqueuelist.c mystringqueuelist.h \
    myTernarySearchTree.c myTernarySearchTree.h \
    myTree.c myTree.h \
    myUnicodeNFC.c myUnicodeNFC.h \
    parser.c parser.h \
    scanner.c scanner.h \
    vlEncoding.c vlEncoding.h \
    vlRedBlackTrees.c vlRedBlackTrees.h \
    cmap/cmap_Adobe-CNS1-7.h \
    cmap/cmap_Adobe-GB1-6.h \
    cmap/cmap_Adobe-Japan1-7.h \
    cmap/cmap_Adobe-Japan2-0.h \
    cmap/cmap_Adobe-KR-9.h \
    cmap/cmap_Adobe-Korea1-2.h \
    cmap/cmap_Adobe-Manga1-0.h \
    cmap/cmap_kangxi.h \
    cmap/cmap_UnicodeNfcData.h \
    cmap/cmap_UnicodeNfkcArabic.h

# Version of the binary interface (NOT the package version). Rules:
#   - code changed, interface unchanged:        REVISION + 1
#   - functions added (backward compatible):    CURRENT + 1, REVISION = 0, AGE + 1
#   - functions removed or changed (breaking):  CURRENT + 1, REVISION = 0, AGE = 0
# Format: -version-info CURRENT:REVISION:AGE
libmypdfsearch_la_LDFLAGS = -version-info 0:0:0 -no-undefined \
    -export-symbols-regex '^mypdf_'
libmypdfsearch_la_LIBADD = -lm -lz -lcrypto
libmypdfsearch_la_CPPFLAGS = $(AM_CPPFLAGS) -DMYPDF_BUILDING_DLL=1
# Only the functions of mypdfsearch.h are visible from outside the library.
libmypdfsearch_la_CFLAGS = $(AM_CFLAGS) -fvisibility=hidden

# ---------------------------------------------------------------------------
# Command line program (example 'console'): uses only the public API.
# ---------------------------------------------------------------------------
bin_PROGRAMS = mypdfsearch

mypdfsearch_SOURCES = \
    main.c \
    mypdf_cli.c mypdf_cli.h \
    mytime.c mytime.h

mypdfsearch_LDADD = libmypdfsearch.la

AM_CFLAGS = -Wall -W -pedantic -Wno-overlength-strings

if DEBUG_BUILD
AM_CPPFLAGS = -DMYPDFSEARCH_BUILD_DEBUG=1
else
AM_CPPFLAGS = -DMYPDFSEARCH_BUILD_RELEASE=1
endif

if SHOW_TIME
AM_CPPFLAGS += -DMYPDFSEARCH_SHOW_TIME=1
endif

if MYPDF_STATIC_ONLY
mypdfsearch_CPPFLAGS = $(AM_CPPFLAGS) -DMYPDF_STATIC=1
endif

# Export list for MSVC (lib.exe /DEF:mypdfsearch.def): generated from the public header.
#   make mypdfsearch.def
mypdfsearch.def: mypdfsearch.h
	( echo "EXPORTS"; sed -n 's/.*MYPDF_CALL *\(mypdf_[A-Za-z0-9_]*\) *(.*/    \1/p' $(srcdir)/mypdfsearch.h ) > $@

CLEANFILES = mypdfsearch.def

# Libreria statica: nasconde i simboli interni (vedi localize-static.sh). Alla installazione
# e anche nella cartella di compilazione (.libs/).
EXTRA_DIST = localize-static.sh

all-local: $(lib_LTLIBRARIES)
	-@if test -f .libs/libmypdfsearch.a; then $(SHELL) $(srcdir)/localize-static.sh .libs/libmypdfsearch.a; fi

install-exec-hook:
	-@if test -f "$(DESTDIR)$(libdir)/libmypdfsearch.a"; then $(SHELL) $(srcdir)/localize-static.sh "$(DESTDIR)$(libdir)/libmypdfsearch.a"; fi
