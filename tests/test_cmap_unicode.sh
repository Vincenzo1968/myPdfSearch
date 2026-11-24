#!/usr/bin/env bash
# Targeted CMap/Unicode regression tests on Japanese PDFs.
#
# The PDFs (grammar_1.pdf, kk190531a.pdf, leall_en_t.pdf, JapVertical_01.pdf) are third-party files
# and are NOT distributed with this project: put them (anywhere) under $MY_PDF_PATH/Giapponesi
# or pass the folder as argument. Every check whose PDF is missing is skipped; if all of them are
# skipped the exit status is 77.
#
# Usage:
#   ./test_cmap_unicode.sh [pdf_folder]
#   ./test_cmap_unicode.sh --frompage=1 --topage=21 [pdf_folder]   # JapVertical_* limited to pages 1-21

set -u
set -o pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)" || exit 1
source "$SCRIPT_DIR/common_paths.sh"

# Options: --frompage=N --topage=M (only for the JapVertical_* files; default: all the pages)
FROMPAGE="${FROMPAGE:-}"
TOPAGE="${TOPAGE:-}"
_args=()
for a in "$@"; do
    case "$a" in
        --frompage=*) FROMPAGE="${a#*=}" ;;
        --topage=*)   TOPAGE="${a#*=}" ;;
        *)            _args+=("$a") ;;
    esac
done
set -- ${_args[@]+"${_args[@]}"}
for v in "$FROMPAGE" "$TOPAGE"; do
    if [[ -n "$v" && ! "$v" =~ ^[1-9][0-9]*$ ]]; then
        printf 'ERROR: --frompage and --topage must be positive integers.\n' >&2
        exit 2
    fi
done
PAGE_OPTS=()
set_page_opts() {
    PAGE_OPTS=()
    [[ "$(basename -- "$1")" == JapVertical_* ]] || return 0
    [[ -n "$FROMPAGE" ]] && PAGE_OPTS+=("--frompage=$FROMPAGE")
    [[ -n "$TOPAGE" ]]   && PAGE_OPTS+=("--topage=$TOPAGE")
    return 0
}

PDF_ROOT="${1:-${PDF_ROOT:-$MY_PDF_PATH/Giapponesi}}"
REPORT_FILE="${REPORT_FILE:-$MY_PDF_REPORT_DIR/report_cmap_unicode.log}"

EXE="$(my_pdf_resolve_exe "${EXE:-$MY_PDF_SEARCH_EXE}")"

PDF_ROOT="$(my_pdf_absolute_path "$PDF_ROOT")"
TIMEOUT_SECS="${TIMEOUT_SECS:-60}"


if [[ ! -d "$PDF_ROOT" ]]; then
    my_pdf_skip "PDF folder not found: $PDF_ROOT"
    exit $?
fi
if [[ ! -x "$EXE" ]]; then
    printf 'ERROR: executable not found or not executable: %s\n' "$EXE" >&2
    exit 2
fi
if ! [[ "$TIMEOUT_SECS" =~ ^[1-9][0-9]*$ ]]; then
    printf 'ERROR: TIMEOUT_SECS must be a positive integer.\n' >&2
    exit 2
fi
my_pdf_prepare_report_dir || exit 1
mkdir -p -- "$(dirname -- "$REPORT_FILE")"
: > "$REPORT_FILE" || exit 2
PASS=0
FAIL=0
SKIP=0
log() { printf '%s\n' "$*" | tee -a "$REPORT_FILE"; }
find_pdf() { find "$PDF_ROOT" -type f -iname "$1" -print -quit; }

