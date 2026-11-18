/*    Copyright (C) 2026 Vincenzo Lo Cicero

Author: Vincenzo Lo Cicero.
e-mail: vincenzolocicero68@gmail.com

This file is part of mypdfsearch.

mypdfsearch is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any side version.

mypdfsearch is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
MERCHANTABILITY or FITNESS FOR PARTICULAR PURPOSE. See the
GNU General Public License for details.

You should have received a copy of the GNU General Public License
along with mypdfsearch. If not, see <http://www.gnu.org/licenses/>. */

/* Funzioni di utilita' condivise tra la libreria e il programma a riga di comando:
   lista dei file da elaborare, conversione di stringhe, spezzatura delle parole da cercare. */

#ifndef MYPDF_UTILS_INCLUDED
#define MYPDF_UTILS_INCLUDED

#include <stddef.h>
#include <wchar.h>

#include "mypdf_internal.h"
#include "scanner.h"

/* Lista dei file PDF da elaborare. */
FilesList* addFileToFilesList(FilesList *first, FilesList *newFile);
void freeFilesList(FilesList *first);

#if defined(_WIN64) || defined(_WIN32)
void GetErrorString(DWORD err, char *szError, int len);
#endif

FilesList* getFilesRecursive(char *dirName, int lenOrig, int bNoSubDirs, FilesList* myFilesList);

/* Numero di processori logici. */
long GetProcessors(void);

/* Converte una stringa multibyte nella locale corrente in wchar_t (da liberare con free()). */
wchar_t* myMultibyteToWidechar(const char *psz, size_t mbslen);

/* Spezza la stringa delle parole da cercare e riempie pParams->pWordsToSearchArray. */
int mySplitWords(Params *pParams, wchar_t *pwszOption);

/* Imposta pParams->nThisMachineEndianness in base alla macchina. */
void checkEndianness(Params *pParams);

// Converte una stringa wchar_t in UTF-8 (indipendente dalla locale). Ritorna un buffer allocato con malloc
// (terminato da '\0'; liberare con free) oppure NULL se la memoria e' esaurita. *pOutLen (se non NULL) = byte scritti.
char *myWideToUtf8(const wchar_t *pwsz, size_t len, size_t *pOutLen);

#endif /* MYPDF_UTILS_INCLUDED */
