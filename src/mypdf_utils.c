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

#define _FILE_OFFSET_BITS 64

#if !defined(_WIN64) && !defined(_WIN32)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#if defined(_WIN64) || defined(_WIN32)
#include <windows.h>
#endif

#include "mypdf_internal.h"
#include "scanner.h"
#include "myUnicodeNFC.h"
#include "mypdf_utils.h"

FilesList* addFileToFilesList(FilesList *first, FilesList *newFile)
{
	FilesList *n;

	n = (FilesList*)malloc(sizeof(FilesList));

	if( n == NULL )
		return NULL;

	#if !defined(_WIN64) && !defined(_WIN32)
	strcpy(n->myPathName, newFile->myPathName);
	#endif
	strcpy(n->myFileName, newFile->myFileName);

	//wprintf(L"ECCO: <%s/%s>\n", n->myPathName, n->myFileName);

	n->next = NULL;

	if ( first != NULL )
		n->next = first;

	return n;
}

void freeFilesList(FilesList *first)
{
	FilesList *n1 = first, *n2;
	while ( n1 != NULL )
	{
		n2 = n1->next;
		free(n1);
		n1 = n2;
	}
}

#if defined(_WIN64) || defined(_WIN32)
void GetErrorString(DWORD err, char *szError, int len)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;

	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf,
		0, NULL);

	strncpy(szError, lpMsgBuf, len);

	LocalFree(lpMsgBuf);
}

FilesList* getFilesRecursive(Params *pParams, char *dirName, int lenOrig, int bNoSubDirs, FilesList* myFilesList)
{
	char pathName[MAX_PATH + 1];

	WIN32_FIND_DATAA data;

	char fname[_MAX_DIR];

	FilesList myFile;

	strncpy(fname, dirName, _MAX_DIR - 1);
	strncat(fname, "\\*.*", _MAX_DIR - 1);

	HANDLE h = FindFirstFileA(fname, &data);
	if (h != INVALID_HANDLE_VALUE)
	{
		do {

			strncpy(pathName, dirName, MAX_PATH);
			strncat(pathName, "\\", MAX_PATH);
			strncat(pathName, data.cFileName, MAX_PATH);

			if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				//if (!(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !(data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
				if ( !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) )
				{
					if (strncmp(data.cFileName, ".", _MAX_DIR) != 0 && strncmp(data.cFileName, "..", _MAX_DIR) != 0)
					{
						if ( !bNoSubDirs )
							myFilesList = getFilesRecursive(pParams, pathName, lenOrig, bNoSubDirs, myFilesList);
					}
				}
			}
			else
			{
				int len = strnlen(pathName, MAX_LEN_STR);
				if ( len > 4 ) // .pdf = 4
				{
					if ( tolower(pathName[len - 1]) == 'f' && tolower(pathName[len - 2]) == 'd' && tolower(pathName[len - 3]) == 'p' && pathName[len - 4] == '.' )
					{
						strncpy(myFile.myFileName, pathName, len + 1);
						myFilesList = addFileToFilesList(myFilesList, &myFile);
						if ( !myFilesList )
							goto uscita;
					}
				}

			}
		} while (FindNextFileA(h, &data) != 0);
		FindClose(h);
	}

	uscita:

	return myFilesList;
}
#else // Linux
FilesList* getFilesRecursive(Params *pParams, char *dirName, int lenOrig, int bNoSubDirs, FilesList* myFilesList)
{
	FilesList myFile;
	DIR *dir = NULL;
	struct dirent *entry;
	char pathName[PATH_MAX + 1];

	int i;
	int len;
	int len2;
	int index;
	char *szExt;

	dir = opendir(dirName);
	if( dir == NULL )
	{
		myReportError(pParams, "Errore nell'apertura della directory '%s': %s", dirName, strerror(errno));
		return myFilesList;
	}

	myFile.myPathName[0] = '\0';
	myFile.myFileName[0] = '\0';

	entry = readdir(dir);
	while( entry != NULL )
	{
		struct stat entryInfo;

		if( ( strncmp(entry->d_name, ".", PATH_MAX) == 0 ) ||
		    ( strncmp(entry->d_name, "..", PATH_MAX) == 0 ) )
		{
			entry = readdir(dir);
		    continue;
		}

		strncpy(pathName, dirName, PATH_MAX);
		strncat(pathName, "/", PATH_MAX);
		strncat(pathName, entry->d_name, PATH_MAX);

		if( lstat(pathName, &entryInfo) == 0 )
		{
			if( S_ISDIR(entryInfo.st_mode) )		// directory
			{
				if ( !bNoSubDirs )
					myFilesList = getFilesRecursive(pParams, pathName, lenOrig, bNoSubDirs, myFilesList);
			}
			else if( S_ISREG(entryInfo.st_mode) )	// regular file
			{
				len = strlen(entry->d_name);

				index = 0;
				for ( i = len - 1; i >= 0; i-- )
				{

					if ( entry->d_name[i] == '.' )
					{
						index = i;
						break;
					}
				}

				if ( index > 0 )
				{
					szExt = &entry->d_name[index];
					if ( strcmp(".pdf", szExt) == 0 )
					{
						len2 = strnlen(pathName, PATH_MAX);
						strncpy(myFile.myPathName, pathName, len2 + 1);
						index = strlen(pathName) - len - 1;
						myFile.myPathName[index] = '\0';

						strncpy(myFile.myFileName, entry->d_name, PATH_MAX);

						myFilesList = addFileToFilesList(myFilesList, &myFile);
						if ( !myFilesList )
							return NULL;
					}
				}
			}
		}
		else
		{
			myReportError(pParams, "Errore lstat %s: %s", pathName, strerror(errno));
		}

		entry = readdir(dir);
	}

	closedir(dir);

	return myFilesList;
}
#endif