run_extract_test() {
    local label="$1" pdf="$2" expected="$3" output rc normalized
    if [[ -z "$pdf" || ! -f "$pdf" ]]; then
        log "SKIP: $label - PDF not found"; SKIP=$((SKIP + 1)); return
    fi
    output=$(my_pdf_exec_in_report_dir timeout "$TIMEOUT_SECS" "$EXE" \
        --extracttextfrom="$pdf" --frompage=1 --topage=1 2>&1)
    rc=$?
    if ((rc != 0)); then
        log "FAIL: $label - extraction exit=$rc"
        printf '%s\n' "$output" >> "$REPORT_FILE"
        FAIL=$((FAIL + 1)); return
    fi
    normalized=$(printf '%s' "$output" | tr -d '[:space:]')
    if [[ "$normalized" == *"$expected"* ]]; then
        log "PASS: $label"; PASS=$((PASS + 1))
    else
        log "FAIL: $label - expected sequence not found: $expected"
        printf '%s\n' "$output" >> "$REPORT_FILE"
        FAIL=$((FAIL + 1))
    fi
}

run_search_test() {
    local label="$1" pdf="$2" term="$3" output rc
    if [[ -z "$pdf" || ! -f "$pdf" ]]; then
        log "SKIP: $label - PDF not found"; SKIP=$((SKIP + 1)); return
    fi
    set_page_opts "$pdf"
    output=$(my_pdf_exec_in_report_dir timeout "$TIMEOUT_SECS" "$EXE" --path="$pdf" --words="$term" ${PAGE_OPTS[@]+"${PAGE_OPTS[@]}"} 2>&1)
    rc=$?
    if ((rc != 0)); then
        log "FAIL: $label - search exit=$rc"
        printf '%s\n' "$output" >> "$REPORT_FILE"
        FAIL=$((FAIL + 1))
    elif grep -Fq "Key '$term' found" <<< "$output"; then
        log "PASS: $label"; PASS=$((PASS + 1))
    else
        log "FAIL: $label - exact occurrence not reported: $term"
        printf '%s\n' "$output" >> "$REPORT_FILE"
        FAIL=$((FAIL + 1))
    fi
}

GRAMMAR=$(find_pdf 'grammar_1.pdf')
KK=$(find_pdf 'kk190531a.pdf')
VERTICAL=$(find_pdf 'JapVertical_01.pdf')
LEALL=$(find_pdf 'leall_en_t.pdf')

log '=============================================================='
log 'TARGETED CMap / Unicode TESTS - mypdfsearch'
log "Date: $(date '+%Y-%m-%d %H:%M:%S')"
log "PDF folder: $PDF_ROOT"
log "Executable: $EXE"
log '=============================================================='
log ''

# --- ACTIVE TESTS ---

# 1. kk190531a: the character 日 is extracted correctly.
run_extract_test 'kk190531a: full date ２０１９年５月３１日' "$KK" '２０１９年５月３１日'
run_extract_test 'kk190531a: heading 日本銀行' "$KK" '日本銀行'
run_search_test  'kk190531a: search 日本銀行' "$KK" '日本銀行'

# 2. JapVertical_01: the whole CJK run is searchable (disabled).
# run_extract_test 'JapVertical_01: vertical title' "$VERTICAL" '法華経の現代語訳'
# run_search_test  'JapVertical_01: ricerca 法華経' "$VERTICAL" '法華経'

# 3. leall_en_t: real text on page 1.
run_extract_test 'leall_en_t: text present' "$LEALL" 'EasyJapanese'

# 4. grammar_1: all the particles of the index.
run_extract_test 'grammar_1: all the particles of the index' "$GRAMMAR" '3.1Particleを3.2Particleに3.3Particleへ3.4Particlesからandまで3.5Particleで3.6Particleか'

log ''
log '==================== SUMMARY ===================='
log "PASS: $PASS"
log "FAIL: $FAIL"
log "SKIP: $SKIP"
log "Total checks: $((PASS + FAIL + SKIP))"
log "Report: $REPORT_FILE"
# All the checks skipped (no fixtures): report "skipped".
if ((PASS + FAIL == 0)); then exit "$MY_PDF_EXIT_SKIP"; fi
((FAIL == 0))
