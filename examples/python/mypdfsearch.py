"""mypdfsearch.py -- Python binding (ctypes) for libmypdfsearch. No dependencies, Python >= 3.8.

    import mypdfsearch
    with mypdfsearch.Context() as ctx:
        ctx.on_page_text(lambda f, page, text: print(page, text))   # return False to stop
        ctx.process_file("document.pdf")

All strings are UTF-8 (ctypes bytes <-> str conversions are done here). An exception raised in a
callback stops the processing and is re-raised by process_file()/process_directory().
The library is searched in the usual places; set the MYPDFSEARCH_LIB environment variable to a
full path to use a specific file.
"""
import ctypes
import ctypes.util
import os
import sys

MYPDF_OK = 0
MYPDF_ERR_INVALID_ARGUMENT = 1
MYPDF_ERR_OUT_OF_MEMORY = 2
MYPDF_ERR_FILE_OPEN = 3
MYPDF_ERR_NOT_A_PDF = 4
MYPDF_ERR_CORRUPTED = 5
MYPDF_ERR_ENCRYPTED = 6
MYPDF_ERR_UNSUPPORTED = 7
MYPDF_ERR_CANCELLED = 8
MYPDF_ERR_FILE_ERRORS = 9
MYPDF_ERR_INTERNAL = 99

MSG_INFO, MSG_WARNING, MSG_ERROR = 0, 1, 2

OPT_SORT_GLYPHS = 1
OPT_SHOW_ERRORS = 2
OPT_SHOW_WARNINGS = 3


class MypdfError(Exception):
    def __init__(self, status, message):
        super().__init__(message)
        self.status = status


def _load():
    env = os.environ.get("MYPDFSEARCH_LIB")
    if env:
        return ctypes.CDLL(env)
    names = []
    found = ctypes.util.find_library("mypdfsearch")
    if found:
        names.append(found)
    if sys.platform.startswith("win"):
        names += ["mypdfsearch.dll", "libmypdfsearch-0.dll"]
    elif sys.platform == "darwin":
        names += ["libmypdfsearch.0.dylib", "libmypdfsearch.dylib"]
    else:
        names += ["libmypdfsearch.so.0", "libmypdfsearch.so"]
    last = None
    for n in names:
        try:
            return ctypes.CDLL(n)
        except OSError as e:
            last = e
    raise OSError("libmypdfsearch not found (set MYPDFSEARCH_LIB): %s" % last)


_lib = _load()

# CFUNCTYPE = cdecl, as MYPDF_CALL.
_FileStartCb = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_char_p)
_PageStartCb = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int32)
_PageTextCb = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int32,
                               ctypes.c_void_p, ctypes.c_size_t)
_WordFoundCb = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int32, ctypes.c_char_p)
_MessageCb = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_char_p)
_PasswordCb = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p,
                               ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t))


def _sig(name, restype, *argtypes):
    f = getattr(_lib, name)
    f.restype = restype
    f.argtypes = list(argtypes)
    return f


_Ctx = ctypes.c_void_p
_GetVersion = _sig("mypdf_GetVersion", ctypes.c_char_p)
_StatusString = _sig("mypdf_StatusString", ctypes.c_char_p, ctypes.c_int)
_GetLastError = _sig("mypdf_GetLastError", ctypes.c_char_p, _Ctx)
_Init = _sig("mypdf_Init", ctypes.c_int)
_Shutdown = _sig("mypdf_Shutdown", None)
_CreateContext = _sig("mypdf_CreateContext", _Ctx)
_DestroyContext = _sig("mypdf_DestroyContext", None, _Ctx)
_SetFileStart = _sig("mypdf_SetFileStartCallback", None, _Ctx, _FileStartCb, ctypes.c_void_p)
_SetPageStart = _sig("mypdf_SetPageStartCallback", None, _Ctx, _PageStartCb, ctypes.c_void_p)
_SetPageText = _sig("mypdf_SetPageTextCallback", None, _Ctx, _PageTextCb, ctypes.c_void_p)
_SetWordFound = _sig("mypdf_SetWordFoundCallback", None, _Ctx, _WordFoundCb, ctypes.c_void_p)
_SetMessage = _sig("mypdf_SetMessageCallback", None, _Ctx, _MessageCb, ctypes.c_void_p)
_SetPassword = _sig("mypdf_SetPasswordCallback", None, _Ctx, _PasswordCb, ctypes.c_void_p)
_SetSearchWords = _sig("mypdf_SetSearchWords", ctypes.c_int, _Ctx, ctypes.c_char_p)
_SetPageRange = _sig("mypdf_SetPageRange", ctypes.c_int, _Ctx, ctypes.c_int32, ctypes.c_int32)
_SetOptionInt = _sig("mypdf_SetOptionInt", ctypes.c_int, _Ctx, ctypes.c_int, ctypes.c_int64)
_ProcessFile = _sig("mypdf_ProcessFile", ctypes.c_int, _Ctx, ctypes.c_char_p)
_ProcessDirectory = _sig("mypdf_ProcessDirectory", ctypes.c_int, _Ctx, ctypes.c_char_p, ctypes.c_int32)
_GetStats = _sig("mypdf_GetStats", None, _Ctx, ctypes.POINTER(ctypes.c_int64),
                 ctypes.POINTER(ctypes.c_int64), ctypes.POINTER(ctypes.c_int64))

