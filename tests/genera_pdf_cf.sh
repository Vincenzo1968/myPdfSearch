#!/usr/bin/env bash
set -euo pipefail

# Generates 10 encrypted test PDFs (5 security profiles x 2 password combinations):
#
#   <profile>_both.pdf    user password "user123", owner password "owner123"
#   <profile>_owner.pdf   empty user password,     owner password "owner123"
#
# Requires qpdf and python3. The base PDF (base_non_cifrato.pdf) must NOT be encrypted.
# It is looked for in $OUT_ROOT and in the directory of this script; if it does not exist,
# a minimal one-page PDF containing the text "Hello world!" is created in $OUT_ROOT.
#
# Environment:
#   OUT_ROOT   output directory (default: <directory of this script>/pdf_tests)
#
# The encryption uses random values (salts, file ID), so every run produces different files that behave
# the same way: tests must compare the extracted text, never the bytes of the PDFs.

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
OUT_ROOT="${OUT_ROOT:-$ROOT_DIR/pdf_tests}"

for cmd in qpdf python3; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "ERROR: command not found: $cmd" >&2
        exit 1
    fi
done

if [[ -f "$OUT_ROOT/base_non_cifrato.pdf" ]]; then
    BASE_PDF="$OUT_ROOT/base_non_cifrato.pdf"
elif [[ -f "$ROOT_DIR/base_non_cifrato.pdf" ]]; then
    mkdir -p "$OUT_ROOT"
    cp "$ROOT_DIR/base_non_cifrato.pdf" "$OUT_ROOT/base_non_cifrato.pdf"
    BASE_PDF="$OUT_ROOT/base_non_cifrato.pdf"
else
    mkdir -p "$OUT_ROOT"
    BASE_PDF="$OUT_ROOT/base_non_cifrato.pdf"
    python3 - "$BASE_PDF" <<'PYBASE'
import sys

content = b"BT /F1 24 Tf 72 700 Td (Hello world!) Tj ET"
objs = [
    b"<< /Type /Catalog /Pages 2 0 R >>",
    b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
    b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Contents 4 0 R"
    b" /Resources << /Font << /F1 5 0 R >> >> >>",
    b"<< /Length %d >>\nstream\n" % len(content) + content + b"\nendstream",
    b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
]
out = b"%PDF-1.4\n"
offsets = []
for i, o in enumerate(objs, 1):
    offsets.append(len(out))
    out += b"%d 0 obj\n" % i + o + b"\nendobj\n"
xref = len(out)
out += b"xref\n0 %d\n0000000000 65535 f \n" % (len(objs) + 1)
for o in offsets:
    out += b"%010d 00000 n \n" % o
out += b"trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%d\n%%%%EOF\n" % (len(objs) + 1, xref)
open(sys.argv[1], "wb").write(out)
PYBASE
    echo "Created the base PDF: $BASE_PDF"
fi

# Recent qpdf versions refuse to create RC4 files unless --allow-weak-crypto is given;
# older versions do not know the option. It is used only if qpdf supports it.
WEAK_CRYPTO_OPT=()
if qpdf --help=all 2>/dev/null | grep -q -- '--allow-weak-crypto'; then
    WEAK_CRYPTO_OPT=(--allow-weak-crypto)
fi

# profile: rc4_40 | rc4_128 | aes_128 | aes_256_r5 | aes_256_r6
make_pair() {
    local folder="$1"
    local stem="$2"
    local profile="$3"
    local bits
    local -a qpdf_global_opts=()
    local -a encrypt_opts=()
    local outdir="$OUT_ROOT/$folder"

    case "$profile" in
        rc4_40)
            bits=40
            qpdf_global_opts=(${WEAK_CRYPTO_OPT[@]+"${WEAK_CRYPTO_OPT[@]}"})
            ;;
        rc4_128)
            bits=128
            qpdf_global_opts=(${WEAK_CRYPTO_OPT[@]+"${WEAK_CRYPTO_OPT[@]}"})
            encrypt_opts+=(--use-aes=n)
            ;;
        aes_128)
            bits=128
            encrypt_opts+=(--use-aes=y)
            ;;
        aes_256_r5)
            bits=256
            encrypt_opts+=(--force-R5)
            ;;
        aes_256_r6)
            bits=256
            ;;
        *)
            echo "INTERNAL ERROR: unknown profile: $profile" >&2
            return 1
            ;;
    esac

    mkdir -p "$outdir"
    # Remove stale files so that they cannot falsify the file count of the verification script
    # (test_crypt_pdf_new.sh).
    find "$outdir" -maxdepth 1 -type f -name '*.pdf' -delete

    echo "Creating $folder/$stem (profile $profile)..."
    # ${arr[@]+"${arr[@]}"}: expands to nothing for an empty array even with 'set -u' on bash < 4.4.
    qpdf --object-streams=disable ${qpdf_global_opts[@]+"${qpdf_global_opts[@]}"} \
        --encrypt user123 owner123 "$bits" ${encrypt_opts[@]+"${encrypt_opts[@]}"} -- \
        "$BASE_PDF" "$outdir/${stem}_both.pdf"

    qpdf --object-streams=disable ${qpdf_global_opts[@]+"${qpdf_global_opts[@]}"} \
        --encrypt '' owner123 "$bits" ${encrypt_opts[@]+"${encrypt_opts[@]}"} -- \
        "$BASE_PDF" "$outdir/${stem}_owner.pdf"
}

