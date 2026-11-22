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


/*
../configure --prefix=./mybuild

mke

make install

make clean
*/

#define _FILE_OFFSET_BITS 64

#if !defined(_WIN64) && !defined(_WIN32)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <locale.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#if !defined(_WIN64) && !defined(_WIN32)
#include <termios.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sched.h>
#endif


#include <wchar.h>

#if defined(_WIN64) || defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

#include "mypdfsearch.h"     // API pubblica: unico header della libreria
#include "mytime.h"
#include "mypdf_cli.h"

//#if !defined(_WIN64) && !defined(_WIN32)
//#include <sys/sysinfo.h>
//#endif


/* -------------------------------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------------------------- */


// Legge la password dal terminale evitando, su Linux e sistemi POSIX,
// di mostrarla sullo schermo durante la digitazione.
static int ReadPasswordFromTerminal(char *pszPassword, size_t capacity, size_t *pPasswordLength)
{
    if (pszPassword == NULL ||
        capacity < 2 ||
        pPasswordLength == NULL)
        return 0;

#if !defined(_WIN64) && !defined(_WIN32)
    {
        struct termios oldSettings;
        struct termios hiddenSettings;
        int bEchoDisabled = 0;

        if (isatty(STDIN_FILENO) &&
            tcgetattr(STDIN_FILENO, &oldSettings) == 0)
        {
            hiddenSettings = oldSettings;
            hiddenSettings.c_lflag &= (tcflag_t)~ECHO;

            if (tcsetattr(
                    STDIN_FILENO,
                    TCSAFLUSH,
                    &hiddenSettings) == 0)
            {
                bEchoDisabled = 1;
            }
        }

        if (fgets(pszPassword, (int)capacity, stdin) == NULL)
        {
            if (bEchoDisabled)
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldSettings);

            return 0;
        }

        if (bEchoDisabled)
        {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldSettings);
            wprintf(L"\n");
            fflush(stdout);
        }
    }
#else
    // Su Windows usiamo per ora l'input standard.
    if (fgets(pszPassword, (int)capacity, stdin) == NULL)
        return 0;
#endif

    // Rimuove CR/LF senza usare funzioni stringa per i dati binari.
    *pPasswordLength = strcspn(pszPassword, "\r\n");
    pszPassword[*pPasswordLength] = '\0';

    return 1;
}

static int MYPDF_CALL MyConsolePasswordCallback(void *pUser,
                                     const char *pdf_filename,
                                     const char *error_msg,
                                     char *out_password,
                                     size_t max_len,
                                     size_t *out_len)
{
    CLI_UNUSED(pUser);

    // Se stdin non e' un terminale interattivo, non c'e' modo di
    // chiedere una password. Annulla subito, senza prompt.
    // Questo copre la modalita' BATCH (es. `mypdfsearch ... < /dev/null`,
    // esecuzione da pipe, cron, script di test).
#if !defined(_WIN64) && !defined(_WIN32)
    if ( !isatty(STDIN_FILENO) )
    {
        if ( error_msg != NULL )
            fprintf(stderr, "%s\n", error_msg);
        return 0;
    }
#else
    if ( !_isatty(_fileno(stdin)) )
    {
        if ( error_msg != NULL )
            fprintf(stderr, "%s\n", error_msg);
        return 0;
    }
#endif

    if ( error_msg != NULL )
        fprintf(stderr, "%s\n", error_msg);

    fprintf(stderr, "Enter the user password for '%s' (empty line to cancel): ", pdf_filename);
    fflush(stderr);

    if ( !ReadPasswordFromTerminal(out_password, max_len, out_len) )
        return 0;

    if ( *out_len == 0 )
        return 0;

    return 1;
}

// IMPORTANTE,  ATTENZIONE!!!
// compilare su Windows con MINGW64, richiede di specificare '-liconv' alla fine, dopo '-lz'
// altrimenti il linker dà errore.