_init_count = 0


def version():
    return _GetVersion().decode("utf-8")


def status_string(status):
    return _StatusString(status).decode("utf-8")


def init():
    """mypdf_Init: once per process (done automatically by Context; calls are counted)."""
    global _init_count
    if _init_count == 0:
        st = _Init()
        if st != MYPDF_OK:
            raise MypdfError(st, "mypdf_Init: " + status_string(st))
    _init_count += 1


def shutdown():
    global _init_count
    if _init_count > 0:
        _init_count -= 1
        if _init_count == 0:
            _Shutdown()


def _s(b):
    return b.decode("utf-8", "replace") if b is not None else ""


class Context:
    """A processing context. Not thread-safe: use one per thread."""

    def __init__(self):
        init()
        self._h = _CreateContext()
        if not self._h:
            shutdown()
            raise MypdfError(MYPDF_ERR_OUT_OF_MEMORY, "mypdf_CreateContext failed")
        self._cb = {}            # keeps the ctypes callbacks alive (otherwise garbage collected -> crash)
        self._pending = None     # exception raised by a callback

    # ---- life cycle -----------------------------------------------------------------------
    def close(self):
        if self._h:
            _DestroyContext(self._h)
            self._h = None
            shutdown()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass

    # ---- helpers --------------------------------------------------------------------------
    def _check(self, st):
        if st != MYPDF_OK:
            raise MypdfError(st, "%s: %s" % (status_string(st), self.last_error()))

    def _guard(self, fn, default):
        def wrapper(*a):
            try:
                return fn(*a)
            except BaseException as e:      # never let an exception cross the C library
                self._pending = e
                return default
        return wrapper

    def last_error(self):
        return _s(_GetLastError(self._h))

    # ---- options --------------------------------------------------------------------------
    def set_search_words(self, words):
        self._check(_SetSearchWords(self._h, words.encode("utf-8")))

    def set_page_range(self, first=0, last=0):
        self._check(_SetPageRange(self._h, first, last))

    def set_sort_glyphs_by_row(self, on=True):
        self._check(_SetOptionInt(self._h, OPT_SORT_GLYPHS, 1 if on else 0))

    # ---- callbacks (return False from on_page_text/on_word_found to stop) -------------------
    def on_file_start(self, fn):
        cb = _FileStartCb(self._guard(lambda u, f: fn(_s(f)), None))
        self._cb["file_start"] = cb
        _SetFileStart(self._h, cb, None)

    def on_page_text(self, fn):
        cb = _PageTextCb(self._guard(
            lambda u, f, page, text, n: 0 if fn(_s(f), page, ctypes.string_at(text, n).decode("utf-8", "replace")) is not False else 1, 1))
        self._cb["page_text"] = cb
        _SetPageText(self._h, cb, None)

    def on_word_found(self, fn):
        cb = _WordFoundCb(self._guard(
            lambda u, f, page, w: 0 if fn(_s(f), page, _s(w)) is not False else 1, 1))
        self._cb["word_found"] = cb
        _SetWordFound(self._h, cb, None)

    def on_message(self, fn):
        cb = _MessageCb(self._guard(lambda u, lvl, f, m: fn(lvl, _s(f), _s(m)), None))
        self._cb["message"] = cb
        _SetMessage(self._h, cb, None)

    # ---- processing -----------------------------------------------------------------------
    def _run(self, st):
        if self._pending is not None:
            e, self._pending = self._pending, None
            raise e
        return st

    def process_file(self, path):
        """Returns the status (MYPDF_OK, MYPDF_ERR_ENCRYPTED, MYPDF_ERR_FILE_ERRORS, ...)."""
        return self._run(_ProcessFile(self._h, os.fsencode(path)))

    def process_directory(self, path, recursive=True):
        return self._run(_ProcessDirectory(self._h, os.fsencode(path), 1 if recursive else 0))

    def stats(self):
        a, b, c = ctypes.c_int64(), ctypes.c_int64(), ctypes.c_int64()
        _GetStats(self._h, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c))
        return a.value, b.value, c.value