long GetProcessors(void)
{
	//long nProcs = -1;
	long nProcs_max = -1;

#if defined(_WIN64) || defined(_WIN32)
	SYSTEM_INFO info;
	//GetSystemInfo(&info);
	GetNativeSystemInfo(&info);
	nProcs_max = info.dwNumberOfProcessors;
#else
	//nProcs_max = sysconf(_SC_NPROCESSORS_ONLN);
	nProcs_max = sysconf(_SC_NPROCESSORS_CONF);
#endif

	return nProcs_max;
}


// Riconosce i separatori di parola per la ricerca.
// Include spazio, punteggiatura latina e punteggiatura Unicode di uso comune
// nelle lingue che gestiamo (danda Devanagari/Bengalese, ideographic full stop,
// virgola araba, ecc.).
static int IsWordSeparator(wchar_t c)
{
    if (c == L' ' || c == L'\t' || c == L'\n' || c == L'\r' || c == L'\f' || c == L'\v') return 1;
    // Punteggiatura latina
    if (c == L'.' || c == L',' || c == L';' || c == L':' || c == L'!' || c == L'?') return 1;
    if (c == L'"' || c == L'\'' || c == L'(' || c == L')' || c == L'[' || c == L']') return 1;
    if (c == L'{' || c == L'}' || c == L'-' || c == L'_' || c == L'/' || c == L'\\') return 1;
    // Punteggiatura Unicode delle lingue gestite
    if (c == 0x0964 || c == 0x0965) return 1;  // Devanagari danda, double danda
    if (c == 0x3002) return 1;                 // Ideographic full stop
    if (c == 0xFF01 || c == 0xFF1F) return 1;  // Fullwidth exclamation / question
    if (c == 0xFF0C) return 1;                 // Fullwidth comma
    if (c == 0x060C) return 1;                 // Arabic comma
    if (c == 0x061B) return 1;                 // Arabic semicolon
    if (c == 0x061F) return 1;                 // Arabic question mark
    if (c == 0x3001) return 1;                 // Ideographic comma
    if (c == 0x00AB || c == 0x00BB) return 1;  // Quotes
    if (c == 0x2018 || c == 0x2019) return 1;  // Single quotes
    if (c == 0x201C || c == 0x201D) return 1;  // Double quotes
    if (c == 0x2026) return 1;                 // Ellipsis
    if (c == 0x2013 || c == 0x2014) return 1;  // Dashes
    if (c == 0xFF1A) return 1;                 // Fullwidth colon
    if (c == 0xFF1B) return 1;                 // Fullwidth semicolon
    if (c == 0x300C || c == 0x300D) return 1;  // CJK brackets
    if (c == 0x300E || c == 0x300F) return 1;  // CJK white brackets
    if (c == 0xFF08 || c == 0xFF09) return 1;  // Fullwidth parentheses
    if (c == 0xFF3B || c == 0xFF3D) return 1;  // Fullwidth square brackets

    return 0;
}

