# libmypdfsearch examples

All examples use **only the public API** (`mypdfsearch.h`).

| Folder | Language | What | Built |
|---|---|---|---|
| `c/console` | C | the command line program `mypdfsearch` | always |
| `c/gtk4` | C + GTK4 | graphical front end | `--enable-examples` (needs GTK4) |
| `cpp/console` | C++17 | header-only RAII wrapper (`mypdfsearch.hpp`) + console program | `--enable-examples` |
| `python` | Python 3 (ctypes) | binding (`mypdfsearch.py`) + `extract_text.py` | no (scripts) |
| `csharp` | C# (.NET 8, P/Invoke) | binding (`MyPdfSearch.cs`) + console project | no (`dotnet run`) |

After `make install` the sources are in `<prefix>/share/doc/mypdfsearch/examples/`.
The installed library is described by `pkg-config`:

    cc main.c $(pkg-config --cflags --libs mypdfsearch) -o prog

## Rules valid for every language

* Call `mypdf_Init()` once per process (and `mypdf_Shutdown()` at the end), then create a
  `mypdf_Context` (`mypdf_CreateContext`). One context per thread.
* All strings are UTF-8 (`char *`). The library never prints, never writes files, never calls `setlocale`:
  text, found words and messages come through the callbacks, each with a `user_data` pointer.
* A callback must never raise an exception / `longjmp` towards the library: catch it, return nonzero (stop
  the processing) and report it after the `mypdf_Process*` call (the bindings do it for you).
* Return value of `mypdf_ProcessFile`: `MYPDF_OK`, `MYPDF_ERR_ENCRYPTED` (file skipped: password missing,
  wrong or cancelled), `MYPDF_ERR_FILE_ERRORS` (non fatal errors), `MYPDF_ERR_CANCELLED`, ...
  `mypdf_ProcessDirectory` keeps going after the errors of single files.

## Running the scripts without installing

    cd examples/python
    MYPDFSEARCH_LIB=../../src/.libs/libmypdfsearch.so python3 extract_text.py file.pdf

    cd examples/csharp
    LD_LIBRARY_PATH=../../src/.libs dotnet run -- file.pdf