# The same five profiles used in the original suite.
# In the V=1 and V=2 profiles /CF is not meaningful: afterwards we add a test /CF entry, not used to encrypt
# strings or streams, only to exercise the parser of the nested dictionary.
make_pair "1_pdf1.1_rc4_40"       "rc4_40"       "rc4_40"
make_pair "2_pdf1.4_rc4_128"      "rc4_128"      "rc4_128"
make_pair "3_pdf1.6_aes_128"      "aes_128"      "aes_128"
make_pair "4_pdf1.7_aes_256_v5"   "aes_256_v5"   "aes_256_r5"
make_pair "5_pdf2.0_aes_256_r6"   "aes_256_r6"   "aes_256_r6"

# qpdf creates an actual /CF for V=4 and V=5. For the two older RC4 formats we add an unused /CF
# with an incremental update. Readers ignore it because /CF is meaningful only with V=4 or V=5.
python3 - \
    5  "$OUT_ROOT/1_pdf1.1_rc4_40/rc4_40_both.pdf" \
    5  "$OUT_ROOT/1_pdf1.1_rc4_40/rc4_40_owner.pdf" \
    16 "$OUT_ROOT/2_pdf1.4_rc4_128/rc4_128_both.pdf" \
    16 "$OUT_ROOT/2_pdf1.4_rc4_128/rc4_128_owner.pdf" <<'PY'
from pathlib import Path
import re
import sys


def matching_dict_end(data: bytes, start: int) -> int:
    """Return the index of the first '>' in the matching top-level '>>'."""
    if data[start:start + 2] != b"<<":
        raise ValueError("the PDF dictionary does not start with '<<'")
    depth = 0
    literal_depth = 0
    escaped = False
    in_hex = False
    in_comment = False
    i = start
    while i < len(data):
        c = data[i]
        nxt = data[i + 1] if i + 1 < len(data) else 0

        if in_comment:
            if c in (10, 13):
                in_comment = False
            i += 1
            continue

        if literal_depth:
            if escaped:
                escaped = False
            elif c == 92:  # backslash
                escaped = True
            elif c == 40:  # nested '('
                literal_depth += 1
            elif c == 41:  # ')'
                literal_depth -= 1
            i += 1
            continue

        if in_hex:
            if c == 62:  # '>'
                in_hex = False
            i += 1
            continue

        if c == 37:  # '%': comment
            in_comment = True
            i += 1
        elif c == 40:  # literal string
            literal_depth = 1
            i += 1
        elif c == 60 and nxt == 60:  # '<<'
            depth += 1
            i += 2
        elif c == 60:  # hex string '<...>'
            in_hex = True
            i += 1
        elif c == 62 and nxt == 62:  # '>>'
            depth -= 1
            if depth == 0:
                return i
            if depth < 0:
                raise ValueError("invalid dictionary end")
            i += 2
        else:
            i += 1

    raise ValueError("unterminated PDF dictionary")


