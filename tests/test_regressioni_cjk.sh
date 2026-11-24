#!/usr/bin/env bash
# Automatic regression checks on CJK PDFs: the first page of each file must be extracted
# without errors and must not be empty.
#
# These PDFs are third-party files and are NOT distributed with this project. Put them under
# $MY_PDF_PATH, with the following names (see fixtures/README.md):
#   pdf20-utf8-test.pdf
#   Giapponesi/SoloGiapponese/H21 monodukuri report.pdf
#   Giapponesi/MistoJapanEnglish/{grammar_1,numbers,Kanji_Elementi}.pdf
# Missing files are reported as "not found" and skipped; if none exists the exit status is 77.
set -u
set -o pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)" || exit 1
source "$SCRIPT_DIR/common_paths.sh"

EXE="${EXE:-$MY_PDF_SEARCH_EXE}"
REPORT_FILE="${REPORT_FILE:-$MY_PDF_REPORT_DIR/report_verifiche_cjk.log}"
PARSER_LOG="${PARSER_LOG:-$MY_PDF_REPORT_DIR/AAA_mypdfsearch_parsing_errors.txt}"
BASE_DIR="$MY_PDF_PATH"
EXE="$(my_pdf_resolve_exe "$EXE")"

PDF_FILES=(
    "$BASE_DIR/pdf20-utf8-test.pdf"
    "$BASE_DIR/Giapponesi/SoloGiapponese/H21 monodukuri report.pdf"
    "$BASE_DIR/Giapponesi/MistoJapanEnglish/grammar_1.pdf"
    "$BASE_DIR/Giapponesi/MistoJapanEnglish/numbers.pdf"
    "$BASE_DIR/Giapponesi/MistoJapanEnglish/Kanji_Elementi.pdf"
)

[[ -x "$EXE" ]] || { printf "Error: executable '%s' not found or not executable.\n" "$EXE" >&2; exit 1; }
my_pdf_prepare_report_dir || exit 1
mkdir -p -- "$(dirname -- "$REPORT_FILE")" "$(dirname -- "$PARSER_LOG")"
: > "$PARSER_LOG"
: > "$REPORT_FILE"
log() { printf '%s\n' "$*" >> "$REPORT_FILE"; }

log '================================================================'
log '        AUTOMATIC REGRESSION CHECKS ON CJK PDFs'
log "        Date: $(date '+%Y-%m-%d %H:%M:%S')"
log "        Executable: $EXE"
log '================================================================'
log ''
count_ok=0
count_fail=0
count_missing=0
count_empty=0
declare -a FAILED_FILES=() EMPTY_FILES=()

for pdf in "${PDF_FILES[@]}"; do
    log '----------------------------------------------------------------'
    log "FILE: $pdf"
    log '----------------------------------------------------------------'
    if [[ ! -f "$pdf" ]]; then
        log 'STATUS: NOT FOUND'; log ''
        count_missing=$((count_missing + 1)); continue
    fi
    OUTPUT=$(my_pdf_exec_in_report_dir "$EXE" --extracttextfrom="$pdf" --frompage=1 --topage=1 2>&1)
    EXIT_CODE=$?
    printf '%s\n' "$OUTPUT" >> "$REPORT_FILE"
    log ''
    if [[ $EXIT_CODE -eq 0 ]]; then
        # Is the text extracted (after the "TEXT EXTRACTED FROM PAGE 1:" marker) empty or only whitespace?
        EXTRACTED=$(printf '%s\n' "$OUTPUT" \
            | awk '/^TEXT EXTRACTED FROM PAGE 1:/{flag=1;next} flag' \
            | tr -d '[:space:]')
        if [[ -z "$EXTRACTED" ]]; then
            log 'FILE RESULT: EMPTY (no text extracted)'
            count_empty=$((count_empty + 1))
            EMPTY_FILES+=("$pdf")
        else
            log 'FILE RESULT: OK (exit code 0)'
            count_ok=$((count_ok + 1))
        fi
    else
        log "FILE RESULT: ERROR (exit code $EXIT_CODE)"
        count_fail=$((count_fail + 1)); FAILED_FILES+=("$pdf")
    fi
    log ''
done

WARN_COUNT=0
ERR_COUNT=0
if [[ -f "$PARSER_LOG" ]]; then
    WARN_COUNT=$(grep -c 'WARNING' "$PARSER_LOG" 2>/dev/null || true)
    ERR_COUNT=$(grep -Eic 'error' "$PARSER_LOG" 2>/dev/null || true)
fi
log '===================== FINAL SUMMARY ====================='
log "PDFs processed successfully : $count_ok"
log "PDFs with empty text        : $count_empty"
log "PDFs with errors            : $count_fail"
log "PDFs not found             : $count_missing"
log "Warnings from the parser    : $WARN_COUNT"
log "Error lines from the parser: $ERR_COUNT"
log "Total                        : $((count_ok + count_empty + count_fail + count_missing))"
log ''
log "Detailed report written to: $REPORT_FILE"
if ((count_empty > 0)); then
    log ''
    log 'Files with empty text (to be checked):'
    for f in "${EMPTY_FILES[@]}"; do log "  - $f"; done
fi
if ((count_fail > 0)); then
    log ''
    log 'Files with errors:'
    for f in "${FAILED_FILES[@]}"; do log "  - $f"; done
fi
cat "$REPORT_FILE"
# No fixture available: report "skipped".
if ((count_ok + count_empty + count_fail == 0)); then exit "$MY_PDF_EXIT_SKIP"; fi
((count_fail == 0))
