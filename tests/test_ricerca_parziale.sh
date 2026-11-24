#!/usr/bin/env bash
# Tests the partial search (--prefix, --suffix, --contains) and the stability of directory scans.
#
#   1. Consistency: for every word v that satisfies the predicate (prefix/suffix/contained), the pages
#      reported by the partial search must be EXACTLY those reported by the whole-word search for v.
#      (candidates = words of the text extracted with -e + words reported by the partial search)
#   2. Mutually exclusive options, and use without --words / with --extracttextfrom: must be errors.
#   3. Scan of the whole folder with --path: the program must not die with a signal
#      (regression test for the use-after-free between one file and the next, exit code >= 128).
#
# Usage:  test_ricerca_parziale.sh [pdf_folder] [number_of_pdfs_for_the_consistency_check]
#   The default folder is $MY_PDF_PATH (see common_paths.sh), scanned recursively; if it has no PDF,
#   $MY_PDF_TEST_LATEX (PDFs built by latex/build_pdfs.sh) is used.
#   EXE           executable (default: $MY_PDF_SEARCH_EXE)
#   PATTERNS      words searched (default: "de ar ti")
#   TIMEOUT_SECS  timeout for every run (default 300)
# Exit status: 0 = passed, 1 = failed, 77 = skipped (no PDF files).

set -u
export LC_ALL=C.UTF-8
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)" || exit 1
source "$SCRIPT_DIR/common_paths.sh"

EXE="${EXE:-$MY_PDF_SEARCH_EXE}"
PDF_DIR="${1:-$MY_PDF_PATH}"
# Without arguments: use the PDFs in $MY_PDF_PATH; if there are none, fall back to the
# multilingual PDFs built by latex/build_pdfs.sh.
if [[ $# -eq 0 ]] && ! find "$PDF_DIR" -iname '*.pdf' -print -quit 2>/dev/null | grep -q .; then
    PDF_DIR="$MY_PDF_TEST_LATEX"
fi
NPDF="${2:-6}"
PATTERNS="${PATTERNS:-de ar ti}"
TIMEOUT_SECS="${TIMEOUT_SECS:-300}"

command -v "$EXE" >/dev/null 2>&1 || { echo "ERROR: executable not found: $EXE" >&2; exit 1; }
EXE="$(my_pdf_resolve_exe "$EXE")"
command -v python3 >/dev/null || { my_pdf_skip "python3 not found"; exit $?; }
[[ -d "$PDF_DIR" ]] || { my_pdf_skip "PDF folder not found: '$PDF_DIR'"; exit $?; }

PDF_DIR="$(my_pdf_absolute_path "$PDF_DIR")"
my_pdf_prepare_report_dir || exit 1
cd -- "$MY_PDF_REPORT_DIR" || exit 1    # the parser log is created in the current directory

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

fail=0
ok()   { printf '[OK]   %s\n' "$1"; }
ko()   { printf '[FAIL] %s\n' "$1"; fail=$((fail + 1)); }

# ---------------------------------------------------------------------------------------------
echo "=== 1. Partial search / whole-word search consistency ($NPDF PDFs, words: $PATTERNS) ==="

# NPDF files are used, taken at regular steps to get some variety (encrypted and Isartor files are excluded).
mapfile -d '' ALL < <(find "$PDF_DIR" -iname '*.pdf' -not -ipath '*isartor*' -not -path '*mypdf_baseline*' -not -path '*crypt*' -print0 | sort -z)
(( ${#ALL[@]} > 0 )) || { my_pdf_skip "no PDF found in $PDF_DIR"; exit $?; }

step=$(( ${#ALL[@]} / NPDF )); (( step < 1 )) && step=1
n=0
for ((i = 0; i < ${#ALL[@]} && n < NPDF; i += step)); do
    f="${ALL[$i]}"
    n=$((n + 1))
    d="$TMP/d$n"; mkdir -p "$d"; cp -- "$f" "$d/f.pdf"

    out="$(EXE="$EXE" PATTERNS="$PATTERNS" TIMEOUT_SECS="$TIMEOUT_SECS" python3 - "$d/f.pdf" "$d" <<'PYEOF'
import os, re, subprocess, sys, unicodedata
exe, pats, to = os.environ["EXE"], os.environ["PATTERNS"].split(), int(os.environ["TIMEOUT_SECS"])
pdf, d = sys.argv[1], sys.argv[2]

def run(args, stdin="user123\n" * 50):
    return subprocess.run([exe] + args, input=stdin, capture_output=True, text=True, timeout=to).stdout

def keys(out):
    r = {}
    for m in re.finditer(r"Key '([^']*)' found on page (\d+)", out):
        r.setdefault(m.group(1), set()).add(int(m.group(2)))
    return r

text = run(["--extracttextfrom=" + pdf])
V = set(w.lower() for w in re.findall(r"\w+", unicodedata.normalize("NFC", text)))
preds = {"--prefix": str.startswith, "--suffix": str.endswith, "--contains": lambda w, p: p in w}
bad_total = 0
for mode, fn in preds.items():
    part = keys(run(["--path=" + d, "--words=" + " ".join(pats), mode]))
    # Words with '\' or '_' are split by the tokenizer of the QUERY but not by that of the text: excluded.
    cand = sorted(w for w in (V | set(part)) if any(fn(w, p) for p in pats) and not re.search(r"[\\_]", w))
    exact = {}
    for i in range(0, len(cand), 15):
        exact.update(keys(run(["--path=" + d, "--words=" + " ".join(cand[i:i + 15])])))
    bad = [w for w in cand if exact.get(w, set()) != part.get(w, set())]
    bad_total += len(bad)
    print("%s candidates=%d reported=%d inconsistent=%d %s" % (mode, len(cand), len(part), len(bad), bad[:5]))
print("TOTAL_BAD=%d" % bad_total)
PYEOF
)"
    bad=$(sed -n 's/^TOTAL_BAD=//p' <<< "$out")
    if [[ "$bad" == "0" ]]; then ok "$(basename "$f")"; else ko "$(basename "$f"): inconsistencies"; echo "$out" | sed 's/^/        /'; fi
done

# ---------------------------------------------------------------------------------------------
echo
echo "=== 2. Invalid options ==="
one="$TMP/d1/f.pdf"
check_err() {   # descrizione, comando...
    local desc="$1"; shift
    if out="$("$@" 2>&1 </dev/null)"; [[ "$out" == *"Error"* ]]; then ok "$desc"; else ko "$desc (no error)"; fi
}
check_err "--prefix and --suffix together"            "$EXE" --path="$TMP/d1" --words=x --prefix --suffix
check_err "--prefix without --words"                 "$EXE" --path="$TMP/d1" --prefix
check_err "--contains with --extracttextfrom"       "$EXE" --extracttextfrom="$one" --contains

# ---------------------------------------------------------------------------------------------
echo
echo "=== 3. Scan of the whole folder (no crash) ==="
for mode in "" --prefix --contains; do
    yes user123 | timeout "$TIMEOUT_SECS" "$EXE" --path="$PDF_DIR" --words="ti" $mode >/dev/null 2>&1
    rc=${PIPESTATUS[1]}
    if (( rc >= 128 )); then ko "--path ${mode:-<whole word>}: killed by a signal (exit $rc)"; else ok "--path ${mode:-<whole word>}: exit $rc"; fi
done

echo
if (( fail == 0 )); then echo "ALL TESTS PASSED"; exit 0; else echo "TESTS FAILED: $fail"; exit 1; fi
