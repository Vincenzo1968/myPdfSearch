// MyPdfSearch.cs -- P/Invoke binding for libmypdfsearch (.NET 6 or later).
//
// The native library is "mypdfsearch": the runtime looks for mypdfsearch.dll (Windows),
// libmypdfsearch.so (Linux; the development symlink installed with the library) or
// libmypdfsearch.dylib (macOS). Put it next to the executable or in the library path.
//
// All strings are UTF-8. Exceptions thrown by a callback stop the processing and are re-thrown
// by ProcessFile()/ProcessDirectory(): they never cross the native library.
using System;
using System.Runtime.InteropServices;

namespace MyPdfSearch
{
    public enum Status
    {
        Ok = 0, InvalidArgument = 1, OutOfMemory = 2, FileOpen = 3, NotAPdf = 4, Corrupted = 5,
        Encrypted = 6, Unsupported = 7, Cancelled = 8, FileErrors = 9, Internal = 99
    }

    public enum MessageLevel { Info = 0, Warning = 1, Error = 2 }

    public sealed class MyPdfException : Exception
    {
        public Status Status { get; }
        public MyPdfException(Status status, string message) : base(message) { Status = status; }
    }

    internal static class Native
    {
        const string Lib = "mypdfsearch";

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void FileStartCb(IntPtr user, IntPtr file);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate int PageTextCb(IntPtr user, IntPtr file, int page, IntPtr text, UIntPtr byteLen);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate int WordFoundCb(IntPtr user, IntPtr file, int page, IntPtr word);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void MessageCb(IntPtr user, int level, IntPtr file, IntPtr message);

        [DllImport(Lib)] public static extern IntPtr mypdf_GetVersion();
        [DllImport(Lib)] public static extern IntPtr mypdf_StatusString(int status);
        [DllImport(Lib)] public static extern IntPtr mypdf_GetLastError(IntPtr ctx);
        [DllImport(Lib)] public static extern int mypdf_Init();
        [DllImport(Lib)] public static extern void mypdf_Shutdown();
        [DllImport(Lib)] public static extern IntPtr mypdf_CreateContext();
        [DllImport(Lib)] public static extern void mypdf_DestroyContext(IntPtr ctx);
        [DllImport(Lib)] public static extern void mypdf_SetFileStartCallback(IntPtr ctx, FileStartCb? cb, IntPtr user);
        [DllImport(Lib)] public static extern void mypdf_SetPageTextCallback(IntPtr ctx, PageTextCb? cb, IntPtr user);
        [DllImport(Lib)] public static extern void mypdf_SetWordFoundCallback(IntPtr ctx, WordFoundCb? cb, IntPtr user);
        [DllImport(Lib)] public static extern void mypdf_SetMessageCallback(IntPtr ctx, MessageCb? cb, IntPtr user);
        [DllImport(Lib)] public static extern int mypdf_SetSearchWords(IntPtr ctx, [MarshalAs(UnmanagedType.LPUTF8Str)] string? words);
        [DllImport(Lib)] public static extern int mypdf_SetPageRange(IntPtr ctx, int first, int last);
        [DllImport(Lib)] public static extern int mypdf_SetOptionInt(IntPtr ctx, int option, long value);
        [DllImport(Lib)] public static extern int mypdf_ProcessFile(IntPtr ctx, [MarshalAs(UnmanagedType.LPUTF8Str)] string path);
        [DllImport(Lib)] public static extern int mypdf_ProcessDirectory(IntPtr ctx, [MarshalAs(UnmanagedType.LPUTF8Str)] string path, int recursive);
        [DllImport(Lib)] public static extern void mypdf_GetStats(IntPtr ctx, out long processed, out long withErrors, out long withWarnings);
    }

    /// <summary>Process-wide initialization (mypdf_Init / mypdf_Shutdown): create one and keep it alive.</summary>
    public sealed class Library : IDisposable
    {
        public Library()
        {
            int st = Native.mypdf_Init();
            if (st != 0) throw new MyPdfException((Status)st, "mypdf_Init failed");
        }
        public static string Version => Marshal.PtrToStringUTF8(Native.mypdf_GetVersion()) ?? "";
        public static string StatusString(Status s) => Marshal.PtrToStringUTF8(Native.mypdf_StatusString((int)s)) ?? "";
        public void Dispose() => Native.mypdf_Shutdown();
    }

