#!/usr/bin/env bash
# Extracts the first page of all the PDFs of the Isartor test suite (PDF/A-1b), with a timeout per file.
# The files named *-fail-a/-fail-b are deliberately invalid: exit code 2 is the expected result for them.
#
# The Isartor test suite is not distributed with this project. Download it from the PDF Association
#   https://www.pdfa.org/wp-content/until2016_uploads/2011/08/isartor-pdfa-2008-08-13.zip
# (see https://pdfa.org/resource/isartor-test-suite/) and unzip it in $MY_PDF_PATH, so that the folder
#   $MY_PDF_PATH/isartor-pdfa-2008-08-13/Isartor testsuite
# exists. Without it the test is skipped (exit status 77).
#
# Usage: ./test_isartor.sh [Isartor_directory]
# Variables: EXE, BASE_DIR, REPORT_FILE, PARSER_LOG, TIMEOUT_SECS, INTERACTIVE.
# Exit status: 0 = no unexpected failures, 1 = failures/timeouts/unexpected parser errors, 77 = skipped.
set -u
set -o pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)" || exit 1
source "$SCRIPT_DIR/common_paths.sh"

DEFAULT_BASE_DIR="$MY_PDF_PATH/isartor-pdfa-2008-08-13/Isartor testsuite"
EXE="${EXE:-$MY_PDF_SEARCH_EXE}"
BASE_DIR="${1:-${BASE_DIR:-$DEFAULT_BASE_DIR}}"
REPORT_FILE="${REPORT_FILE:-$MY_PDF_REPORT_DIR/report_extract.log}"
PARSER_LOG="${PARSER_LOG:-$MY_PDF_REPORT_DIR/AAA_mypdfsearch_parsing_errors.txt}"
TIMEOUT_SECS="${TIMEOUT_SECS:-30}"
INTERACTIVE="${INTERACTIVE:-0}"
BASE_DIR="$(my_pdf_absolute_path "$BASE_DIR")"
if command -v "$EXE" >/dev/null 2>&1; then
    EXE="$(command -v "$EXE")"
