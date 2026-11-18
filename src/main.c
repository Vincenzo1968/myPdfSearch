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

#include "mypdf_internal.h"
#include "mytime.h"
#include "parser.h"
#include "myUnicodeNFC.h"
#include "mypdf_utils.h"
#include "mypdf_cli.h"

//#if !defined(_WIN64) && !defined(_WIN32)
//#include <sys/sysinfo.h>
//#endif


/* -------------------------------------------------------------------------------------------------- */

/*
Per generare le dipendenze per il Makefile, usare -MM:

[vincenzo]$ gcc -MM -D_GNU_SOURCE myoctal.c mydictionaryqueuelist.c mystringqueuelist.c mycontentqueuelist.c mynumstacklist.c myintqueuelist.c mydecode.c scanner.c parser.c main.c -lz

myoctal.o: myoctal.c myoctal.h mypdfsearch.h

mydictionaryqueuelist.o: mydictionaryqueuelist.c scanner.h mypdfsearch.h \
 myobjrefqueuelist.h mynumstacklist.h myintqueuelist.h \
 mystringqueuelist.h mydictionaryqueuelist.h mycontentqueuelist.h \
 myScopeHashTable.h myTernarySearchTree.h mydecode.h

mystringqueuelist.o: mystringqueuelist.c mystringqueuelist.h

mycontentqueuelist.o: mycontentqueuelist.c mycontentqueuelist.h \
 mystringqueuelist.h mydictionaryqueuelist.h

mynumstacklist.o: mynumstacklist.c mynumstacklist.h

myintqueuelist.o: myintqueuelist.c myintqueuelist.h

mydecode.o: mydecode.c mydecode.h myoctal.h mypdfsearch.h
scanner.o: scanner.c myoctal.h mypdfsearch.h scanner.h \
 myobjrefqueuelist.h mynumstacklist.h myintqueuelist.h \
 mystringqueuelist.h mydictionaryqueuelist.h mycontentqueuelist.h \
 myScopeHashTable.h myTernarySearchTree.h mydecode.h

parser.o: parser.c parser.h mypdfsearch.h scanner.h myobjrefqueuelist.h \
 mynumstacklist.h myintqueuelist.h mystringqueuelist.h \
 mydictionaryqueuelist.h mycontentqueuelist.h myScopeHashTable.h \
 myTernarySearchTree.h mydecode.h

main.o: main.c mypdfsearch.h parser.h scanner.h myobjrefqueuelist.h \
 mynumstacklist.h myintqueuelist.h mystringqueuelist.h \
 mydictionaryqueuelist.h mycontentqueuelist.h myScopeHashTable.h \
 myTernarySearchTree.h mydecode.h
*/

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

