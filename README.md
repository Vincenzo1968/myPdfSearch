# MyPdfSearch

A standalone C library and command-line utility for PDF parsing, Unicode text extraction, and direct keyword searching.

MyPdfSearch is a command-line program and shared C library (`libmypdfsearch`). It extracts text from PDF documents and searches for words directly inside PDF files—both single documents and entire directory trees—without requiring the user to export the complete text first.

## Project status

**Stable release — version 1.0.0**

MyPdfSearch is actively maintained. While the core parsing engine, memory safety, and CMap/ToUnicode extraction have been validated against the Isartor PDF/A-1b test suite and multilingual documents, support for complex layouts and non-standard font encodings continues to evolve.

MyPdfSearch is a structural text parser, not a PDF page renderer or an OCR application:
* **Layout & Reading Order:** It does not reproduce the original visual layout of a page. In complex documents (such as multi-column newspapers, mixed text and graphics, or unusual writing modes), the reading order may differ from the visual flow.
* **No OCR:** Image-only pages or scanned documents without an embedded text layer cannot be read as text.

## Features

- Extract text from a PDF document.
- Extract text from a selected page range.
- Write extracted text to standard output or to an output file.
- Search for one or more words directly in a PDF file.
- Search PDF files in a directory, including subdirectories by default.
- Optionally limit a directory search to the specified directory.
- Optionally select text according to glyph row order.
- Handle Unicode text mappings and several PDF font-encoding mechanisms; international-text support is still being tested and improved.

## Requirements

The current build uses:

- GCC or another compatible C compiler (GCC is the tested compiler).
- GNU Autotools (for global system installation via `./configure && make && sudo make install`).
- GNU Make.
- zlib.
- OpenSSL 3.x.

### Build on Ubuntu

Install the required development packages:

```sh
sudo apt update
sudo apt install build-essential zlib1g-dev libssl-dev
```

Build and install the program globally from the project directory:

```sh
./configure
make -j2
sudo make install
```

The executable is named `mypdfsearch` and can be launched from any directory.

To display the available command-line options:

```sh
mypdfsearch --help
```

MyPdfSearch has been built and tested on Ubuntu 24.04 LTS with GCC 13.3.0. Other platforms have not yet been fully validated for this release.

## Usage

### Extract text from a PDF

```sh
mypdfsearch --extracttextfrom="/path/to/document.pdf"
```

Extract only a page range:

```sh
mypdfsearch --extracttextfrom="/path/to/document.pdf" --frompage=1 --topage=5
```

Write the extracted text to a file:

```sh
mypdfsearch --extracttextfrom="/path/to/document.pdf" --outputfile="extracted.txt"
```

### Search a PDF file

```sh
mypdfsearch --path="/path/to/document.pdf" --words="word1 word2"
```

### Search a directory of PDF files

```sh
mypdfsearch --path="/path/to/pdf-directory" --words="word1 word2"
```

By default, directory searches include subdirectories. Use `--nosubdirs` to search only the specified directory:

```sh
mypdfsearch --path="/path/to/pdf-directory" --words="word1 word2" --nosubdirs
```

### Partial search (prefix, suffix, contained text)

By default a searched word must match a whole word of the text. With one of these options, each word given to `--words` is instead compared with the beginning, the end or any part of the words of the text:

| Option | Finds the words of the text that... | Searched word | Matches, for example |
| --- | --- | --- | --- |
| `--prefix` | start with the searched word | `pre` | `pre`, `prezzo`, `prefazione` |
| `--suffix` | end with the searched word | `zione` | `informazione`, `stazione` |
| `--contains` | contain the searched word | `tazi` | `stazione`, `presentazione` |

```sh
mypdfsearch --path="/path/to/pdf-directory" --words="pre inter" --prefix
```

```text
File: '/path/to/pdf-directory/book.pdf'
	Key 'prezzo' found on page 3
	Key 'interno' found on page 3
	Key 'prefazione' found on page 7
```

