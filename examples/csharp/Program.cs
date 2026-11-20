// .NET console example: text extraction and word search with libmypdfsearch.
//
//   dotnet run -- <file.pdf | folder> [words]
using System;
using System.IO;
using System.Text;
using MyPdfSearch;

if (args.Length < 1 || args.Length > 2)
{
    Console.Error.WriteLine("usage: MyPdfSearchExample <file.pdf | folder> [words]");
    return 1;
}

Console.OutputEncoding = new UTF8Encoding(false);
string path = args[0];
string words = args.Length == 2 ? args[1] : "";

using var lib = new Library();      // mypdf_Init / mypdf_Shutdown
using var ctx = new Context();

Console.Error.WriteLine("libmypdfsearch " + Library.Version);

ctx.OnMessage((level, file, msg) =>
    Console.Error.WriteLine($"[{level.ToString().ToUpperInvariant()}] {(file.Length > 0 ? file + ": " : "")}{msg}"));
ctx.OnFileStart(file => Console.WriteLine("== " + file));

if (words.Length > 0)
{
    ctx.SetSearchWords(words);
    ctx.OnWordFound((file, page, word) => { Console.WriteLine($"  '{word}' found on page {page}"); return true; });
}
else
{
    ctx.OnPageText((file, page, text) => { Console.WriteLine($"--- page {page} ---\n{text}"); return true; });
}

Status st = Directory.Exists(path) ? ctx.ProcessDirectory(path) : ctx.ProcessFile(path);

var (done, withErrors, withWarnings) = ctx.Stats();
Console.Error.WriteLine($"{done} file(s), {withErrors} with errors, {withWarnings} with warnings");

if (st != Status.Ok && st != Status.Encrypted)
{
    Console.Error.WriteLine($"{Library.StatusString(st)}: {ctx.LastError}");
    return st == Status.FileErrors ? 2 : 1;
}
return 0;