def patch_cf(path: Path, cf_length: int) -> None:
    data = path.read_bytes()
    m = re.search(rb"startxref\s+(\d+)\s+%%EOF\s*$", data)
    if not m:
        raise ValueError(f"{path}: final startxref not found")
    old_xref = int(m.group(1))
    if data[old_xref:old_xref + 4] != b"xref":
        raise ValueError(
            f"{path}: qpdf did not produce a classic xref table; "
            "the incremental update is not applied"
        )

    trailer_kw = re.search(rb"\btrailer\b", data[old_xref:])
    if not trailer_kw:
        raise ValueError(f"{path}: trailer not found after xref")
    trailer_start = old_xref + trailer_kw.end()
    while trailer_start < len(data) and data[trailer_start] in b" \t\r\n":
        trailer_start += 1
    trailer_end = matching_dict_end(data, trailer_start)
    trailer = data[trailer_start:trailer_end + 2]

    em = re.search(rb"/Encrypt\s+(\d+)\s+(\d+)\s+R\b", trailer)
    if not em:
        raise ValueError(f"{path}: the trailer has no indirect /Encrypt")
    obj_num, generation = int(em.group(1)), int(em.group(2))

    obj_re = re.compile(
        rb"(?<!\d)" + str(obj_num).encode() + rb"[ \t\r\n]+" +
        str(generation).encode() + rb"[ \t\r\n]+obj\b"
    )
    matches = [match for match in obj_re.finditer(data[:old_xref])]
    if not matches:
        raise ValueError(f"{path}: /Encrypt object {obj_num} {generation} not found")
    obj_header = matches[-1]
    dict_start = data.find(b"<<", obj_header.end(), old_xref)
    if dict_start < 0:
        raise ValueError(f"{path}: dictionary of the /Encrypt object not found")
    dict_end = matching_dict_end(data, dict_start)
    enc_dict = data[dict_start:dict_end + 2]

    if re.search(rb"/CF(?![A-Za-z0-9])", enc_dict):
        print(f"/CF already present: {path}")
        return

    # /Length in the crypt filters of the Standard Security Handler is expressed in bytes:
    # 5 for the 40-bit RC4 key and 16 for the 128-bit one.
    cf_entry = (
        b"\n/CF << /TestCF << /CFM /V2 /Length " +
        str(cf_length).encode() + b" /AuthEvent /DocOpen >> >>\n"
    )
    new_enc_dict = enc_dict[:-2] + cf_entry + b">>"
    revised_object = (
        str(obj_num).encode() + b" " + str(generation).encode() +
        b" obj\n" + new_enc_dict + b"\nendobj\n"
    )

    # Copy the current trailer and link it with /Prev to the previous revision.
    # The redefined object keeps the same number and generation.
    new_trailer = re.sub(rb"/XRefStm\s+\d+\b", b"", trailer)
    prev_pat = rb"/Prev\s+\d+\b"
    if re.search(prev_pat, new_trailer):
        new_trailer = re.sub(
            prev_pat, b"/Prev " + str(old_xref).encode(), new_trailer, count=1
        )
    else:
        new_trailer = new_trailer[:-2].rstrip() + b" /Prev " + str(old_xref).encode() + b" >>"

    prefix = b"" if data.endswith((b"\n", b"\r")) else b"\n"
    object_offset = len(data) + len(prefix)
    xref_offset = object_offset + len(revised_object)
    xref_block = (
        b"xref\n" + str(obj_num).encode() + b" 1\n" +
        f"{object_offset:010d} {generation:05d} n \n".encode() +
        b"trailer\n" + new_trailer + b"\nstartxref\n" +
        str(xref_offset).encode() + b"\n%%EOF\n"
    )
    path.write_bytes(data + prefix + revised_object + xref_block)
    print(f"Added test /CF (Length={cf_length}): {path}")


if len(sys.argv) != 9:
    raise SystemExit("internal usage: four Length/path pairs expected")

for i in range(1, len(sys.argv), 2):
    patch_cf(Path(sys.argv[i + 1]), int(sys.argv[i]))
PY

# Verifies the ten files: known passwords and presence of the /CF entry in the /Encrypt object.
# This check does not replace test_crypt_pdf_new.sh, which also compares the extracted text.
shopt -s nullglob
files=("$OUT_ROOT"/*/*_both.pdf "$OUT_ROOT"/*/*_owner.pdf)
if [[ ${#files[@]} -ne 10 ]]; then
    echo "ERROR: 10 PDFs expected, found ${#files[@]}." >&2
    exit 1
fi

for pdf in "${files[@]}"; do
    if [[ "$pdf" == *_both.pdf ]]; then
        password="user123"
    else
        password=""
    fi
    if ! qpdf --password="$password" --check "$pdf" >/dev/null 2>&1; then
        echo "ERROR: qpdf --check failed: $pdf" >&2
        echo "Run qpdf --password='$password' --check '$pdf' for the details." >&2
        exit 1
    fi

    # The password is needed here too: without it qpdf cannot open the *_both.pdf files.
    trailer="$(qpdf --password="$password" --show-object=trailer "$pdf" 2>/dev/null)" || {
        echo "ERROR: cannot read the trailer of $pdf" >&2
        exit 1
    }
    enc_ref="$(printf '%s\n' "$trailer" | python3 -c '
import re, sys
s=sys.stdin.read()
m=re.search(r"/Encrypt\s+(\d+)\s+(\d+)\s+R", s)
if m: print(m.group(1)+","+m.group(2))
')"
    if [[ -z "$enc_ref" ]]; then
        echo "ERROR: /Encrypt not found in the trailer of $pdf" >&2
        exit 1
    fi
    enc_obj="$(qpdf --password="$password" --show-object="$enc_ref" "$pdf" 2>/dev/null)" || {
        echo "ERROR: cannot read the /Encrypt object of $pdf" >&2
        exit 1
    }
    if ! grep -Eq '/CF([^[:alnum:]_]|$)' <<< "$enc_obj"; then
        echo "ERROR: /CF missing in the /Encrypt object of $pdf" >&2
        exit 1
    fi
    printf 'OK: /CF present and qpdf --check passed: %s\n' "$pdf"
done

printf '\nCreated and verified 10 encrypted PDFs with a /CF entry in the /Encrypt dictionary.\n'
printf 'You can now run the verification script: ./test_crypt_pdf_new.sh\n'