// Aggiunge una parola all'array di ricerca, evitando duplicati.
// Ritorna 1 se OK (aggiunta o gia' presente), 0 su errore di allocazione.
static int AppendWordToSearchArray(Params *pParams, const wchar_t *pwszWord, size_t len)
{
    int i;

    // Deduplica: se la parola e' gia' presente, non la riaggiungo.
    for ( i = 0; i < pParams->countWordsToSearch; i++ )
    {
        if ( pParams->pWordsToSearchArray[i] != NULL &&
             wcslen(pParams->pWordsToSearchArray[i]) == len &&
             wcsncmp(pParams->pWordsToSearchArray[i], pwszWord, len) == 0 )
        {
            return 1;
        }
    }

    if ( pParams->countWordsToSearch >= MAX_WORDS_TO_SEARCH )
        return 0;

    pParams->pWordsToSearchArray[pParams->countWordsToSearch] =
        (wchar_t*)malloc(sizeof(wchar_t) * len + sizeof(wchar_t));

    if ( NULL == pParams->pWordsToSearchArray[pParams->countWordsToSearch] )
        return 0;

    wcsncpy(pParams->pWordsToSearchArray[pParams->countWordsToSearch], pwszWord, len);
    pParams->pWordsToSearchArray[pParams->countWordsToSearch][len] = L'\0';
    pParams->countWordsToSearch++;

    return 1;
}

int mySplitWords(Params *pParams, wchar_t *pwszOption)
{
    int retValue = 1;

    size_t x = 0;
    size_t queryLength = 0;
    size_t normalizedLength = 0;

    wchar_t c;

    wchar_t *pwszNormalizedOption = NULL;

    MyUnicodeNFCStatus nfcStatus;

    if ( NULL == pParams || NULL == pwszOption )
        return 0;

    // ============================================================
    // PASSAGGIO 1: normalizzazione NFC della query.
    // ============================================================

    queryLength = wcslen(pwszOption);

    nfcStatus = myUnicodeNFC_Normalize(
        pwszOption, queryLength,
        &pwszNormalizedOption, &normalizedLength
    );

    if ( MY_UNICODE_NFC_OK != nfcStatus )
    {
        myReportError(pParams, "ERRORE mySplitWords: normalizzazione NFC fallita: %s.",
                myUnicodeNFC_StatusString(nfcStatus));
        retValue = 0;
        goto uscita;
    }

    if ( normalizedLength > MAX_LEN_STR )
    {
        myReportError(pParams, "ERRORE mySplitWords: la query normalizzata NFC supera MAX_LEN_STR (%d caratteri).",
                MAX_LEN_STR);
        retValue = 0;
        goto uscita;
    }

    // ============================================================
    // PASSAGGIO 2: suddivisione in parole e token CJK.
    // ============================================================

    {
        wchar_t wszTemp[MAX_LEN_STR + 1];
        wchar_t wszAsianRun[MAX_LEN_STR + 1];
        int y = 0;
        int idxAsianRun = 0;
        wchar_t cPrevAsian = L'\0';

        while ( 1 )
        {
            c = pwszNormalizedOption[x];

            if ( L'\0' != c && L' ' != c && !IsWordSeparator(c) )
            {
                wchar_t cLower = towlower(c);

                if ( IS_ASIAN_SCRIPT(cLower) )
                {
                    // Se c'era una parola occidentale accumulata, salviamola.
                    if ( y > 0 && cPrevAsian == L'\0' )
                    {
                        wszTemp[y] = L'\0';
                        if ( !AppendWordToSearchArray(pParams, wszTemp, (size_t)y) )
                        {
                            retValue = 0;
                            goto uscita;
                        }
                        y = 0;
                    }

                    // 1. Emette il singolo carattere.
                    wszTemp[0] = cLower;
                    wszTemp[1] = L'\0';
                    if ( !AppendWordToSearchArray(pParams, wszTemp, 1) )
                    {
                        retValue = 0;
                        goto uscita;
                    }

                    // 2. Emette il bigramma.
                    if ( cPrevAsian != L'\0' )
                    {
                        wszTemp[0] = cPrevAsian;
                        wszTemp[1] = cLower;
                        wszTemp[2] = L'\0';
                        if ( !AppendWordToSearchArray(pParams, wszTemp, 2) )
                        {
                            retValue = 0;
                            goto uscita;
                        }
                    }

                    // 3. Accumula nella run completa.
                    if ( idxAsianRun < MAX_LEN_STR )
                    {
                        wszAsianRun[idxAsianRun++] = cLower;
                        wszAsianRun[idxAsianRun] = L'\0';
                    }

                    cPrevAsian = cLower;
                }
                else
                {
                    // Carattere occidentale: flush della run CJK e reset.
                    if ( idxAsianRun > 0 )
                    {
                        if ( !AppendWordToSearchArray(pParams, wszAsianRun, (size_t)idxAsianRun) )
                        {
                            retValue = 0;
                            goto uscita;
                        }
                        idxAsianRun = 0;
                    }

                    if ( cPrevAsian != L'\0' )
                    {
                        cPrevAsian = L'\0';
                        y = 0;
                    }

                    if ( y < MAX_LEN_STR )
                        wszTemp[y++] = cLower;
                }
            }
            else
            {
                // Separatore o fine stringa: flush di entrambi i buffer.
                if ( idxAsianRun > 0 )
                {
                    if ( !AppendWordToSearchArray(pParams, wszAsianRun, (size_t)idxAsianRun) )
                    {
                        retValue = 0;
                        goto uscita;
                    }
                    idxAsianRun = 0;
                }

                if ( y > 0 && cPrevAsian == L'\0' )
                {
                    wszTemp[y] = L'\0';
                    if ( !AppendWordToSearchArray(pParams, wszTemp, (size_t)y) )
                    {
                        retValue = 0;
                        goto uscita;
                    }
                }

                cPrevAsian = L'\0';
                y = 0;

                if ( L'\0' == c )
                    goto uscita;
            }

            x++;
        }
    }

uscita:

    if ( NULL != pwszNormalizedOption )
    {
        free(pwszNormalizedOption);
        pwszNormalizedOption = NULL;
    }

    return retValue;
}