    public sealed class Context : IDisposable
    {
        IntPtr _ctx;
        Exception? _pending;

        // The delegates must stay referenced while registered: otherwise the GC collects them.
        Native.FileStartCb? _fileStart;
        Native.PageTextCb? _pageText;
        Native.WordFoundCb? _wordFound;
        Native.MessageCb? _message;

        public Context()
        {
            _ctx = Native.mypdf_CreateContext();
            if (_ctx == IntPtr.Zero) throw new MyPdfException(Status.OutOfMemory, "mypdf_CreateContext failed");
        }

        public string LastError => Marshal.PtrToStringUTF8(Native.mypdf_GetLastError(_ctx)) ?? "";

        void Check(int st)
        {
            if (st != 0) throw new MyPdfException((Status)st, Library.StatusString((Status)st) + ": " + LastError);
        }

        // ---- options ----------------------------------------------------------------------
        public void SetSearchWords(string words) => Check(Native.mypdf_SetSearchWords(_ctx, words));
        public void SetPageRange(int first, int last) => Check(Native.mypdf_SetPageRange(_ctx, first, last));
        public void SetSortGlyphsByRow(bool on) => Check(Native.mypdf_SetOptionInt(_ctx, 1, on ? 1 : 0));

        // ---- callbacks (return false from OnPageText / OnWordFound to stop) ------------------
        public void OnFileStart(Action<string> fn)
        {
            _fileStart = (u, f) => { try { fn(Marshal.PtrToStringUTF8(f) ?? ""); } catch (Exception e) { _pending = e; } };
            Native.mypdf_SetFileStartCallback(_ctx, _fileStart, IntPtr.Zero);
        }

        public void OnPageText(Func<string, int, string, bool> fn)
        {
            _pageText = (u, f, page, text, len) =>
            {
                try { return fn(Marshal.PtrToStringUTF8(f) ?? "", page, Marshal.PtrToStringUTF8(text, (int)len) ?? "") ? 0 : 1; }
                catch (Exception e) { _pending = e; return 1; }
            };
            Native.mypdf_SetPageTextCallback(_ctx, _pageText, IntPtr.Zero);
        }

        public void OnWordFound(Func<string, int, string, bool> fn)
        {
            _wordFound = (u, f, page, w) =>
            {
                try { return fn(Marshal.PtrToStringUTF8(f) ?? "", page, Marshal.PtrToStringUTF8(w) ?? "") ? 0 : 1; }
                catch (Exception e) { _pending = e; return 1; }
            };
            Native.mypdf_SetWordFoundCallback(_ctx, _wordFound, IntPtr.Zero);
        }

        public void OnMessage(Action<MessageLevel, string, string> fn)
        {
            _message = (u, level, f, m) =>
            {
                try { fn((MessageLevel)level, Marshal.PtrToStringUTF8(f) ?? "", Marshal.PtrToStringUTF8(m) ?? ""); }
                catch (Exception e) { _pending = e; }
            };
            Native.mypdf_SetMessageCallback(_ctx, _message, IntPtr.Zero);
        }

        // ---- processing -------------------------------------------------------------------
        Status Run(int st)
        {
            if (_pending != null) { var e = _pending; _pending = null; throw e; }
            return (Status)st;
        }

        public Status ProcessFile(string path) => Run(Native.mypdf_ProcessFile(_ctx, path));
        public Status ProcessDirectory(string path, bool recursive = true) => Run(Native.mypdf_ProcessDirectory(_ctx, path, recursive ? 1 : 0));

        public (long Processed, long WithErrors, long WithWarnings) Stats()
        {
            Native.mypdf_GetStats(_ctx, out long p, out long e, out long w);
            return (p, e, w);
        }

        public void Dispose()
        {
            if (_ctx != IntPtr.Zero) { Native.mypdf_DestroyContext(_ctx); _ctx = IntPtr.Zero; }
            GC.SuppressFinalize(this);
        }
    }
}
