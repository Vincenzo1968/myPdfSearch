#!/usr/bin/env python3
"""Extracts the text of a PDF, or searches words in a PDF or in a folder.

    python3 extract_text.py file.pdf                 # text of every page
    python3 extract_text.py folder "word1 word2"     # where the words are found
    python3 extract_text.py folder "pre" --prefix    # words of the text that START with 'pre'
                                                     # (also --suffix, --contains)
"""
import os
import sys

import mypdfsearch as mp


MODES = {"--prefix": mp.MATCH_PREFIX, "--suffix": mp.MATCH_SUFFIX, "--contains": mp.MATCH_CONTAINS}


def main(argv):
    flags = [a for a in argv[1:] if a in MODES]
    argv = [argv[0]] + [a for a in argv[1:] if a not in MODES]
    if len(argv) < 2 or len(argv) > 3 or len(flags) > 1 or (flags and len(argv) < 3):
        print("usage: %s <file.pdf | folder> [words [--prefix | --suffix | --contains]]" % argv[0], file=sys.stderr)
        return 1
    path = argv[1]
    words = argv[2] if len(argv) == 3 else ""

    with mp.Context() as ctx:
        print("libmypdfsearch", mp.version(), file=sys.stderr)
        ctx.on_message(lambda lvl, f, m: print("[%s] %s%s" % (("INFO", "WARNING", "ERROR")[lvl], f + ": " if f else "", m), file=sys.stderr))
        ctx.on_file_start(lambda f: print("==", f))
        if words:
            ctx.set_search_words(words)
            if flags:
                ctx.set_match_mode(MODES[flags[0]])
            ctx.on_word_found(lambda f, page, w: print("  '%s' found on page %d" % (w, page)))
        else:
            ctx.on_page_text(lambda f, page, text: print("--- page %d ---\n%s" % (page, text)))

        st = ctx.process_directory(path) if os.path.isdir(path) else ctx.process_file(path)
        done, errs, warns = ctx.stats()
        print("%d file(s), %d with errors, %d with warnings" % (done, errs, warns), file=sys.stderr)
        if st not in (mp.MYPDF_OK, mp.MYPDF_ERR_ENCRYPTED):
            print("%s: %s" % (mp.status_string(st), ctx.last_error()), file=sys.stderr)
            return 2 if st == mp.MYPDF_ERR_FILE_ERRORS else 1
    return 0


if __name__ == "__main__":
    # stdout in UTF-8 whatever the console
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.exit(main(sys.argv))