The three options are mutually exclusive and are meant for `--path` searches (they need `--words`). The words reported are the ones **found in the text** (lowercase), each once per page, even when several searched words match it. Searched words containing Chinese, Japanese or Korean characters are always matched as whole words, because CJK text is already indexed by single characters and character pairs.

In the library the same feature is the option `MYPDF_OPT_MATCH_MODE` (`MYPDF_MATCH_WORD`, `MYPDF_MATCH_PREFIX`, `MYPDF_MATCH_SUFFIX`, `MYPDF_MATCH_CONTAINS`), set with `mypdf_SetOptionInt()`.

### Select text by glyph row order

```sh
mypdfsearch --extracttextfrom="/path/to/document.pdf" --sortglyphsbyrow
```

Without `--sortglyphsbyrow`, text is selected according to the order in which glyphs appear in the PDF content.

### Show parsing errors and warnings on screen

Parsing problems are always written to the log file `AAA_mypdfsearch_parsing_errors.txt`, created in the current directory. By default nothing is printed on screen about them. Use `--show_errors` and/or `--show_warnings` to get, for each file, a one-line summary on the standard error stream (stderr) with the number of errors and/or warnings found:

```sh
mypdfsearch --path="/path/to/pdf-directory" --words="word1 word2" --show_errors --show_warnings
```

```text
ERRORS (2) while processing '/path/to/broken.pdf'
WARNING (1) while processing '/path/to/odd.pdf'
```

The summaries go to stderr, so they do not mix with the extracted text on stdout. The exit code is not affected by these options.

### Exit status

| Code | Meaning |
|------|---------|
| `0` | All files were processed without errors (PDF files skipped because they need a password are not counted as errors). |
| `1` | Fatal error or invalid command line. |
| `2` | At least one file could not be processed; the remaining files were processed anyway. |

For the complete and current list of options, run `mypdfsearch --help`.

## Validation and Robustness

MyPdfSearch undergoes rigorous regression testing to ensure cross-platform consistency and syntactic reliability. 

### Isartor PDF/A Test Suite Compliance
The parsing engine was stress-tested against the industry-standard **Isartor Test Suite (PDF/A-1b Compliance)**, analyzing 205 structural compliance files. In this test run, the parser completed the 205-file test set with **0 crashes (no segmentation faults)** and **0 memory leaks**. It also detected and isolated deliberate syntax violations, including:
- **File Header Corruptions:** Properly intercepted and rejected invalid `%PDF-1.x` headers.
- **Trailer & XRef Anomalies:** Safely caught corrupted cross-reference tables and malformed trailer dictionaries.
- **Stream Object Violations:** Fully compliant with ISO specifications by identifying and blocking invalid whitespace characters following the `stream` keyword token.

## Text Extraction: Multilingual Support and Comparison Tests

MyPdfSearch is being developed with particular attention to Unicode text extraction from PDF files. The implementation handles PDF character-mapping mechanisms such as CMaps and `ToUnicode` maps, and works with different font encodings and Unicode character sequences.

The following tests use multilingual PDF documents created to exercise a range of writing systems. They provide examples of the text currently extracted by MyPdfSearch and, where available, a comparison with Poppler's `pdftotext` utility.

### Individual language tests

The following examples show the first page of each test document. The commands are shown in a simplified form; run `mypdfsearch --help` for the complete list of available options.

#### Bengali

`test_bengalese.pdf`

```text
নমস্কার বিশ্ব! এটি বাংলা ভাষা পরীক্ষা।
```

In this test, MyPdfSearch produces the expected placement of the Bengali combining marks, accurately.

#### Traditional Chinese

`test_cinese_tradizionale.pdf`

```text
你好世界！這是一個中文測試。
```

#### Korean

`test_coreano.pdf`

```text
안녕하세요 세계! 이것은 한국어 테스트입니다.
```

#### Hindi

`test_hindi.pdf`

