# myPdfSearch tests

Shell test suites for `mypdfsearch` (the command line program built on top of `libmypdfsearch`).

## Requirements

* `mypdfsearch` installed (in the `PATH`) or built in `examples/c/console/`
* bash, python3
* optional: `qpdf` and `pdftotext` (poppler-utils) for the encryption tests;
  `xelatex`, `lualatex`, `polyglossia` and the Noto fonts to build the multilingual PDFs

## Running

    ./run_tests.sh            # everything
    ./run_tests.sh --quick    # skips the slow suites
    ./test_pdf_sintetici.sh   # a single suite

Exit status of every script: `0` passed, `77` skipped (missing prerequisite or fixture), other = failed.

## Suites

| Script | What it checks | Needs |
|---|---|---|
| `test_pdf_sintetici.sh` | synthetic PDFs written on the fly: parser robustness, exit codes, warnings | python3 |
| `test_crypt_pdf_new.sh` | RC4/AES encrypted PDFs, right/wrong passwords, text identical to the reference (also through a pseudo terminal) | qpdf, pdftotext, python3 |
| `test_actualtext_multilingua.sh` | text extraction from the multilingual PDFs | `latex/build_pdfs.sh` |
| `test_cmap_unicode.sh` | CMap/Unicode regressions on Japanese PDFs | optional fixtures |
| `test_regressioni_cjk.sh` | first page of several CJK PDFs is extracted and not empty | optional fixtures |
| `test_ricerca_parziale.sh` | `--prefix/--suffix/--contains` consistent with the whole-word search; no crash scanning a folder | any PDFs under `fixtures/` (or the LaTeX ones) |
| `test_isartor.sh` | the Isartor PDF/A-1b suite: no crash, no timeout | Isartor suite (link in `fixtures/README.md`) |

## Environment variables

| Variable | Meaning | Default |
|---|---|---|
| `MY_PDF_SEARCH_EXE` | executable under test | `mypdfsearch` in the `PATH`, otherwise `examples/c/console/mypdfsearch` |
| `MY_PDF_PATH` | folder of the test PDFs | `tests/fixtures` |
| `MY_PDF_TEST_CF` | encrypted PDFs | `tests/generated/crypt` |
| `MY_PDF_TEST_LATEX` | PDFs built from the `.tex` files | `tests/generated/latex` |
| `MY_PDF_REPORT_DIR` | logs and reports | `tests/reports` |

## Generating the PDFs

    ./genera_pdf_cf.sh        # encrypted PDFs (RC4 40/128, AES 128/256), 10 files in generated/crypt
    latex/build_pdfs.sh       # multilingual PDFs (Arabic, Hebrew, Hindi, Thai, CJK, Tamil, ...)

See `fixtures/README.md` for the third-party files (Isartor, PDF 2.0 examples, Japanese PDFs)
that are not distributed with the project.
