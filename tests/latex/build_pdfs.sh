#!/usr/bin/env bash
# Builds the multilingual test PDFs from the .tex sources in this folder.
#
# Every source declares its engine in the first lines ("% !TEX program = xelatex|lualatex").
# Requirements: xelatex and lualatex (TeX Live), the polyglossia package and the Noto fonts
# (Debian/Ubuntu: texlive-xetex texlive-luatex texlive-lang-all fonts-noto fonts-noto-cjk).
#
# Usage: ./build_pdfs.sh [output_folder]
#   The default output folder is $MY_PDF_TEST_LATEX (see ../common_paths.sh).
# Exit status: 0 = all built, 1 = some failed, 77 = skipped (LaTeX tools missing).
set -u
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)" || exit 1
source "$SCRIPT_DIR/../common_paths.sh"

OUT_DIR="$(my_pdf_absolute_path "${1:-$MY_PDF_TEST_LATEX}")"

for cmd in xelatex lualatex; do
    command -v "$cmd" >/dev/null 2>&1 || { my_pdf_skip "$cmd not found"; exit $?; }
done

mkdir -p -- "$OUT_DIR" || exit 1
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

ok=0; failed=0
for tex in "$SCRIPT_DIR"/*.tex; do
    name="$(basename -- "$tex" .tex)"
    engine="$(sed -n 's/^% *!TEX program *= *\([a-z]*\).*/\1/p' "$tex" | head -n 1)"
    engine="${engine:-xelatex}"
    # Two runs for a correct pagination; the build happens in a scratch folder.
    if ( cd -- "$work" && "$engine" -interaction=nonstopmode -halt-on-error \
            -output-directory="$work" "$tex" >"$work/$name.log" 2>&1 \
         && "$engine" -interaction=nonstopmode -halt-on-error \
            -output-directory="$work" "$tex" >>"$work/$name.log" 2>&1 ) \
       && cp -- "$work/$name.pdf" "$OUT_DIR/$name.pdf"; then
        printf 'OK      %s (%s)\n' "$name.pdf" "$engine"
        ok=$((ok + 1))
    else
        printf 'FAILED  %s (%s): last lines of the log:\n' "$name" "$engine" >&2
        tail -n 8 -- "$work/$name.log" >&2
        failed=$((failed + 1))
    fi
done
printf '\n%d PDF built in %s, %d failed\n' "$ok" "$OUT_DIR" "$failed"
((failed == 0))
