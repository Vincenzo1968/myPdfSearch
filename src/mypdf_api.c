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

// Implementazione dell'API pubblica (vedi mypdfsearch.h).
//
// NOTA (tappa 3b, passo 2): il testo delle pagine, le parole trovate e l'inizio di ogni file escono
// dalle callback. La callback dei messaggi non e' ancora collegata (passo 3).

#include <sys/stat.h>

#include "mypdf_internal.h"
#include "mypdf_utils.h"
#include "mycrypt.h"
#include "parser.h"
#include "scanner.h"

#ifndef VERSION
#define VERSION "0.0.0"
#endif

struct mypdf_Context
{
	Params params;

	mypdf_PasswordCallback pfnPassword;
	void *pPasswordUser;

	char szLastError[512];
};

/* ------------------------------------------------------------------------ */

const char * MYPDF_CALL mypdf_GetVersion(void)
{
	return VERSION;
}

const char * MYPDF_CALL mypdf_StatusString(mypdf_Status status)
{
	switch ( status )
	{
		case MYPDF_OK:                   return "OK";
		case MYPDF_ERR_INVALID_ARGUMENT: return "invalid argument";
		case MYPDF_ERR_OUT_OF_MEMORY:    return "out of memory";
		case MYPDF_ERR_FILE_OPEN:        return "cannot open file";
		case MYPDF_ERR_NOT_A_PDF:        return "not a PDF file";
		case MYPDF_ERR_CORRUPTED:        return "corrupted PDF structure";
		case MYPDF_ERR_ENCRYPTED:        return "encrypted PDF: password missing or wrong";
		case MYPDF_ERR_UNSUPPORTED:      return "unsupported feature";
		case MYPDF_ERR_CANCELLED:        return "cancelled by callback";
		case MYPDF_ERR_FILE_ERRORS:      return "processed, but with errors on one or more files";
		case MYPDF_ERR_INTERNAL:         return "internal error";
		default:                         return "unknown status";
	}
}

const char * MYPDF_CALL mypdf_GetLastError(const mypdf_Context *ctx)
{
	if ( NULL == ctx )
		return "";
	return ctx->szLastError;
}

/* ------------------------------------------------------------------------ */

mypdf_Status MYPDF_CALL mypdf_Init(void)
{
	if ( !PdfCryptoInitialize() )
		return MYPDF_ERR_INTERNAL;
	return MYPDF_OK;
}

void MYPDF_CALL mypdf_Shutdown(void)
{
	PdfCryptoShutdown();
}

/* ------------------------------------------------------------------------ */

static void FreeWordsToSearch(Params *pParams)
{
	int i;

	if ( NULL == pParams->pWordsToSearchArray )
		return;

	for ( i = 0; i < pParams->countWordsToSearch; i++ )
		free(pParams->pWordsToSearchArray[i]);

	free(pParams->pWordsToSearchArray);
	pParams->pWordsToSearchArray = NULL;
	pParams->countWordsToSearch = 0;
}

mypdf_Context * MYPDF_CALL mypdf_CreateContext(void)
{
	mypdf_Context *ctx = (mypdf_Context*)calloc(1, sizeof(mypdf_Context));
	if ( NULL == ctx )
		return NULL;

	// Stessi valori iniziali del programma a riga di comando (main.c).
	ctx->params.countWordsToSearch = 0;
	ctx->params.pWordsToSearchArray = NULL;
	ctx->params.bOptVersionOrHelp = 0;
	ctx->params.szPath[0] = '\0';
	ctx->params.szFilePdf[0] = '\0';
	ctx->params.fromPage = 0;
	ctx->params.toPage = 0;
	ctx->params.bNoSubDirs = 0;
	ctx->params.nSortGlyphs = SORT_GLYPHS_BY_APPEARANCE;
	ctx->params.szOutputFile[0] = '\0';
	ctx->params.fpOutput = NULL;
	ctx->params.fpErrors = NULL;   // il file di log, se serve, e' del chiamante (callback dei messaggi)

	checkEndianness(&(ctx->params));

	return ctx;
}

void MYPDF_CALL mypdf_DestroyContext(mypdf_Context *ctx)
{
	if ( NULL == ctx )
		return;

	FreeWordsToSearch(&(ctx->params));

	free(ctx);
}

/* ------------------------------------------------------------------------ */
/* Callback                                                                  */
/* ------------------------------------------------------------------------ */

void MYPDF_CALL mypdf_SetPageTextCallback(mypdf_Context *ctx, mypdf_PageTextCallback cb, void *user_data)
{
	if ( NULL == ctx ) return;
	ctx->params.pfnPageText = cb;
	ctx->params.pPageTextUser = user_data;
}

