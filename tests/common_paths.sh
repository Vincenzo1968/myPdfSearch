#!/usr/bin/env bash
# Paths shared by the mypdfsearch test scripts.
# Sourced by the scripts with:  source "$SCRIPT_DIR/common_paths.sh"
#
# Every variable can be set in the environment before running a script:
#
#   MY_PDF_PATH        root of the test PDFs                  (default: <tests>/fixtures)
#   MY_PDF_TEST_CF     folder of the encrypted PDFs           (default: <tests>/generated/crypt)
#   MY_PDF_TEST_LATEX  folder of the PDFs built from latex/*.tex (default: <tests>/generated/latex)
#   MY_PDF_REPORT_DIR  where logs and reports are written     (default: <tests>/reports)
#   MY_PDF_SEARCH_EXE  the mypdfsearch executable             (default: the one in the PATH,
#                      otherwise the one built in the source tree)
#
# Exit status 77 means "test skipped" (the Automake convention).

# Directory that contains this file and all the test scripts.
MY_PDF_SCRIPTS_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)" || return 1

# Top level directory of the source tree (the parent of tests/).
MY_PDF_PROJECT_DIR="$(cd -- "$MY_PDF_SCRIPTS_DIR/.." && pwd -P)"

: "${MY_PDF_PATH:=$MY_PDF_SCRIPTS_DIR/fixtures}"
: "${MY_PDF_TEST_CF:=$MY_PDF_SCRIPTS_DIR/generated/crypt}"
: "${MY_PDF_TEST_LATEX:=$MY_PDF_SCRIPTS_DIR/generated/latex}"
: "${MY_PDF_REPORT_DIR:=$MY_PDF_SCRIPTS_DIR/reports}"

# --- Executable: installed in the system or built in the source tree ---
if [[ -z "${MY_PDF_SEARCH_EXE:-}" ]]; then
    if command -v mypdfsearch >/dev/null 2>&1; then
        MY_PDF_SEARCH_EXE="$(command -v mypdfsearch)"
    else
        MY_PDF_SEARCH_EXE="$MY_PDF_PROJECT_DIR/examples/c/console/mypdfsearch"
    fi
fi

export MY_PDF_PATH MY_PDF_TEST_CF MY_PDF_TEST_LATEX MY_PDF_SEARCH_EXE
export MY_PDF_PROJECT_DIR MY_PDF_SCRIPTS_DIR MY_PDF_REPORT_DIR

MY_PDF_EXIT_SKIP=77

my_pdf_prepare_report_dir() {
    mkdir -p -- "$MY_PDF_REPORT_DIR"
}

# Runs a command inside the report directory: the parser log
# (AAA_mypdfsearch_parsing_errors.txt) is created in the current directory.
my_pdf_exec_in_report_dir() {
    my_pdf_prepare_report_dir || return 1
    ( cd -- "$MY_PDF_REPORT_DIR" && "$@" )
}

# Works with absolute paths and with commands found in the PATH.
my_pdf_require_exe() {
    if ! command -v "$MY_PDF_SEARCH_EXE" >/dev/null 2>&1; then
        printf 'ERROR: executable not found or not executable: %s\n' "$MY_PDF_SEARCH_EXE" >&2
        printf "Build the project (./configure && make) and install it, or set MY_PDF_SEARCH_EXE.\n" >&2
        return 1
    fi
}

# Makes a relative path absolute using the current directory of the caller,
# without changing the directory of the calling shell.
my_pdf_absolute_path() {
    local path="$1"
    if [[ "$path" = /* ]]; then
        printf '%s\n' "$path"
    else
        printf '%s/%s\n' "$PWD" "$path"
    fi
}

# Resolves an executable name or path to an absolute path.
my_pdf_resolve_exe() {
    local exe="$1"
    if command -v "$exe" >/dev/null 2>&1; then
        command -v "$exe"
    elif [[ "$exe" != /* ]]; then
        my_pdf_absolute_path "$exe"
    else
        printf '%s\n' "$exe"
    fi
}

# Prints a SKIP message and returns the "skipped" status.
#   my_pdf_skip "reason"; exit $?
my_pdf_skip() {
    printf 'SKIP: %s\n' "$*"
    return "$MY_PDF_EXIT_SKIP"
}
