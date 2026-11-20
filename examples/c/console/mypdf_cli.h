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

/* Parte del programma a riga di comando: aiuto, versione, opzioni, file di output, informazioni sull'hardware.
   Questi file NON fanno parte della libreria. */

#ifndef MYPDF_CLI_INCLUDED
#define MYPDF_CLI_INCLUDED

#include <stdio.h>
#include <stddef.h>
#include <wchar.h>
#include <limits.h>

#include "mypdfsearch.h"      // UNICO header della libreria usato dal programma a riga di comando

#define CLI_UNUSED(x) (void)(x)
#define CLI_MAX_STR   1024            // lunghezza massima di un'opzione
#define CLI_MAX_PATH  4096            // buffer per percorsi e parole (UTF-8)

void PrintHelpCommandLine(void);
void PrintVersionInfo(void);

// Opzioni della riga di comando (nessuna dipendenza dai tipi interni della libreria).
typedef struct tagCliOptions
{
	int  bOptVersionOrHelp;
	int  bNoSubDirs;
	int  bSortGlyphs;
	int  bShowErrors;
	int  bShowWarnings;
	int  fromPage;
	int  toPage;
	char szPath[CLI_MAX_PATH];
	char szFilePdf[CLI_MAX_PATH];
	char szOutputFile[CLI_MAX_PATH];
	char szWords[CLI_MAX_PATH];       // UTF-8, parole separate da spazi
} CliOptions;

int myParseOption(CliOptions *pParams, char *pszOption);
int myParseCommandLine(CliOptions *pParams, int argc, char **argv);

// Crea (senza sovrascrivere) il file di output; *ppFile = NULL in caso di errore.
int MakeAndOpenOutputFile(const char *pszOutputFile, FILE **ppFile);

void PrintHardwareInfo(void);


// Exit codes:
//   0 (EXIT_SUCCESS)  all files processed without errors
//   1 (EXIT_FAILURE)  fatal error or invalid command line
//   2                 at least one file could not be processed (the others have been processed anyway)
#define MYPDFSEARCH_EXIT_FILE_ERRORS 2

wchar_t* myMultibyteToWidechar(const char *psz, size_t mbslen);

// Uscita del programma a riga di comando: callback che stampano esattamente come prima del refactoring.
// 'pUser' deve essere un puntatore a CliOutput (fpOutput = file di -o, oppure NULL per stampare solo a video).
typedef struct tagCliOutput
{
	FILE *fpOutput;      // file di -o (NULL = solo video)
	FILE *fpErrors;      // log degli errori, aperto al primo messaggio
} CliOutput;

void MYPDF_CALL CliOnMessage(void *pUser, mypdf_MessageLevel level, const char *pszFile, const char *pszMessage);
void MYPDF_CALL CliOnFileStart(void *pUser, const char *pszFile);
void MYPDF_CALL CliOnPageStart(void *pUser, const char *pszFile, int32_t nPage);
int  MYPDF_CALL CliOnPageText(void *pUser, const char *pszFile, int32_t nPage, const char *pszText, size_t nLen);
int  MYPDF_CALL CliOnWordFound(void *pUser, const char *pszFile, int32_t nPage, const char *pszWord);

#endif /* MYPDF_CLI_INCLUDED */
