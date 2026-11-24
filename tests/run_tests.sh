#!/usr/bin/env bash
# Runs all the test suites and prints a summary.
#
# Usage: ./run_tests.sh [--quick] [suite ...]
#   --quick   skips the slow suites (partial search on whole folders, Isartor)
#   suite     names of the scripts to run (default: all)
#
# Every script follows the Automake convention: exit status 0 = passed, 77 = skipped
# (a prerequisite or a fixture is missing), anything else = failed.
# The exit status of run_tests.sh is 1 if at least one suite failed, otherwise 0.
set -u
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)" || exit 1
source "$SCRIPT_DIR/common_paths.sh"

quick=0
suites=()
for a in "$@"; do
    case "$a" in
        --quick) quick=1 ;;
        -h|--help) sed -n '2,10p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) suites+=("$a") ;;
    esac
done
if ((${#suites[@]} == 0)); then
    suites=(test_pdf_sintetici.sh test_crypt_pdf_new.sh test_actualtext_multilingua.sh
            test_cmap_unicode.sh test_regressioni_cjk.sh test_ricerca_parziale.sh test_isartor.sh)
    ((quick)) && suites=(test_pdf_sintetici.sh test_crypt_pdf_new.sh test_actualtext_multilingua.sh
                         test_cmap_unicode.sh test_regressioni_cjk.sh)
fi

my_pdf_require_exe || exit 1

# Build the multilingual PDFs if they are missing and LaTeX is available (failures are not fatal:
# the suites that need them are simply skipped).
if ! compgen -G "$MY_PDF_TEST_LATEX/*.pdf" >/dev/null && command -v xelatex >/dev/null 2>&1 \
   && command -v lualatex >/dev/null 2>&1; then
    echo "Building the multilingual PDFs from latex/*.tex ..."
    "$SCRIPT_DIR/latex/build_pdfs.sh" || echo "(some PDFs could not be built: the related tests may be skipped)"
fi
echo "mypdfsearch: $MY_PDF_SEARCH_EXE"
echo

passed=0; failed=0; skipped=0
declare -a summary=()
for s in "${suites[@]}"; do
    printf '################ %s ################\n' "$s"
    "$SCRIPT_DIR/$s"
    rc=$?
    case $rc in
        0)  passed=$((passed + 1));   summary+=("PASS  $s") ;;
        77) skipped=$((skipped + 1)); summary+=("SKIP  $s") ;;
        *)  failed=$((failed + 1));   summary+=("FAIL  $s (exit $rc)") ;;
    esac
    echo
done

echo '=================== SUMMARY ==================='
printf '%s\n' "${summary[@]}"
printf 'passed: %d   skipped: %d   failed: %d\n' "$passed" "$skipped" "$failed"
((failed == 0))
