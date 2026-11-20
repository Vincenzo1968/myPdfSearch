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

#include "mypdf_internal.h"
#include "scanner.h"

void PrintHelpCommandLine(void);
void PrintVersionInfo(void);

int myParseOption(Params *pParams, char *pszOption);
int myParseCommandLine(Params *pParams, int argc, char **argv);

int MakeAndOpenOutputFile(Params *pParams);

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
	FILE *fpOutput;
	Params *pParams;     // per il file di log degli errori (pParams->fpErrors, aperto al primo messaggio)
} CliOutput;

void MYPDF_CALL CliOnMessage(void *pUser, mypdf_MessageLevel level, const char *pszFile, const char *pszMessage);
void MYPDF_CALL CliOnFileStart(void *pUser, const char *pszFile);
void MYPDF_CALL CliOnPageStart(void *pUser, const char *pszFile, int32_t nPage);
int  MYPDF_CALL CliOnPageText(void *pUser, const char *pszFile, int32_t nPage, const char *pszText, size_t nLen);
int  MYPDF_CALL CliOnWordFound(void *pUser, const char *pszFile, int32_t nPage, const char *pszWord);

#endif /* MYPDF_CLI_INCLUDED */