void MYPDF_CALL mypdf_SetWordFoundCallback(mypdf_Context *ctx, mypdf_WordFoundCallback cb, void *user_data)
{
	if ( NULL == ctx ) return;
	ctx->params.pfnWordFound = cb;
	ctx->params.pWordFoundUser = user_data;
}

void MYPDF_CALL mypdf_SetPageStartCallback(mypdf_Context *ctx, mypdf_PageStartCallback cb, void *user_data)
{
	if ( NULL == ctx ) return;
	ctx->params.pfnPageStart = cb;
	ctx->params.pPageStartUser = user_data;
}

void MYPDF_CALL mypdf_SetFileStartCallback(mypdf_Context *ctx, mypdf_FileStartCallback cb, void *user_data)
{
	if ( NULL == ctx ) return;
	ctx->params.pfnFileStart = cb;
	ctx->params.pFileStartUser = user_data;
}

void MYPDF_CALL mypdf_SetMessageCallback(mypdf_Context *ctx, mypdf_MessageCallback cb, void *user_data)
{
	if ( NULL == ctx ) return;
	ctx->params.pfnMessage = cb;
	ctx->params.pMessageUser = user_data;
}

// Adattatore: dalla callback interna (pUser = contesto) a quella pubblica.
static int PasswordAdapter(void *pUser, const char *pdf_filename, const char *error_msg,
                           char *out_password, size_t max_len, size_t *out_len)
{
	mypdf_Context *ctx = (mypdf_Context*)pUser;

	if ( NULL == ctx || NULL == ctx->pfnPassword )
		return 0;

	return ctx->pfnPassword(ctx->pPasswordUser, pdf_filename, error_msg, out_password, max_len, out_len);
}

void MYPDF_CALL mypdf_SetPasswordCallback(mypdf_Context *ctx, mypdf_PasswordCallback cb, void *user_data)
{
	if ( NULL == ctx ) return;
	ctx->pfnPassword = cb;
	ctx->pPasswordUser = user_data;
	SetOnUserPasswordCallback(&(ctx->params), (NULL != cb) ? PasswordAdapter : NULL, ctx);
}

/* ------------------------------------------------------------------------ */
/* Opzioni                                                                   */
/* ------------------------------------------------------------------------ */

// Decodifica UTF-8 -> wchar_t senza dipendere dalla locale del processo (la libreria non chiama setlocale).
// Ritorna il numero di wchar_t scritti, oppure -1 se la sequenza e' invalida o non entra in 'cap'.
static int Utf8ToWide(const char *s, wchar_t *out, int cap)
{
	const unsigned char *p = (const unsigned char*)s;
	int n = 0;

	while ( *p )
	{
		uint32_t cp;
		int extra;

		if ( *p < 0x80 )                { cp = *p; extra = 0; }
		else if ( (*p & 0xE0) == 0xC0 ) { cp = *p & 0x1F; extra = 1; }
		else if ( (*p & 0xF0) == 0xE0 ) { cp = *p & 0x0F; extra = 2; }
		else if ( (*p & 0xF8) == 0xF0 ) { cp = *p & 0x07; extra = 3; }
		else return -1;
		p++;

		while ( extra-- > 0 )
		{
			if ( (*p & 0xC0) != 0x80 ) return -1;
			cp = (cp << 6) | (*p & 0x3F);
			p++;
		}

		if ( cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF) )
			return -1;

		#if defined(_WIN32) || defined(_WIN64)
		if ( cp >= 0x10000 )   // wchar_t a 16 bit: coppia surrogata
		{
			if ( n + 2 > cap ) return -1;
			cp -= 0x10000;
			out[n++] = (wchar_t)(0xD800 + (cp >> 10));
			out[n++] = (wchar_t)(0xDC00 + (cp & 0x3FF));
			continue;
		}
		#endif

		if ( n + 1 > cap ) return -1;
		out[n++] = (wchar_t)cp;
	}

	out[n] = L'\0';
	return n;
}

