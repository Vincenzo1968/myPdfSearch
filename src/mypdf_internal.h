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

#ifndef MYPDF_INTERNAL_INCLUDED
#define MYPDF_INTERNAL_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <wctype.h>
#include <stdint.h>

#include "mypdfsearch.h"   // API pubblica (anche l'implementazione ne vede i prototipi)

//#define HT SIZE 8191
#define BLOCK_SIZE 4096
//#define BLOCK SIZE 4096 #21

#ifndef MAX_LEN_STR
	#define MAX_LEN_STR 1024
#endif

#ifndef MAX_WORDS_TO_SEARCH
	#define MAX_WORDS_TO_SEARCH 100
#endif

#define UNUSED(x) (void)(x)

// Range of CJK scripts managed with bigram tokenization.
// NOTE: Thai (U+0E00 - U+0E7F) has been removed intentionally,
// because Thai separates words with spaces and is to be managed in the branch
// "Western" (which in turn should be extended with the Thai range, see below).
// The vertical "rejoining" below uses this same macro, so
// Even that logic automatically doesn't touch Thai anymore.
#ifndef IS_ASIAN_SCRIPT
#define IS_ASIAN_SCRIPT(c) ( \
    ((c) >= 0x3040 && (c) <= 0x309F) || \
    ((c) >= 0x30A0 && (c) <= 0x30FF) || \
    ((c) >= 0x3400 && (c) <= 0x4DBF) || \
    ((c) >= 0x4E00 && (c) <= 0x9FFF) || \
    ((c) >= 0xAC00 && (c) <= 0xD7AF) || \
    ((c) >= 0x20000 && (c) <= 0x2FA1F)    \
)
#endif

typedef struct tagFilesList
{
	#if !defined(_WIN64) && !defined(_WIN32)
	char myPathName[PATH_MAX + 1];
	#endif
	char myFileName[PATH_MAX + 1];
	//char relativeName[PATH MAX + 1];
	struct tagFilesList *next;
} FilesList;


// ---------------------------------------------------------------------------
// Macro condivise, raccolte qui da parser.c e scanner.c (uso solo interno alla libreria)
// ---------------------------------------------------------------------------

// Surrogati UTF-16
#define HI_SURROGATE_START   0xD800
#define LO_SURROGATE_START   0xDC00

#define HI_SURROGATE_END     0xDBFF
#define LO_SURROGATE_END     0xDFFF

#define LEAD_OFFSET          (0xD800 - (0x10000 >> 10))
#define SURROGATE_OFFSET     (0x10000 - (0xD800 << 10) - 0xDC00)

// Intervalli CJK (usati dal parser)
#define IS_CJK(c) ( \
    ((c) >= 0x2E80 && (c) <= 0x9FFF) || \
    ((c) >= 0x3400 && (c) <= 0x4DBF) || \
    ((c) >= 0xF900 && (c) <= 0xFAFF) || \
    ((c) >= 0xFF00 && (c) <= 0xFFEF) || \
    ((c) >= 0x3000 && (c) <= 0x303F) || \
    ((c) >= 0x3040 && (c) <= 0x30FF)    \
) ? 1 : 0

// Limite massimo di oggetti indirettamente indirizzabili (PDF 32000-1, Annex C: 8.388.607). Evita che un file corrotto
// o ostile ci faccia allocare tabelle enormi.
#define MYPDF_MAX_OBJS_TABLE_SIZE 8388608u

// Macro per byte/word (usate da scanner.c)
#define MY_BYTE  uint8_t
#define MY_WORD  uint16_t
#define MY_DWORD uint32_t
#define MY_LONG  uint32_t

#define MY_MAKEWORD(a, b) ((MY_WORD)(((MY_BYTE)((MY_DWORD)(a) & 0xff)) | ((MY_WORD)((MY_BYTE)((MY_DWORD)(b) & 0xff))) << 8))

#define MY_MAKELONG(a, b) ((MY_LONG)(((MY_WORD)((MY_DWORD)(a) & 0xffff)) | ((MY_DWORD)((MY_WORD)((MY_DWORD)(b) & 0xffff))) << 16))

#define MY_LOWORD(l) ((MY_WORD)((MY_DWORD)(l) & 0xFFFF))
#define MY_HIWORD(l) ((MY_WORD)((MY_DWORD)(l) >> 16))
#define MY_LOBYTE(w) ((MY_BYTE)((MY_DWORD)(w) & 0xFF))
#define MY_HIBYTE(w) ((MY_BYTE)((MY_DWORD)(w) >> 8))

