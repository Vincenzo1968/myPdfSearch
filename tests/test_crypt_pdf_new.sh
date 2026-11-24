#!/usr/bin/env bash
# Tests the encrypted PDFs and the decryption done by mypdfsearch.
#
#   1-4  qpdf/pdftotext checks on the test files: valid passwords, wrong passwords rejected,
#        extracted text identical to the unencrypted reference.
#   5    mypdfsearch itself: it must open every file with the right password, extract the same
#        text as the reference, and refuse a wrong password.
#
# The encrypted PDFs are generated (if they do not exist yet) by genera_pdf_cf.sh in $MY_PDF_TEST_CF
# (see common_paths.sh). Requires qpdf, pdftotext (poppler-utils) and python3.
#
# mypdfsearch asks for the password only on an interactive terminal, so step 5 runs it
# inside a pseudo terminal and types the password at the prompt.
set -euo pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)" || exit 1
source "$SCRIPT_DIR/common_paths.sh"

PDF_BASE="$MY_PDF_TEST_CF"
BASE_PDF="$PDF_BASE/base_non_cifrato.pdf"

for cmd in qpdf pdftotext diff python3; do
    command -v "$cmd" >/dev/null 2>&1 || { my_pdf_skip "command not found: $cmd"; exit $?; }
done

# Generate the encrypted files if the folder is empty.
if ! compgen -G "$PDF_BASE/*/*_both.pdf" >/dev/null; then
    echo "Encrypted PDFs not found in $PDF_BASE: generating them..."
    OUT_ROOT="$PDF_BASE" "$SCRIPT_DIR/genera_pdf_cf.sh" >/dev/null
fi
[[ -f "$BASE_PDF" ]] || { echo "ERROR: reference file not found: $BASE_PDF" >&2; exit 1; }

