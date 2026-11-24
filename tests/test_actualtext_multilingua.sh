#!/usr/bin/env bash

# ============================================================
# ActualText test - multilingual PDFs
#
# Extracts the text of every page of the PDFs built from the LaTeX sources in tests/latex
# (see latex/build_pdfs.sh) and checks that mypdfsearch exits without errors.
# Exit status: 0 = passed, 1 = failed, 77 = skipped (no PDF found).
# ============================================================

set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Percorsi e funzioni comuni
source "$SCRIPT_DIR/common_paths.sh"

# Folder of the PDFs to test
PDF_DIR="${1:-${PDF_DIR:-$MY_PDF_TEST_LATEX}}"

# Executable
PDF_BIN="${PDF_BIN:-$MY_PDF_SEARCH_EXE}"

# Report
REPORT="$MY_PDF_REPORT_DIR/test_actualtext_multilingua_release.log"

# ============================================================
# Preliminary checks
# ============================================================

if [[ ! -d "$PDF_DIR" ]]; then
    my_pdf_skip "folder not found: $PDF_DIR (build the PDFs with latex/build_pdfs.sh)"
    exit $?
fi

if ! command -v "$PDF_BIN" >/dev/null 2>&1; then
    echo "ERROR: executable not found: $PDF_BIN"
    exit 1
fi

mkdir -p "$MY_PDF_REPORT_DIR"

shopt -s nullglob
PDF_FILES=("$PDF_DIR"/*.pdf)

if (( ${#PDF_FILES[@]} == 0 )); then
    my_pdf_skip "no PDF found in $PDF_DIR (build them with latex/build_pdfs.sh)"
    exit $?
fi

# ============================================================
# Start
# ============================================================

{
    echo "============================================================"
    echo " ACTUALTEXT TEST - MULTILINGUAL"
    echo "============================================================"
    echo "Date:       $(date)"
    echo "Folder:     $PDF_DIR"
    echo "Executable: $PDF_BIN"
    echo "PDFs found: ${#PDF_FILES[@]}"
    echo "============================================================"
    echo
} > "$REPORT"

passed=0
failed=0
total=${#PDF_FILES[@]}

for pdf in "${PDF_FILES[@]}"; do

    filename="$(basename "$pdf")"

    echo
    echo "============================================================"
    echo "PDF: $filename"
    echo "============================================================"

    # Number of pages
    if command -v pdfinfo >/dev/null 2>&1; then
        pages=$(pdfinfo "$pdf" 2>/dev/null |
            awk '/^Pages:/ {print $2; exit}')
    else
        pages=1
    fi

    if [[ ! "$pages" =~ ^[0-9]+$ ]] || (( pages < 1 )); then
        pages=1
    fi

    echo "Pages: $pages"

    {
        echo
        echo "============================================================"
        echo "PDF: $filename"
        echo "Pages: $pages"
        echo "============================================================"
    } >> "$REPORT"

    # Text extraction
    my_pdf_exec_in_report_dir \
    "$PDF_BIN" \
    --extracttextfrom="$pdf" \
    --frompage=1 \
    --topage="$pages" 2>&1 |
    tee -a "$REPORT"

    result=${PIPESTATUS[0]}

    if (( result == 0 )); then
        echo "FILE RESULT: OK"
        echo "FILE RESULT: OK" >> "$REPORT"
        passed=$((passed + 1))
    else
        echo "FILE RESULT: ERROR (exit code $result)"
        echo "FILE RESULT: ERROR (exit code $result)" >> "$REPORT"
        failed=$((failed + 1))
    fi

done

# ============================================================
# Final summary
# ============================================================

{
    echo
    echo "============================================================"
    echo " SUMMARY OF THE MULTILINGUAL ACTUALTEXT TEST"
    echo "============================================================"
    echo "Total PDFs:             $total"
    echo "Completed without error: $passed"
    echo "With errors:             $failed"
    echo "============================================================"
    echo
} | tee -a "$REPORT"

echo
echo "Report saved in:"
echo "$REPORT"

(( failed == 0 ))