// ./configure CFLAGS="-DMYPDFSEARCH_SHOW_TIME=1"
// make
// sudo make install

int main(int argc, char **argv)
{
	int retValue = EXIT_SUCCESS;

	#if defined(__GLIBC__)
	// Con le impostazioni predefinite glibc alza dinamicamente la soglia oltre la quale usa mmap() (fino a 32 MB) e
	// tiene nell'heap i grandi buffer liberati: il picco di RAM resta quello del buffer piu' grande mai allocato.
	// Fissando la soglia, i buffer grandi (stream decodificati, array dei font) tornano al sistema appena liberati.
	// Stesso output, stessi tempi; picchi di RAM molto piu' bassi sui PDF con stream grandi.
	mallopt(M_MMAP_THRESHOLD, 128 * 1024);
	#endif

	CliOptions cliOptions;
	CliOutput cliOutput = { NULL, NULL };

	mypdf_Context *ctx = NULL;
	mypdf_Status status;
	int bInitDone = 0;

	#if defined(MYPDFSEARCH_SHOW_TIME)
	myTimes mt_start;
	myTimes mt_end;
	int bTimeStartValid = 0;

	if ( !getTimes(&mt_start) )
	{
		wprintf(L"\nError getTimes(&mt_start) in main.c\n\n");
		retValue = EXIT_FAILURE;
		goto uscita;
	}
	bTimeStartValid = 1;
	#endif

#if defined(_WIN64) || defined(_WIN32)
	//SetConsoleOutputCP(65001);
	//_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stdout), _O_U8TEXT);
#else
	//if ( !setlocale(LC_CTYPE, "it_IT.utf8") )
	if ( !setlocale(LC_ALL, "C.UTF-8") )
	{
		wprintf(L"Unable to select utf8!\n");
		printf("Unable to select utf8!\n");
		retValue = EXIT_FAILURE;
		goto uscita;
	}
#endif

	if ( MYPDF_OK != mypdf_Init() )
	{
		fprintf(stderr, "ERROR: global OpenSSL initialization failed.\n");

		retValue = EXIT_FAILURE;
		goto uscita;
	}
	bInitDone = 1;

	if ( !myParseCommandLine(&cliOptions, argc, argv) )
	{
		retValue = EXIT_FAILURE;
		goto uscita;
	}

	if ( cliOptions.bOptVersionOrHelp )
	{
		retValue = EXIT_SUCCESS;
		goto uscita;
	}

	if ( cliOptions.szOutputFile[0] != '\0' )
	{
		if ( !MakeAndOpenOutputFile(cliOptions.szOutputFile, &(cliOutput.fpOutput)) )
		{
			retValue = EXIT_FAILURE;
			goto uscita;
		}
	}

	ctx = mypdf_CreateContext();
	if ( NULL == ctx )
	{
		fprintf(stderr, "ERROR: unable to create the library context.\n");
		retValue = EXIT_FAILURE;
		goto uscita;
	}

	// Opzioni
	if ( MYPDF_OK != mypdf_SetSearchWords(ctx, cliOptions.szWords) ||
	     MYPDF_OK != mypdf_SetPageRange(ctx, cliOptions.fromPage, cliOptions.toPage) ||
	     MYPDF_OK != mypdf_SetOptionInt(ctx, MYPDF_OPT_SORT_GLYPHS, cliOptions.bSortGlyphs) ||
	     MYPDF_OK != mypdf_SetOptionInt(ctx, MYPDF_OPT_SHOW_ERRORS, cliOptions.bShowErrors) ||
	     MYPDF_OK != mypdf_SetOptionInt(ctx, MYPDF_OPT_SHOW_WARNINGS, cliOptions.bShowWarnings) ||
	     MYPDF_OK != mypdf_SetOptionInt(ctx, MYPDF_OPT_MATCH_MODE,
	                                    cliOptions.bPrefix ? MYPDF_MATCH_PREFIX :
	                                    cliOptions.bSuffix ? MYPDF_MATCH_SUFFIX :
	                                    cliOptions.bContains ? MYPDF_MATCH_CONTAINS : MYPDF_MATCH_WORD) )
	{
		fprintf(stderr, "ERROR: %s\n", mypdf_GetLastError(ctx));
		retValue = EXIT_FAILURE;
		goto uscita;
	}

	// Callback: messaggi sempre; password; uscita: -e (estrazione del testo) -> testo delle pagine; -p (ricerca) -> parole trovate.
	mypdf_SetMessageCallback(ctx, CliOnMessage, &cliOutput);
	mypdf_SetPasswordCallback(ctx, MyConsolePasswordCallback, NULL);
	mypdf_SetFileStartCallback(ctx, CliOnFileStart, &cliOutput);
	if ( cliOptions.szFilePdf[0] != '\0' )
	{
		mypdf_SetPageStartCallback(ctx, CliOnPageStart, &cliOutput);
		mypdf_SetPageTextCallback(ctx, CliOnPageText, &cliOutput);
	}
	else
	{
		mypdf_SetWordFoundCallback(ctx, CliOnWordFound, &cliOutput);
	}

	wprintf(L"\n");

	if ( cliOptions.szFilePdf[0] != '\0' )
		status = mypdf_ProcessFile(ctx, cliOptions.szFilePdf);
	else
		status = mypdf_ProcessDirectory(ctx, cliOptions.szPath, !(cliOptions.bNoSubDirs));

	switch ( status )
	{
		case MYPDF_OK:
		case MYPDF_ERR_ENCRYPTED:      // saltato per la password: l'avviso e' gia' stato mostrato da CliOnMessage
			break;
		case MYPDF_ERR_FILE_ERRORS:
			retValue = MYPDFSEARCH_EXIT_FILE_ERRORS;
			break;
		default:
			fprintf(stderr, "ERROR: %s\n", mypdf_GetLastError(ctx));
			retValue = EXIT_FAILURE;
			break;
	}

uscita:

	mypdf_DestroyContext(ctx);

	if ( NULL != cliOutput.fpErrors )
		fclose(cliOutput.fpErrors);

	if ( NULL != cliOutput.fpOutput )
	{
		fclose(cliOutput.fpOutput);
		cliOutput.fpOutput = NULL;
	}

	// Rilascio del provider globale OpenSSL: a questo punto tutte le elaborazioni crittografiche sono terminate.
	if ( bInitDone )
		mypdf_Shutdown();

	#if defined(MYPDFSEARCH_SHOW_TIME)
	if ( !(cliOptions.bOptVersionOrHelp) && bTimeStartValid )
	{
		double elapsedReal;
		double elapsedUser;
		double elapsedSystem;
		double cpuUsagePercent;

		if ( !getTimes(&mt_end) )
		{
			wprintf(L"\nError getTimes(&mt_end) in main.c\n\n");
			retValue = EXIT_FAILURE;
		}
		else
		{
			elapsedReal = mt_end.realTime - mt_start.realTime;
			elapsedUser = mt_end.userTime - mt_start.userTime;
			elapsedSystem = mt_end.systemTime - mt_start.systemTime;

			wprintf(L"\n\n-----------------------------------------------------------------------\n");
			wprintf(L"Execution times (seconds):\n");
			wprintf(L"  Real Time  : %5.5f\n  User Time  : %5.5f\n  System Time: %5.5f\n",
				elapsedReal, elapsedUser, elapsedSystem);

			if (elapsedReal > 0.0)
			{
				cpuUsagePercent = ((elapsedUser + elapsedSystem) / elapsedReal) * 100.0;
				wprintf(L"  Average CPU: %.1f%% (100%% equals one logical CPU)\n\n",
					cpuUsagePercent);
			}
			else
			{
				wprintf(L"  Average CPU: not computable (zero real time)\n\n");
			}

			PrintHardwareInfo();
		}
	}
	#else
	wprintf(L"\n\n");
	#endif

	return retValue;
}