void checkEndianness(Params *pParams)
{
	//int i = 0xDEED1234;
	//int i = 0xABCDEF12;
	uint32_t i = 0xABCDEF12;
	//int j;
	unsigned char *buff;

	//wprintf(L"\nvalue = %lX\n", i);

	buff = (unsigned char*)&i;

	pParams->nThisMachineEndianness = MACHINE_ENDIANNESS_UNKNOWN;

	if( buff[0] == 0xAB )
	{
		//wprintf(L"This machine is BIG endian - HIGHER order bytes come first");
		pParams->nThisMachineEndianness = MACHINE_ENDIANNESS_BIG_ENDIAN;
	}
	else
	{
		pParams->nThisMachineEndianness = MACHINE_ENDIANNESS_LITTLE_ENDIAN;
		//wprintf(L"This machine is LITTLE endian - LOWER order bytes come first");
	}

	//wprintf(L"\nhere are the 4 bytes\n");

	//for( j = 0; j < 4; j++ )
	//{
	//	wprintf(L"byte [%d] = %X\n", j, buff[j]);
	//}

	//wprintf(L"\n");
}


char *myWideToUtf8(const wchar_t *pwsz, size_t len, size_t *pOutLen)
{
	size_t i;
	size_t n = 0;
	char *out = (char*)malloc(len * 4 + 1);

	if ( NULL == out )
		return NULL;

	for ( i = 0; i < len; i++ )
	{
		uint32_t cp = (uint32_t)pwsz[i];

		#if defined(_WIN32) || defined(_WIN64)
		// wchar_t a 16 bit: ricompone le coppie surrogate
		if ( cp >= 0xD800 && cp <= 0xDBFF && (i + 1) < len && (uint32_t)pwsz[i + 1] >= 0xDC00 && (uint32_t)pwsz[i + 1] <= 0xDFFF )
		{
			cp = 0x10000 + ((cp - 0xD800) << 10) + ((uint32_t)pwsz[i + 1] - 0xDC00);
			i++;
		}
		#endif

		if ( cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF) )
			cp = 0xFFFD;

		if ( cp < 0x80 )
			out[n++] = (char)cp;
		else if ( cp < 0x800 )
		{
			out[n++] = (char)(0xC0 | (cp >> 6));
			out[n++] = (char)(0x80 | (cp & 0x3F));
		}
		else if ( cp < 0x10000 )
		{
			out[n++] = (char)(0xE0 | (cp >> 12));
			out[n++] = (char)(0x80 | ((cp >> 6) & 0x3F));
			out[n++] = (char)(0x80 | (cp & 0x3F));
		}
		else
		{
			out[n++] = (char)(0xF0 | (cp >> 18));
			out[n++] = (char)(0x80 | ((cp >> 12) & 0x3F));
			out[n++] = (char)(0x80 | ((cp >> 6) & 0x3F));
			out[n++] = (char)(0x80 | (cp & 0x3F));
		}
	}

	out[n] = '\0';
	if ( NULL != pOutLen )
		*pOutLen = n;

	return out;
}