#endif // MYPDF_INTERNAL_INCLUDED


/*
MYDEBUG_PRINT_ON_ReadEncrypt_R

MYDEBUG_PRINT_SINGLE_CHAR_ENCODING
MYDEBUG_PRINT_SINGLE_CHAR_COORD

MYDEBUG_PRINT_ON_SHOW_COORDINATE_STRING
MYDEBUG_PRINT_ON_SHOW_STRING_OPERATOR
MYDEBUG_PRINT_ON_POSITIONING_STRING_OPERATOR
MYDEBUG_PRINT_ON_TEXT_STATE_OPERATOR
MYDEBUG_PRINT_ON_CTM_OPERATOR

MYPDFSEARCH_SHOW_TIME
MYDEBUG_PRINT_COUNT_CONTENT_TYPE
MYDEBUG_PRINT_DECODED_CONTENT_TOKENS
MYDEBUG_PRINT_DECODED_CONTENT_NUMSTACK

MYDEBUG_PRINT_ALL
MYDEBUG_PRINT_ON_MATCH
MYDEBUG_PRINT_ON_PARSE_OBJ
MYDEBUG_PRINT_ON_PARSE_XREF_STREAMOBJ
MYDEBUG_PRINT_ON_PARSE_STMOBJ
MYDEBUG_PRINT_ON_PARSE_STREAMOBJ
MYDEBUG_PRINT_ON_PARSE_STREAMXOBJ
MYDEBUG_PRINT_ON_PARSE_TOUNICODE_STREAM
MYDEBUG_PRINT_ON_PARSE_CMAP_STREAM
MYDEBUG_PRINT_ON_PARSE_PAGETREEOBJ
MYDEBUG_PRINT_ON_PARSE_LENGTHOBJ
MYDEBUG_PRINT_ON_PARSE_INTEGEROBJ
MYDEBUG_PRINT_ON_PARSE_RESOURCESDICT
MYDEBUG_PRINT_ON_PARSE_FONTDESCRIPTOROBJ
MYDEBUG_PRINT_ON_PARSE_FONTOBJ
MYDEBUG_PRINT_ON_PARSE_FONTOBJ_WIDTHS
MYDEBUG_PRINT_ON_PARSE_ENCODINGOBJ
MYDEBUG_PRINT_ON_PARSE_COT

MYDEBUG_PRINT_TST

MYDEBUG_PRINT_ON_GetNextToken_FN

MYDEBUG_PRINT_ON_getObjsOffsets_FN

MYDEBUG_PRINT_ON_ReadHeader_FN
MYDEBUG_PRINT_ON_ReadTrailer_FN
MYDEBUG_PRINT_ON_ReadTrailer_OBJ
MYDEBUG_PRINT_ON_ReadLastTrailer_FN
MYDEBUG_PRINT_ON_ReadLastTrailer_FN_BLOCCO
MYDEBUG_PRINT_ON_ReadTrailerBody_FN

MYDEBUG_PRINT_ON_ParseInfoObject_FN

MYDEBUG_PRINT_ON_ReadTrailerEncryptObject
MYDEBUG_PRINT_ON_PARSE_ENCRYPT_OBJ
MYDEBUG_PRINT_ON_Decrypt

MYDEBUG_PRINT_ON_ManageDecodeParams_FN
MYDEBUG_PRINT_ON_ParseTrailerXRefStreamObject_FN

MYDEBUG_PRINT_ON_PARSE_FN
MYDEBUG_PRINT_ON_ParseStmObj_FN
MYDEBUG_PRINT_ON_ManageContent_FN
MYDEBUG_PRINT_ON_ManageContent_FN_ShowResourceSelected
MYDEBUG_PRINT_ON_ManageContent_FN_ShowFontSelected
MYDEBUG_PRINT_ON_ManageContent_FN_ShowGsSelected
MYDEBUG_PRINT_ON_ManageContent_PrintPageNum
MYDEBUG_PRINT_ON_ManageContent_PrintContent
MYDEBUG_PRINT_ON_ManageContent_PrintContent_HEXCODECHAR
MYDEBUG_PRINT_ON_ManageContent_PrintStrings
MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL
MYDEBUG_PRINT_ON_ParseObject_FN
MYDEBUG_PRINT_ON_myTreeTraversePostOrderLeafOnly_FN
MYDEBUG_PRINT_ON_ParseStreamObject_FN
MYDEBUG_PRINT_ON_ParseDictionaryObject_FN
MYDEBUG_PRINT_ON_ParseLengthObject_FN
*/