shopt -s nullglob
both_files=("$PDF_BASE"/*/*_both.pdf)
owner_files=("$PDF_BASE"/*/*_owner.pdf)
encrypted_files=("$PDF_BASE"/*/*.pdf)
shopt -u nullglob

if [[ ${#both_files[@]} -ne 5 || ${#owner_files[@]} -ne 5 || ${#encrypted_files[@]} -ne 10 ]]; then
    echo 'ERROR: expected 5 *_both.pdf files, 5 *_owner.pdf files and 10 encrypted PDFs.' >&2
    echo "Found: ${#both_files[@]} *_both.pdf, ${#owner_files[@]} *_owner.pdf and ${#encrypted_files[@]} PDFs." >&2
    exit 1
fi

tmpdir=$(mktemp -d)
trap 'rm -rf -- "$tmpdir"' EXIT
BASE_TXT="$tmpdir/base.txt"
pdftotext "$BASE_PDF" "$BASE_TXT" || { echo 'ERROR: text extraction of the reference PDF failed.' >&2; exit 1; }

printf '\n===== 1. Valid passwords: *_both.pdf files =====\n'
for f in "${both_files[@]}"; do
    for pw in user123 owner123; do
        if qpdf --password="$pw" --check "$f" >/dev/null 2>&1; then
            printf 'OK       %s [password %s]\n' "$f" "$pw"
        else
            printf 'FAILED   %s [password %s]\n' "$f" "$pw" >&2; exit 1
        fi
    done
done

printf '\n===== 2. Valid passwords: *_owner.pdf files =====\n'
for f in "${owner_files[@]}"; do
    for pw in '' owner123; do
        if qpdf --password="$pw" --check "$f" >/dev/null 2>&1; then
            [[ -n "$pw" ]] && label="owner password $pw" || label='empty user password'
            printf 'OK       %s [%s]\n' "$f" "$label"
        else
            printf 'FAILED   %s [password %s]\n' "$f" "${pw:-empty user password}" >&2; exit 1
        fi
    done
done

printf '\n===== 3. Wrong passwords rejected =====\n'
for f in "${encrypted_files[@]}"; do
    if qpdf --password=wrong --check "$f" >/dev/null 2>&1; then
        printf 'ERROR: wrong password accepted: %s\n' "$f" >&2; exit 1
    else
        printf 'OK: wrong password rejected: %s\n' "$f"
    fi
done

text_tests=0
check_text() {
    local input_pdf="$1" password="$2" label="$3"
    local output_pdf="$tmpdir/${label}.pdf" output_txt="$tmpdir/${label}.txt"
    qpdf --password="$password" --decrypt "$input_pdf" "$output_pdf" || {
        echo "ERROR: qpdf could not decrypt: $input_pdf [$label]" >&2; return 1;
    }
    pdftotext "$output_pdf" "$output_txt" || {
        echo "ERROR: pdftotext could not process: $input_pdf [$label]" >&2; return 1;
    }
    if diff -q "$BASE_TXT" "$output_txt" >/dev/null; then
        printf 'OK: text identical to the reference: %s [%s]\n' "$input_pdf" "$label"
        text_tests=$((text_tests + 1))
    else
        printf 'ERROR: text differs: %s [%s]\n' "$input_pdf" "$label" >&2
        diff -u "$BASE_TXT" "$output_txt" || true
        return 1
    fi
}

printf '\n===== 4. Text comparison for all the valid passwords =====\n'
for f in "${both_files[@]}"; do
    base_name="$(basename "$(dirname "$f")")_$(basename "$f" .pdf)"
    check_text "$f" user123 "${base_name}_user"
    check_text "$f" owner123 "${base_name}_owner"
done
for f in "${owner_files[@]}"; do
    base_name="$(basename "$(dirname "$f")")_$(basename "$f" .pdf)"
    check_text "$f" '' "${base_name}_user_empty"
    check_text "$f" owner123 "${base_name}_owner"
done

# ---------------------------------------------------------------------------------------------
printf '\n===== 5. mypdfsearch: decryption and text extraction =====\n'
my_pdf_require_exe || exit 1
EXE="$(my_pdf_resolve_exe "$MY_PDF_SEARCH_EXE")"

set +e
EXE="$EXE" BASE_TXT="$BASE_TXT" python3 - "${both_files[@]}" -- "${owner_files[@]}" <<'PYEOF'
import os
import pty
import select
import signal
import sys
import time

exe = os.environ["EXE"]
base_words = open(os.environ["BASE_TXT"], encoding="utf-8").read().split()
sep = sys.argv.index("--")
both, owner = sys.argv[1:sep], sys.argv[sep + 1:]
PROMPT = b"(empty line to cancel): "


def run(pdf, passwords, timeout=60):
    """Runs mypdfsearch in a pseudo terminal; types the next password at every prompt
    (an empty line when the list is over, which cancels the request).
    Returns (exit_code, extracted_text, whole_output)."""
    pid, fd = pty.fork()
    if pid == 0:
        os.execv(exe, [exe, "--extracttextfrom=" + pdf])
    out = b""
    seen = 0
    queue = list(passwords)
    deadline = time.time() + timeout
    status = None
    while time.time() < deadline:
        ready, _, _ = select.select([fd], [], [], 0.2)
        if ready:
            try:
                chunk = os.read(fd, 65536)
            except OSError:
                break
            if not chunk:
                break
            out += chunk
            while out.count(PROMPT) > seen:
                seen += 1
                # The program flushes the terminal input (tcsetattr with TCSAFLUSH) right after
                # printing the prompt: a human is slower than that, a script must wait a moment.
                time.sleep(0.3)
                os.write(fd, ((queue.pop(0) if queue else "") + "\n").encode())
        else:
            done, status = os.waitpid(pid, os.WNOHANG)
            if done:
                break
    else:
        os.kill(pid, signal.SIGKILL)
        os.waitpid(pid, 0)
        return -1, "", "TIMEOUT"
    if status is None:
        _, status = os.waitpid(pid, 0)
    text = out.decode("utf-8", "replace").replace("\r", "")
    marker = "TEXT EXTRACTED FROM PAGE 1:"
    body = text.split(marker)[-1] if marker in text else ""
    # Drop the lines of the password dialogue and the warning: what is left is the extracted text.
    kept = [line for line in body.split("\n")
            if not line.startswith(("Enter the user password for ", "Invalid password", "File '"))]
    return os.waitstatus_to_exitcode(status), "\n".join(kept), text


failures = 0


def report(ok, what, pdf, extra=""):
    global failures
    if not ok:
        failures += 1
    print("%s: %s [%s]%s" % ("OK" if ok else "FAILED", what, pdf, extra))


for pdf in both:
    for pw in ("user123", "owner123"):
        rc, text, _ = run(pdf, [pw])
        report(rc == 0 and text.split() == base_words,
               "mypdfsearch extracts the text (password %s)" % pw, pdf, "" if rc == 0 else " exit=%d" % rc)
    # wrong password, then the request is cancelled: no text, no crash
    rc, text, whole = run(pdf, ["wrong"])
    report(rc == 0 and text.strip() == "" and "Password request cancelled" in whole,
           "mypdfsearch refuses a wrong password", pdf,
           "" if rc == 0 else " exit=%d" % rc)

for pdf in owner:
    rc, text, whole = run(pdf, [])    # empty user password: no prompt expected
    report(rc == 0 and text.split() == base_words and "Enter the user password" not in whole,
           "mypdfsearch opens the file without asking for a password", pdf, "" if rc == 0 else " exit=%d" % rc)
    rc, text, _ = run(pdf, ["owner123"])
    report(rc == 0 and text.split() == base_words,
           "mypdfsearch extracts the text (password owner123 never needed)", pdf, "" if rc == 0 else " exit=%d" % rc)

sys.exit(1 if failures else 0)
PYEOF
rc_prog=$?
set -e
[[ $rc_prog -eq 0 ]] || { echo "ERROR: mypdfsearch decryption checks failed." >&2; exit 1; }

printf '\n===============================================================\n'
printf 'ALL TESTS PASSED\n'
printf 'PDFs with valid passwords checked (qpdf): 20 combinations\n'
printf 'Wrong passwords rejected (qpdf): %s PDFs\n' "${#encrypted_files[@]}"
printf 'Text comparisons passed (qpdf): %s\n' "$text_tests"
printf 'mypdfsearch decryption checks passed: 25\n'
printf '===============================================================\n'