mypdf_Status MYPDF_CALL mypdf_SetSearchWords(mypdf_Context *ctx, const char *utf8_words)
{
	if ( NULL == ctx )
		return MYPDF_ERR_INVALID_ARGUMENT;

	FreeWordsToSearch(&(ctx->params));
	ctx->params.szWordsToSearch[0] = L'\0';

	if ( NULL == utf8_words || '\0' == utf8_words[0] )
		return MYPDF_OK;

	if ( Utf8ToWide(utf8_words, ctx->params.szWordsToSearch, MAX_LEN_STR) < 0 )
	{
		snprintf(ctx->szLastError, sizeof(ctx->szLastError), "search words: invalid UTF-8 or text too long");
		return MYPDF_ERR_INVALID_ARGUMENT;
	}

	// L'array delle parole va allocato prima (mySplitWords lo riempie), come fa il programma a riga di comando.
	ctx->params.pWordsToSearchArray = (wchar_t**)calloc(MAX_WORDS_TO_SEARCH, sizeof(wchar_t*));
	if ( NULL == ctx->params.pWordsToSearchArray )
		return MYPDF_ERR_OUT_OF_MEMORY;
	ctx->params.countWordsToSearch = 0;

	if ( !mySplitWords(&(ctx->params), ctx->params.szWordsToSearch) )
	{
		FreeWordsToSearch(&(ctx->params));
		snprintf(ctx->szLastError, sizeof(ctx->szLastError), "search words: cannot split the query");
		return MYPDF_ERR_INVALID_ARGUMENT;
	}

	return MYPDF_OK;
}

mypdf_Status MYPDF_CALL mypdf_SetPageRange(mypdf_Context *ctx, int32_t first, int32_t last)
{
	if ( NULL == ctx )
		return MYPDF_ERR_INVALID_ARGUMENT;

	if ( first > 0 && last > 0 && first > last )
	{
		snprintf(ctx->szLastError, sizeof(ctx->szLastError), "page range: first page greater than last page");
		return MYPDF_ERR_INVALID_ARGUMENT;
	}

	ctx->params.fromPage = (first > 0) ? first : 0;
	ctx->params.toPage = (last > 0) ? last : 0;

	return MYPDF_OK;
}

mypdf_Status MYPDF_CALL mypdf_SetOptionInt(mypdf_Context *ctx, mypdf_Option option, int64_t value)
{
	if ( NULL == ctx )
		return MYPDF_ERR_INVALID_ARGUMENT;

	switch ( option )
	{
		case MYPDF_OPT_SORT_GLYPHS:
			ctx->params.nSortGlyphs = value ? SORT_GLYPHS_BY_ROW : SORT_GLYPHS_BY_APPEARANCE;
			return MYPDF_OK;
		case MYPDF_OPT_SHOW_ERRORS:
			ctx->params.bShowErrors = value ? 1 : 0;
			return MYPDF_OK;
		case MYPDF_OPT_SHOW_WARNINGS:
			ctx->params.bShowWarnings = value ? 1 : 0;
			return MYPDF_OK;
		case MYPDF_OPT_MATCH_MODE:
			if ( value < MYPDF_MATCH_WORD || value > MYPDF_MATCH_CONTAINS )
			{
				snprintf(ctx->szLastError, sizeof(ctx->szLastError), "invalid match mode %lld", (long long)value);
				return MYPDF_ERR_INVALID_ARGUMENT;
			}
			ctx->params.nMatchMode = (int)value;
			return MYPDF_OK;
		default:
			snprintf(ctx->szLastError, sizeof(ctx->szLastError), "unknown option %d", (int)option);
			return MYPDF_ERR_INVALID_ARGUMENT;
	}
}

/* ------------------------------------------------------------------------ */
/* Elaborazione                                                              */
/* ------------------------------------------------------------------------ */

// Lista con un solo file (stessa logica di main.c: su Linux nome e cartella separati).
static FilesList *MakeSingleFileList(const char *path)
{
	FilesList *pList = (FilesList*)malloc(sizeof(FilesList));
	if ( NULL == pList )
		return NULL;

	pList->next = NULL;

	#if !defined(_WIN64) && !defined(_WIN32)
	{
		const char *pSlash = strrchr(path, '/');

		if ( NULL == pSlash )
		{
			// Nome senza cartella: Parse compone "cartella/nome", quindi serve "./nome" (con "" diventerebbe "/nome").
			strcpy(pList->myPathName, ".");
			strcpy(pList->myFileName, path);
		}
		else
		{
			size_t dirLen = (size_t)(pSlash - path);
			memcpy(pList->myPathName, path, dirLen);
			pList->myPathName[dirLen] = '\0';
			strcpy(pList->myFileName, pSlash + 1);
		}
	}
	#else
	strcpy(pList->myFileName, path);
	#endif

	return pList;
}