static int MyConsolePasswordCallback(void *pUser,
                                     const char *pdf_filename,
                                     const char *error_msg,
                                     char *out_password,
                                     size_t max_len,
                                     size_t *out_len)
{
    UNUSED(pUser);

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

    fprintf(stderr, "Inserisci la password utente per '%s' (invio a vuoto per annullare): ", pdf_filename);
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

	Params myParams = {0};
	CliOutput cliOutput = { NULL };

	FilesList* myFilesList = NULL;

	#if !defined(_WIN64) && !defined(_WIN32)
	struct stat entryInfo;
	#endif

	//long numProcessors;

	int len;

	#if defined(MYPDFSEARCH_SHOW_TIME)
	myTimes mt_start;
	myTimes mt_end;
	int bTimeStartValid = 0;

	if ( !getTimes(&mt_start) )
	{
		wprintf(L"\nErrore getTimes(&mt_start) in main.c\n\n");
		retValue = EXIT_FAILURE;
		goto uscita;
	}
	bTimeStartValid = 1;
	#endif

	myParams.countWordsToSearch = 0;
	myParams.pWordsToSearchArray = NULL;

	myParams.bOptVersionOrHelp = 0;
	myParams.szPath[0] = '\0';
	myParams.szFilePdf[0] = '\0';
	myParams.fromPage = 0;
	myParams.toPage = 0;
	myParams.bNoSubDirs = 0;
	myParams.nSortGlyphs = SORT_GLYPHS_BY_APPEARANCE;
	myParams.szOutputFile[0] = '\0';
	//myParams.szWordsToSearch[0] = '\0';
	myParams.countWordsToSearch = 0;

	for ( int k = 0; k < MAX_LEN_STR + 1; k++ )
		myParams.szWordsToSearch[k] = '\0';

	myParams.fpOutput = NULL;

	myParams.fpErrors = NULL;

#if defined(_WIN64) || defined(_WIN32)
	//SetConsoleOutputCP(65001);
	//_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stdout), _O_U8TEXT);
#else
	//if ( !setlocale(LC_CTYPE, "it_IT.utf8") )
	if ( !setlocale(LC_ALL, "C.UTF-8") )
	{
		wprintf(L"Impossibile selezionare utf8!\n");
		printf("Impossibile selezionare utf8!\n");
		retValue = EXIT_FAILURE;
		goto uscita;
	}
#endif

	if (!PdfCryptoInitialize())
	{
		fprintf(stderr, "ERRORE: inizializzazione globale OpenSSL fallita.\n");

		retValue = EXIT_FAILURE;
		goto uscita;
	}

	checkEndianness(&myParams);

	if ( !myParseCommandLine(&myParams, argc, argv) )
	{
		retValue = EXIT_FAILURE;
		goto uscita;
	}

	if ( myParams.bOptVersionOrHelp )
	{
		retValue = EXIT_SUCCESS;
		goto uscita;
	}

	if ( myParams.szOutputFile[0] != '\0' )
	{
		if ( !MakeAndOpenOutputFile(&myParams) )
		{
			retValue = EXIT_FAILURE;
			goto uscita;
		}
	}

	if ( myParams.szFilePdf[0] != '\0' )
	{
		#if !defined(_WIN64) && !defined(_WIN32)
		char* psz = NULL;
		int x;
		int y;
		#endif

		myFilesList = (FilesList*)malloc(sizeof(FilesList));

		if( myFilesList == NULL )
		{
			wprintf(L"Errore main: impossibile aggiungere il file alla lista.\n");
			retValue = EXIT_FAILURE;
			goto uscita;
		}

		#if !defined(_WIN64) && !defined(_WIN32)
		strcpy(myFilesList->myPathName, myParams.szFilePdf);
		myFilesList->next = NULL;

		x = strlen(myFilesList->myPathName);
		while ( x >= 0 )
		{
			if ( myFilesList->myPathName[x] == '/' )
			{
				myFilesList->myPathName[x] = '\0';
				break;
			}
			x--;
		}

		if ( x > 0 )
			x++;
		psz = &(myParams.szFilePdf[x]);
		y = 0;
		while ( *psz != '\0' )
			myFilesList->myFileName[y++] = *(psz++);
		myFilesList->myFileName[y] = '\0';
		#else
		strcpy(myFilesList->myFileName, myParams.szFilePdf);
		myFilesList->next = NULL;
		#endif
	}
	else
	{
		#if !defined(_WIN64) && !defined(_WIN32)
		if( lstat(myParams.szPath, &entryInfo) == 0 )
		{
			if( S_ISDIR(entryInfo.st_mode) )		// directory
			{
				len = strnlen(myParams.szPath, MAX_LEN_STR);
				myFilesList = getFilesRecursive(myParams.szPath, len, myParams.bNoSubDirs, myFilesList);
				if ( !myFilesList )
				{
					wprintf(L"Errore: impossibile ottenere la lista dei file.\n");
					retValue = EXIT_FAILURE;
					goto uscita;
				}
			}
			else if( S_ISREG(entryInfo.st_mode) )	// regular file
			{
				char* psz = NULL;
				int x;
				int y;

				myFilesList = (FilesList*)malloc(sizeof(FilesList));

				if( myFilesList == NULL )
				{
					wprintf(L"Errore main: impossibile aggiungere il file alla lista.\n");
					retValue = EXIT_FAILURE;
					goto uscita;
				}

				strcpy(myFilesList->myPathName, myParams.szPath);
				myFilesList->next = NULL;

				x = strlen(myFilesList->myPathName);
				while ( x >= 0 )
				{
					if ( myFilesList->myPathName[x] == '/' )
					{
						myFilesList->myPathName[x] = '\0';
						break;
					}
					x--;
				}

				if ( x > 0 )
					x++;
				psz = &(myParams.szPath[x]);
				y = 0;
				while ( *psz != '\0' )
					myFilesList->myFileName[y++] = *(psz++);
				myFilesList->myFileName[y] = '\0';
			}
		}
		#else
		if ( GetFileAttributesA(myParams.szPath) & FILE_ATTRIBUTE_DIRECTORY )
		{
			len = strnlen(myParams.szPath, MAX_LEN_STR);
			myFilesList = getFilesRecursive(myParams.szPath, len, myParams.bNoSubDirs, myFilesList);
			if ( !myFilesList )
			{
				wprintf(L"Errore: impossibile ottenere la lista dei file.\n");
				retValue = EXIT_FAILURE;
				goto uscita;
			}
		}
		else
		{
			myFilesList = (FilesList*)malloc(sizeof(FilesList));
			if( myFilesList == NULL )
			{
				wprintf(L"Errore main: impossibile aggiungere il file alla lista.\n");
				retValue = EXIT_FAILURE;
				goto uscita;
			}
			strcpy(myFilesList->myFileName, myParams.szPath);
			myFilesList->next = NULL;
		}
		#endif
	}

	wprintf(L"\n");

	SetOnUserPasswordCallback(&myParams, MyConsolePasswordCallback, NULL);

	// Uscita: -e (estrazione del testo) -> testo delle pagine; -p (ricerca) -> parole trovate.
	cliOutput.fpOutput = myParams.fpOutput;
	myParams.pfnFileStart = CliOnFileStart;
	myParams.pFileStartUser = &cliOutput;
	if ( myParams.szFilePdf[0] != '\0' )
	{
		myParams.pfnPageStart = CliOnPageStart;
		myParams.pPageStartUser = &cliOutput;
		myParams.pfnPageText = CliOnPageText;
		myParams.pPageTextUser = &cliOutput;
	}
	else
	{
		myParams.pfnWordFound = CliOnWordFound;
		myParams.pWordFoundUser = &cliOutput;
	}

	//Parse(&myParams, myFilesList, 0);
	if ( !Parse(&myParams, myFilesList) )
		retValue = MYPDFSEARCH_EXIT_FILE_ERRORS;

uscita:

	freeFilesList(myFilesList);

	if ( NULL != myParams.pWordsToSearchArray )
	{
		int myIndex;

		for ( myIndex = 0; myIndex < myParams.countWordsToSearch; myIndex++ )
		{
			if ( NULL != myParams.pWordsToSearchArray[myIndex] )
			{
				free(myParams.pWordsToSearchArray[myIndex]);
				myParams.pWordsToSearchArray[myIndex] = NULL;
			}

		}
		free(myParams.pWordsToSearchArray);
		myParams.pWordsToSearchArray = NULL;
	}

	if ( NULL != myParams.fpErrors )
		fclose(myParams.fpErrors);

	if ( NULL != myParams.fpOutput )
	{
		fclose(myParams.fpOutput);
		myParams.fpOutput = NULL;
	}

    // Rilascio del provider globale OpenSSL.
    // A questo punto tutte le elaborazioni crittografiche
    // devono essere terminate.
    if (!PdfCryptoShutdown())
    {
        fprintf(stderr, "ERRORE: rilascio del provider globale OpenSSL fallito.\n");
        retValue = EXIT_FAILURE;
    }

	#if defined(MYPDFSEARCH_SHOW_TIME)
	if ( !(myParams.bOptVersionOrHelp) && bTimeStartValid )
	{
		double elapsedReal;
		double elapsedUser;
		double elapsedSystem;
		double cpuUsagePercent;

		if ( !getTimes(&mt_end) )
		{
			wprintf(L"\nErrore getTimes(&mt_end) in main.c\n\n");
			retValue = EXIT_FAILURE;
		}
		else
		{
			elapsedReal = mt_end.realTime - mt_start.realTime;
			elapsedUser = mt_end.userTime - mt_start.userTime;
			elapsedSystem = mt_end.systemTime - mt_start.systemTime;

			wprintf(L"\n\n-----------------------------------------------------------------------\n");
			wprintf(L"Tempi di esecuzione (secondi):\n");
			wprintf(L"  Real Time  : %5.5f\n  User Time  : %5.5f\n  System Time: %5.5f\n",
				elapsedReal, elapsedUser, elapsedSystem);

			if (elapsedReal > 0.0)
			{
				cpuUsagePercent = ((elapsedUser + elapsedSystem) / elapsedReal) * 100.0;
				wprintf(L"  CPU media  : %.1f%% (100%% equivale a una CPU logica)\n\n",
					cpuUsagePercent);
			}
			else
			{
				wprintf(L"  CPU media  : non calcolabile (tempo reale nullo)\n\n");
			}

			PrintHardwareInfo();
		}
	}
	#else
	wprintf(L"\n\n");
	#endif

	return retValue;
}
