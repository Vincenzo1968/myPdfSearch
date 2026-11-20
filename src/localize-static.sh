#!/bin/sh
# Rende "locali" i simboli interni della libreria statica, cosi' che nel programma che la linka
# restino visibili solo le funzioni pubbliche mypdf_*  (evita "multiple definition" con nomi
# generici come Parse, match, array...). Gli oggetti sono compilati con -fvisibility=hidden:
# tutto cio' che non e' dichiarato MYPDF_API e' "hidden" e qui diventa locale.
#
# uso: localize-static.sh <libreria.a>
# Richiede GNU binutils (ld, objcopy, ar). Se mancano, lascia la libreria com'e' (senza errore).
a=$1
[ -f "$a" ] || exit 0
for t in ld objcopy ar ranlib; do
    command -v "$t" >/dev/null 2>&1 || { echo "localize-static: '$t' non trovato: simboli interni della libreria statica lasciati visibili" >&2; exit 0; }
done
case $a in /*) ;; *) a=$(pwd)/$a ;; esac
d=$(mktemp -d) || exit 0
trap 'rm -rf "$d"' EXIT
if ld -r -o "$d/merged.o" --whole-archive "$a" 2>/dev/null &&
   objcopy --localize-hidden "$d/merged.o" 2>/dev/null &&
   ar cr "$d/new.a" "$d/merged.o" 2>/dev/null &&
   ranlib "$d/new.a" 2>/dev/null; then
    cp "$d/new.a" "$a"
else
    echo "localize-static: elaborazione non riuscita: libreria statica lasciata invariata" >&2
fi
exit 0