static mypdf_Status RunParse(mypdf_Context *ctx, FilesList *pList, int bSingleFile)
{
	mypdf_Status st = MYPDF_OK;

	ctx->params.nTotFilesProcessed = 0;
	ctx->params.nTotFilesWithErrors = 0;
	ctx->params.nTotFilesWithWarnings = 0;
	ctx->params.nTotFilesSkippedForPassword = 0;
	ctx->params.bCancelRequested = 0;
	ctx->szLastError[0] = '\0';

	if ( !Parse(&(ctx->params), pList) )
	{
		st = MYPDF_ERR_FILE_ERRORS;
		snprintf(ctx->szLastError, sizeof(ctx->szLastError), "one or more files could not be processed");
	}

	// Un solo file saltato perche' cifrato (password mancante/errata/annullata): stato specifico.
	if ( MYPDF_OK == st && 1 == bSingleFile && ctx->params.nTotFilesSkippedForPassword > 0 )
	{
		st = MYPDF_ERR_ENCRYPTED;
		snprintf(ctx->szLastError, sizeof(ctx->szLastError), "encrypted file skipped: password missing, wrong or request cancelled");
	}

	if ( ctx->params.bCancelRequested )
	{
		st = MYPDF_ERR_CANCELLED;
		snprintf(ctx->szLastError, sizeof(ctx->szLastError), "cancelled by a callback");
	}

	return st;
}

mypdf_Status MYPDF_CALL mypdf_ProcessFile(mypdf_Context *ctx, const char *utf8_path)
{
	FilesList *pList;
	mypdf_Status st;

	if ( NULL == ctx || NULL == utf8_path || '\0' == utf8_path[0] || strlen(utf8_path) >= MAX_LEN_STR )
		return MYPDF_ERR_INVALID_ARGUMENT;

	strcpy(ctx->params.szFilePdf, utf8_path);
	ctx->params.szPath[0] = '\0';

	pList = MakeSingleFileList(utf8_path);
	if ( NULL == pList )
		return MYPDF_ERR_OUT_OF_MEMORY;

	st = RunParse(ctx, pList, 1);

	freeFilesList(pList);

	return st;
}

mypdf_Status MYPDF_CALL mypdf_ProcessDirectory(mypdf_Context *ctx, const char *utf8_dir, int32_t recursive)
{
	FilesList *pList = NULL;
	mypdf_Status st;
	int bIsDir;

	if ( NULL == ctx || NULL == utf8_dir || '\0' == utf8_dir[0] || strlen(utf8_dir) >= MAX_LEN_STR )
		return MYPDF_ERR_INVALID_ARGUMENT;

	strcpy(ctx->params.szPath, utf8_dir);
	ctx->params.szFilePdf[0] = '\0';
	ctx->params.bNoSubDirs = recursive ? 0 : 1;

	#if !defined(_WIN64) && !defined(_WIN32)
	{
		struct stat entryInfo;

		if ( lstat(utf8_dir, &entryInfo) != 0 )
		{
			snprintf(ctx->szLastError, sizeof(ctx->szLastError), "cannot access '%s'", utf8_dir);
			return MYPDF_ERR_FILE_OPEN;
		}
		bIsDir = S_ISDIR(entryInfo.st_mode);
		if ( !bIsDir && !S_ISREG(entryInfo.st_mode) )
			return MYPDF_ERR_INVALID_ARGUMENT;
	}
	#else
	{
		DWORD attr = GetFileAttributesA(utf8_dir);

		if ( INVALID_FILE_ATTRIBUTES == attr )
		{
			snprintf(ctx->szLastError, sizeof(ctx->szLastError), "cannot access '%s'", utf8_dir);
			return MYPDF_ERR_FILE_OPEN;
		}
		bIsDir = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}
	#endif

	if ( bIsDir )
	{
		pList = getFilesRecursive(&(ctx->params), ctx->params.szPath, (int)strlen(utf8_dir), ctx->params.bNoSubDirs, NULL);
		if ( NULL == pList )
		{
			snprintf(ctx->szLastError, sizeof(ctx->szLastError), "cannot build the file list of '%s'", utf8_dir);
			return MYPDF_ERR_FILE_OPEN;
		}
	}
	else
	{
		pList = MakeSingleFileList(utf8_dir);
		if ( NULL == pList )
			return MYPDF_ERR_OUT_OF_MEMORY;
	}

	st = RunParse(ctx, pList, !bIsDir);

	freeFilesList(pList);

	return st;
}

void MYPDF_CALL mypdf_GetStats(const mypdf_Context *ctx,
                               int64_t *files_processed,
                               int64_t *files_with_errors,
                               int64_t *files_with_warnings)
{
	if ( NULL != files_processed )     *files_processed     = (NULL != ctx) ? ctx->params.nTotFilesProcessed : 0;
	if ( NULL != files_with_errors )   *files_with_errors   = (NULL != ctx) ? ctx->params.nTotFilesWithErrors : 0;
	if ( NULL != files_with_warnings ) *files_with_warnings = (NULL != ctx) ? ctx->params.nTotFilesWithWarnings : 0;
}

void MYPDF_CALL mypdf_Free(void *ptr)
{
	free(ptr);
}