```text
नमस्ते दुनिया! यह हिंदी पाठ का एक परीक्षण है।
```

#### Kannada

`test_kannada.pdf`

```text
ನಮಸ್ಕಾರ ವಿಶ್ವ! ಇದು ಕನ್ನಡ ಭಾಷೆ ಪರೀಕ್ಷೆ.
```

#### Tamil

`test_tamil.pdf`

```text
வணக்கம் உலகம்! இது தமிழ் மொழி சோதனை.
```

In this example, MyPdfSearch reconstructs the Tamil words in the expected form.

#### Telugu

`test_telugu.pdf`

```text
నమస్కారం ప్రపంచం! ఇది తెలుగు భాష పరీక్ష.
```

### Multilingual documents

In addition to the individual language tests, two documents exercise several writing systems in a single PDF.

#### `test_multilingua_all.pdf`

This document contains examples in 16 language/script groups:

- Italian and Spanish
- Japanese
- Simplified and Traditional Chinese
- Korean
- Arabic and Hebrew
- Hindi (Devanagari)
- Bengali
- Thai
- Russian (Cyrillic)
- Modern Greek
- Amharic / Ge'ez
- Georgian
- Armenian

MyPdfSearch extracts text from both pages, including the scripts listed above. The test is useful for checking that different font encodings and writing systems can coexist within the same document. The output is not reproduced here in full.

#### `test_multilingua.pdf`

This document contains Italian, Arabic, Hebrew, Hindi, Thai, Japanese, and Simplified Chinese. MyPdfSearch extracts text from all these language sections.

These aggregate tests demonstrate the range of scripts covered by the current test material. They should not be interpreted as proof of complete support for every PDF using those writing systems: results can vary with the PDF producer, embedded fonts, character maps, and text encoding.

### Current limitations and areas for improvement

The examples above are encouraging, but they represent a limited set of test documents. Further testing is needed with PDFs produced by different applications, using different fonts and encoding strategies.

Some areas still require attention:

- **Reading order and layout:** MyPdfSearch extracts text but does not reproduce the original page layout. Multi-column pages, mixed text and images, and complex positioning can result in an order that differs from the intended reading order.
- **Right-to-left writing:** Arabic and Hebrew extraction requires particular care. Correct character decoding does not by itself guarantee correct visual ordering or spacing.
- **Combining characters and normalization:** Unicode characters can be represented by different sequences. Combining marks and normalization forms need to be handled consistently.
- **CJK and vertical writing:** East Asian text, vertical writing modes, and less common font encodings remain important areas for additional testing.
- **OCR-generated PDFs:** MyPdfSearch does not perform OCR. It can extract text only when a usable text layer is present in the PDF; image-only pages are outside its scope.

The comparison examples should therefore be read as observations about these particular files, not as a general ranking of PDF extraction tools. Poppler remains a valuable and much broader project, and its developers and contributors have produced software that serves a wide range of PDF use cases.

The goal of MyPdfSearch is to provide an independent implementation focused on understanding PDF structures, extracting text, and searching directly within PDF files. Correctness, standards-aware parsing, and continued improvement are more important than claiming to outperform other tools.

## Feedback and bug reports

Bug reports, PDF test cases that can be shared legally, and suggestions for improvement are welcome. Please include the command used, the relevant output, and a short description of the expected result. Do not upload documents containing private or confidential information.

## License

The original MyPdfSearch source code is distributed under the **GNU General Public License, version 3 or (at your option) any later version**. See `COPYING` for the complete license text.

Some bundled character-mapping data are derived from third-party resources and are covered by separate notices. The Adobe CMap mapping tables are accompanied by the BSD 3-Clause license notice, and Unicode-derived data are accompanied by the Unicode License V3 notice. See the files in `LICENSES/` for details. These third-party notices apply to the corresponding data, not to the original MyPdfSearch source code.

## Author

Vincenzo Lo Cicero\
vincenzolocicero68@gmail.com