elif [[ "$EXE" != /* ]]; then
    EXE="$(my_pdf_absolute_path "$EXE")"
fi

[[ -x "$EXE" ]] || { printf "Error: executable '%s' not found or not executable.\n" "$EXE" >&2; exit 1; }
if [[ ! -d "$BASE_DIR" ]]; then
    my_pdf_skip "Isartor test suite not found: '$BASE_DIR' (see the instructions at the top of this script)"
    exit $?
fi
[[ "$TIMEOUT_SECS" =~ ^[1-9][0-9]*$ ]] || { echo 'TIMEOUT_SECS must be a positive integer.' >&2; exit 2; }
[[ "$INTERACTIVE" == 0 || "$INTERACTIVE" == 1 ]] || { echo 'INTERACTIVE must be 0 or 1.' >&2; exit 2; }
my_pdf_prepare_report_dir || exit 1
mkdir -p -- "$(dirname -- "$REPORT_FILE")" "$(dirname -- "$PARSER_LOG")"

if [[ -f "$PARSER_LOG" ]]; then
    mv -- "$PARSER_LOG" "${PARSER_LOG}.$(date +%Y%m%d_%H%M%S)"
fi
: > "$REPORT_FILE"
: > "$PARSER_LOG"
log() { printf '%s\n' "$*" >> "$REPORT_FILE"; }

log '================================================================'
log '      PDF EXTRACT SUITE (page 1, recursive)'
log "      Date: $(date '+%Y-%m-%d %H:%M:%S')"
log "      Directory: $BASE_DIR"
log "      Executable: $EXE"
log "      Report: $REPORT_FILE"
log "      Mode: $([[ $INTERACTIVE -eq 1 ]] && echo 'INTERACTIVE (password prompt)' || echo 'BATCH (passwords ignored)')"
log '================================================================'
log ''

count_ok=0
count_fail=0
count_timeout=0
count_skipped=0
declare -a FAILED=() TIMED_OUT=() SKIPPED=()
mapfile -d '' PDF_LIST < <(find "$BASE_DIR" -type f -iname '*.pdf' -print0 | sort -z)

for pdf in "${PDF_LIST[@]}"; do
    log '----------------------------------------------------------------'
    log "FILE: $pdf"
    log '----------------------------------------------------------------'

    if [[ "$INTERACTIVE" -eq 1 ]]; then
        OUTPUT=''
        my_pdf_exec_in_report_dir timeout --foreground "$TIMEOUT_SECS" "$EXE" \
            --extracttextfrom="$pdf" --frompage=1 --topage=1
        EXIT_CODE=$?
    else
        OUTPUT=$(my_pdf_exec_in_report_dir timeout "$TIMEOUT_SECS" "$EXE" \
            --extracttextfrom="$pdf" --frompage=1 --topage=1 </dev/null 2>&1)
        EXIT_CODE=$?
        printf '%s\n' "$OUTPUT" >> "$REPORT_FILE"
    fi
    log ''

    if [[ $EXIT_CODE -eq 124 ]]; then
        log 'RESULT: TIMEOUT'; count_timeout=$((count_timeout + 1)); TIMED_OUT+=("$pdf")
    elif [[ $EXIT_CODE -eq 2 && ( "$pdf" == *"-fail-a"* || "$pdf" == *"-fail-b"* ) ]]; then
        log 'RESULT: EXPECTED ERROR (exit 2, -fail- file)'; count_expected_exit=$((${count_expected_exit:-0} + 1))
    elif [[ $EXIT_CODE -ne 0 ]]; then
        log "RESULT: ERROR (exit $EXIT_CODE)"; count_fail=$((count_fail + 1)); FAILED+=("$pdf")
    elif [[ "$INTERACTIVE" -eq 0 ]] && grep -q 'Password request cancelled' <<< "$OUTPUT"; then
        log 'RESULT: SKIPPED (password required)'; count_skipped=$((count_skipped + 1)); SKIPPED+=("$pdf")
    else
        log 'RESULT: OK'; count_ok=$((count_ok + 1))
    fi
    log ''
done

# Analyses PARSER_LOG to attribute the errors to the files.
# The log contains paths between single quotes (e.g. ... file header '/path/to/x.pdf').
# Errors without a path (e.g. "Error ReadHeader invalid file format")
# cannot be attributed and are not counted per file.
count_expected_errors=0
count_unexpected_errors=0
declare -a EXPECTED_ERRORS=() UNEXPECTED_ERRORS=()

if [[ -f "$PARSER_LOG" ]]; then
    while IFS= read -r errfile; do
        [[ -z "$errfile" ]] && continue
        if [[ "$errfile" == *"-fail-a"* || "$errfile" == *"-fail-b"* ]]; then
            count_expected_errors=$((count_expected_errors + 1))
            EXPECTED_ERRORS+=("$errfile")
        else
            count_unexpected_errors=$((count_unexpected_errors + 1))
            UNEXPECTED_ERRORS+=("$errfile")
        fi
    done < <(grep -oE "'/[^']+\.pdf'" "$PARSER_LOG" 2>/dev/null | tr -d "'" | sort -u)
fi

WARN_COUNT=0
ERR_COUNT=0
if [[ -f "$PARSER_LOG" ]]; then
    WARN_COUNT=$(grep -c 'WARNING' "$PARSER_LOG" 2>/dev/null || true)
    ERR_COUNT=$(grep -Eic 'error' "$PARSER_LOG" 2>/dev/null || true)
fi

log '===================== FINAL SUMMARY ====================='
log "PDFs extracted successfully : $count_ok"
log "PDFs skipped (password)     : $count_skipped"
log "PDFs with errors           : $count_fail"
log "PDFs with expected error  : ${count_expected_exit:-0} (exit 2 on -fail- files)"
log "PDFs timed out            : $count_timeout"
log "Warnings in the parser log : $WARN_COUNT"
log "Error lines in the log    : $ERR_COUNT"
log "Files with expected errors: $count_expected_errors"
log "Files with unexpected errors: $count_unexpected_errors"
log ''
if ((count_skipped > 0)); then
    log 'Skipped files (password required):'
    for f in "${SKIPPED[@]}"; do log "  - $f"; done
    log ''
fi
if ((count_fail > 0)); then
    log 'Files with extraction errors:'
    for f in "${FAILED[@]}"; do log "  - $f"; done
    log ''
fi
if ((count_timeout > 0)); then
    log 'Files timed out:'
    for f in "${TIMED_OUT[@]}"; do log "  - $f"; done
    log ''
fi
if ((count_unexpected_errors > 0)); then
    log 'Files with UNEXPECTED parser errors:'
    for f in "${UNEXPECTED_ERRORS[@]}"; do log "  - $f"; done
    log ''
fi
if ((ERR_COUNT > 0)) && [[ -f "$PARSER_LOG" ]]; then
    log '---- Error lines from the parser ----'
    grep -Ei 'error' "$PARSER_LOG" | head -50 >> "$REPORT_FILE"
    log ''
fi

cat "$REPORT_FILE"
# Expected errors (-fail- files) do not make the test fail; unexpected ones do.
((count_fail == 0 && count_timeout == 0 && count_unexpected_errors == 0))
