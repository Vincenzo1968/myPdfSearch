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
#include "scanner.h"
#include "mypdf_utils.h"
#include "mypdf_cli.h"


void PrintHelpCommandLine(void)
{
	wprintf(L"\nUsage:\n");
	wprintf(L"./mypdfsearch Options\n");
	wprintf(L"Options:\n");
	wprintf(L"At least one of:\n\n");
	wprintf(L"'-h, --help' (optional)              Print this message and exit\n");
	wprintf(L"\n");
	wprintf(L"'-v, --version' (optional)           Print version info and exit\n");
	wprintf(L"\n");
	wprintf(L"'-e, --extracttextfrom' (optional)   Extract text from specified file and exit\n");
	wprintf(L"   Arguments for option '-e, --extracttextfrom': exactly 1(string)\n");
	wprintf(L"\n");
	wprintf(L"'-f, --frompage' (optional)          Extract text from pagenum specified\n");
	wprintf(L"   Arguments for option '-f, --frompage': exactly 1(integer)\n");
	wprintf(L"\n");
	wprintf(L"'-t, --topage' (optional)            Extract text to pagenum specified\n");
	wprintf(L"   Arguments for option '-t, --topage': exactly 1(integer)\n");
	wprintf(L"\n");
	wprintf(L"'-p, --path' (optional)              Specifies the input file\n");
	wprintf(L"   Arguments for option '-p, --path': exactly 1(string)\n");
	wprintf(L"\n");
	wprintf(L"'-n, --nosubdirs' (optional)         Specifies that the search should be performed only in the specified directory and not in its subdirectories\n");
	wprintf(L"\n");
	wprintf(L"'-s, --sortglyphsbyrow' (optional)   Select the text based on the row order of the glyphs. If this option is not specified, the text is selected based on the order of appearance.\n");
	wprintf(L"\n");
	wprintf(L"'-w, --words' (optional)             Specifies words to search\n");
	wprintf(L"   Arguments for option '-w, --words': exactly 1(string) containing one on more words to search, separated by space character\n");
	wprintf(L"\n");
	wprintf(L"'-o, --outputfile' (optional)        Specifies the output file\n");
	wprintf(L"   Arguments for option '-o, --outputfile': exactly 1(string)\n");
	wprintf(L"\n");
	wprintf(L"'--show_errors' (optional)           For each file, print on screen (stderr) the number of parsing errors written to the errors log file\n");
	wprintf(L"\n");
	wprintf(L"'--show_warnings' (optional)         For each file, print on screen (stderr) the number of parsing warnings written to the errors log file\n");
	wprintf(L"\n");
}

void PrintVersionInfo(void)
{
	//wprintf(L"\n   mypdfsearch version 1.0.0\n");
	wprintf(L"\n   mypdfsearch version %s\n", VERSION);

	wprintf(L"\n   Copyright (C) 2026 Vincenzo Lo Cicero\n\n");

	wprintf(L"   Author: Vincenzo Lo Cicero.\n");
	wprintf(L"   e-mail: vincenzolocicero68@gmail.com\n\n");

	wprintf(L"   mypdfsearch is free software: you can redistribute it and/or modify\n");
	wprintf(L"   it under the terms of the GNU General Public License as published by\n");
	wprintf(L"   the Free Software Foundation, either version 3 of the License, or\n");
	wprintf(L"   (at your option) any later version.\n\n");

	wprintf(L"   mypdfsearch is distributed in the hope that it will be useful,\n");
	wprintf(L"   but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	wprintf(L"   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
	wprintf(L"   GNU General Public License for more details.\n\n");

	wprintf(L"   You should have received a copy of the GNU General Public License\n");
	wprintf(L"   along with mypdfsearch.  If not, see <http://www.gnu.org/licenses/>.\n");

	wprintf(L"\n");
}

int myParseOption(Params *pParams, char *pszOption)
{
	int retValue = 1;

	int bLongName = 0;
	int bEqualCharFound = 0;

	wchar_t c;
	int x;
	int y;

	int n;

	int nLen;

	wchar_t szName[MAX_LEN_STR + 1];

	wchar_t *pwszOption = NULL;

	szName[0] = '\0';

	x = 0;
	y = 0;
	n = 0;

	//wprintf(L"pszOption = <%s>\n", pszOption);

	n = strnlen(pszOption, MAX_LEN_STR);
	if ( n < 2 )
	{
		retValue = 0;
		wprintf(L"\n\nError: unrecognized option '%s'\n\n", pszOption);
		goto uscita;
	}

	//pwszOption = myConvertToUtf8WideCharString(pszOption, n, "UTF-8");
	pwszOption = myMultibyteToWidechar(pszOption, n);
	if ( NULL == pwszOption )
	{
		retValue = 0;
		wprintf(L"\n\nError myParseOption: myConvertToUtf8WideCharString return NULL for '%s'\n\n", pszOption);
		goto uscita;
	}

	n = 0;


	x = 0;
	c = pwszOption[x];
	if ( c != L'-' )
	{
		retValue = 0;
		wprintf(L"\n\nError: invalid option '%s'\n\n", pszOption);
		goto uscita;
	}

	x = 1;
	c = pwszOption[x];
	if ( c == L'-' )
	{
		bLongName = 1;
		x++;
		c = pwszOption[x];
		szName[0] = c;
		szName[1] = L'\0';
		y = 1;
		goto readlongname;
	}
	else if ( c == L'\0' )
	{
		retValue = 0;
		wprintf(L"\n\nError: invalid option '%s'\n\n", pszOption);
		goto uscita;
	}
	else
	{
		szName[0] = c;
		szName[1] = '\0';
		x++;

		if ( c == L'h' )
		{
			pParams->bOptVersionOrHelp = 1;
			PrintHelpCommandLine();
			goto uscita;
		}
		else if ( c == L'v' )
		{
			pParams->bOptVersionOrHelp = 1;
			PrintVersionInfo();
			goto uscita;
		}
		else if ( c == L'n' )
		{
			pParams->bNoSubDirs = 1;
			goto uscita;
		}
		else if ( c == L's' )
		{
			pParams->nSortGlyphs = SORT_GLYPHS_BY_ROW;
			goto uscita;
		}

		goto readvalue;
	}

	readlongname:

	if ( bLongName )
	{
		c = pwszOption[x];

		x++;
		c = pwszOption[x];
		while ( c != L'\0' )
		{
			szName[y] = c;
			y++;

			if ( y > MAX_LEN_STR )
			{
				retValue = 0;
				wprintf(L"Error: option string too long.\n");
				goto uscita;
			}

			if ( c == L'=' )
			{
				y--;
				break;
			}

			x++;
			c = pwszOption[x];
		}

		if ( y < 0 )
		{
			retValue = 0;
			wprintf(L"\n\nError: invalid option '%s'\n\n", pszOption);
			goto uscita;
		}

		szName[y] = L'\0';


		if ( wcsncmp(szName, L"path", MAX_LEN_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"words", MAX_LEN_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"nosubdirs", MAX_LEN_STR) == 0 )
		{
			pParams->bNoSubDirs = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"sortglyphsbyrow", MAX_LEN_STR) == 0 )
		{
			pParams->nSortGlyphs = SORT_GLYPHS_BY_ROW;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"show_errors", MAX_LEN_STR) == 0 )
		{
			pParams->bShowErrors = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"show_warnings", MAX_LEN_STR) == 0 )
		{
			pParams->bShowWarnings = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"extracttextfrom", MAX_LEN_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"frompage", MAX_LEN_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"topage", MAX_LEN_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"outputfile", MAX_LEN_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"nosubdirs", MAX_LEN_STR) == 0 )
		{
			pParams->bNoSubDirs = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"sortglyphsbyrow", MAX_LEN_STR) == 0 )
		{
			pParams->nSortGlyphs = SORT_GLYPHS_BY_ROW;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"version", MAX_LEN_STR) == 0 )
		{
			pParams->bOptVersionOrHelp = 1;
			PrintVersionInfo();
			goto uscita;
		}
		else if ( wcsncmp(szName, L"help", MAX_LEN_STR) == 0 )
		{
			pParams->bOptVersionOrHelp = 1;
			PrintHelpCommandLine();
			goto uscita;
		}
		else
		{
			retValue = 0;
			wprintf(L"\n\nError: unrecognized option '%s'\n\n", pszOption);
			goto uscita;
		}
	}

	readvalue:

	c = szName[0];
	y = 0;
	switch ( c )
	{
		case L'p':   // path
			c = pwszOption[x];
			while ( c != L'\0' )
			{
				nLen = wctomb(&(pParams->szPath[y]), c);
				y += nLen;
				//pParams->szPath[y] = (char)c;
				//y++;
				if ( c == L'=' )
				{
					y = 0;
					pParams->szPath[0] = '\0';
				}
				x++;
				c = pwszOption[x];
			}
			pParams->szPath[y] = '\0';
			break;
		case L'w':   // words
			c = pwszOption[x];
			while ( c != L'\0' )
			{
				pParams->szWordsToSearch[y] = c;
				y++;
				if ( c == L'=' )
				{
					y = 0;
					pParams->szWordsToSearch[0] = L'\0';
				}
				x++;
				c = pwszOption[x];
			}
			pParams->szWordsToSearch[y] = '\0';

			mySplitWords(pParams, pParams->szWordsToSearch);

			break;
		case L'e':   // extracttextfrom
			c = pwszOption[x];
			while ( c != '\0' )
			{
				nLen = wctomb(&(pParams->szFilePdf[y]), c);
				y += nLen;
				//pParams->szFilePdf[y] = (char)c;
				//y++;
				if ( c == L'=' )
				{
					y = 0;
					pParams->szFilePdf[0] = '\0';
				}
				x++;
				c = pwszOption[x];
			}
			pParams->szFilePdf[y] = '\0';
			break;
		case 'f':   // frompage
			n = 0;
			c = pwszOption[x];
			while ( c != L'\0' )
			{
				if ( c >= L'0' && c <= L'9' )
				{
					n = n * 10 + c - L'0';
					x++;
					c = pwszOption[x];
				}
				else if ( c == L'=' )
				{
					bEqualCharFound = 1;
					x++;
					c = pwszOption[x];
				}
				else
				{
					if ( bEqualCharFound )
					{
						retValue = 0;
						wprintf(L"\n\nError: invalid option value -> '%s'. Must be a integer number\n\n", pszOption);
						goto uscita;
					}

					x++;
					c = pwszOption[x];
				}
			}
			pParams->fromPage = n;
			break;
		case L't':   // topage
			n = 0;
			c = pwszOption[x];
			while ( c != L'\0' )
			{
				if ( c >= L'0' && c <= L'9' )
				{
					n = n * 10 + c - L'0';
					x++;
					c = pwszOption[x];
				}
				else if ( c == L'=' )
				{
					bEqualCharFound = 1;
					x++;
					c = pwszOption[x];
				}
				else
				{
					if ( bEqualCharFound )
					{
						retValue = 0;
						wprintf(L"\n\nError: invalid option value -> '%s'. Must be a integer number\n\n", pszOption);
						goto uscita;
					}

					x++;
					c = pszOption[x];
				}
			}
			pParams->toPage = n;
			break;
		case L'o':   // outputfile
			c = pwszOption[x];
			while ( c != L'\0' )
			{
				nLen = wctomb(&(pParams->szOutputFile[y]), c);
				y += nLen;
				//pParams->szOutputFile[y] = (char)c;
				//y++;
				if ( c == L'=' )
				{
					y = 0;
					pParams->szOutputFile[0] = '\0';
				}
				x++;
				c = pwszOption[x];
			}
			pParams->szOutputFile[y] = '\0';
			break;
		default:
			retValue = 0;
			wprintf(L"\n\nError: unrecognized option '%s'\n\n", pszOption);
			goto uscita;
			break;
	}

	uscita:

	if ( NULL != pwszOption )
	{
		free(pwszOption);
		pwszOption = NULL;
	}

	return retValue;
}

int myParseCommandLine(Params *pParams, int argc, char **argv)
{
	int retValue = 1;

	int x;
	int n;

	pParams->bOptVersionOrHelp = 0;
	pParams->szPath[0] = '\0';
	pParams->szFilePdf[0] = '\0';
	pParams->fromPage = 0;
	pParams->toPage = 0;
	pParams->szOutputFile[0] = '\0';
	pParams->szWordsToSearch[0] = '\0';
	pParams->countWordsToSearch = 0;

	pParams->pWordsToSearchArray = NULL;


	pParams->pWordsToSearchArray = (wchar_t**)malloc(sizeof(wchar_t*) * MAX_WORDS_TO_SEARCH);
	if ( NULL == pParams->pWordsToSearchArray )
	{
		retValue = 0;
		wprintf(L"\n\nError myParseOption: memory allocation failed for pWordsToSearchArray\n\n");
		goto uscita;
	}
	for ( n = 0; n < MAX_WORDS_TO_SEARCH; n++ )
	{
		pParams->pWordsToSearchArray[n] = NULL;
	}

	x = 1;
	if ( argc > 1 )
	{
		while ( x < argc )
		{
			retValue = myParseOption(pParams, argv[x]);
			if ( !retValue )
				goto uscita;
			x++;
		}
	}
	else
	{
		retValue = 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->szFilePdf[0] == '\0' && pParams->szPath[0] == '\0') )
	{
		wprintf(L"\n\nErrore: dev'essere specificata una(e solo una, non entrambe) delle due opzioni: '-p (--path)' o '-e(--extracttextfrom)'\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->szFilePdf[0] != '\0' && pParams->szPath[0] != '\0') )
	{
		wprintf(L"\n\nErrore: dev'essere specificata soltanto una delle due opzioni mutualmente esclusive: '-p (--path)' o '-e(--extracttextfrom)'\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->toPage > 0 && pParams->toPage < pParams->fromPage) )
	{
		wprintf(L"\n\nErrore: il valore specificato per l'opzione topage e' minore del valore specificato per l'opzione frompage. Dev'essere maggiore o uguale.\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->szPath[0] != '\0' && pParams->szWordsToSearch[0] == '\0') )
	{
		wprintf(L"\n\nErrore: bisogna specificare le parole da cercare tramite l'opzione '-w (--words)'\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->szPath[0] == '\0' && pParams->szWordsToSearch[0] != '\0') )
	{
		wprintf(L"\n\nErrore: bisogna specificare la cartella contenente i file pdf tramite l'opzione '-p (--path)'\n\n");
		return 0;
	}

uscita:

	return retValue;
}

int MakeAndOpenOutputFile(Params *pParams)
{
	int retValue = 1;

	#if !defined(_WIN64) && !defined(_WIN32)
	unsigned char szBOM[21];
	// UTF-8 BOM -> EF BB BF
	szBOM[0] = 0xEF;
	szBOM[1] = 0xBB;
	szBOM[2] = 0xBF;
	szBOM[3] = '\0';
	#endif

	pParams->fpOutput = fopen(pParams->szOutputFile, "rb");
	if ( pParams->fpOutput != NULL )
	{
		wprintf(L"\n\nERRORE: il file specificato per l'output esiste già.\nBisogna indicare il nome di un file non esistente.\n\n");
		retValue = 0;
		goto uscita;
	}

	pParams->fpOutput = fopen(pParams->szOutputFile, "wb");
	if ( pParams->fpOutput == NULL )
	{
		wprintf(L"\n\nERRORE: impossibile creara il file specificato per l'output: '%s'.\n\n", pParams->szOutputFile);
		retValue = 0;
		goto uscita;
	}

	#if defined(_WIN64) || defined(_WIN32)
	_setmode(_fileno(pParams->fpOutput), _O_U8TEXT);
	#endif

	#if !defined(_WIN64) && !defined(_WIN32)
	// UTF-8 BOM -> EF BB BF
	fwprintf(pParams->fpOutput, L"%s", szBOM);
	#endif

	uscita:

	return retValue;
}

#if defined(__linux__)
/* Legge una riga da sysfs e rimuove i terminatori. */
static int ReadSysfsText(const char *path, char *buffer, size_t bufferSize)
{
    FILE *fp;
    size_t len;

    if (path == NULL || buffer == NULL || bufferSize == 0)
        return 0;

    buffer[0] = '\0';
    fp = fopen(path, "r");
    if (fp == NULL)
        return 0;

    if (fgets(buffer, (int)bufferSize, fp) == NULL)
    {
        fclose(fp);
        buffer[0] = '\0';
        return 0;
    }

    fclose(fp);

    len = strcspn(buffer, "\r\n");
    buffer[len] = '\0';

    while (len > 0 && (buffer[len - 1] == ' ' || buffer[len - 1] == '\t'))
        buffer[--len] = '\0';

    return len > 0;
}

static const char *LinuxStorageTransport(const char *devicePath)
{
    char resolvedPath[PATH_MAX];

    if (devicePath == NULL || realpath(devicePath, resolvedPath) == NULL)
        return "non rilevato";

    if (strstr(resolvedPath, "/nvme") != NULL)
        return "NVMe";
    if (strstr(resolvedPath, "/ata") != NULL)
        return "SATA/ATA";
    if (strstr(resolvedPath, "/usb") != NULL)
        return "USB";
    if (strstr(resolvedPath, "/mmc") != NULL)
        return "MMC/SD";
    if (strstr(resolvedPath, "/virtio") != NULL)
        return "VirtIO";
    if (strstr(resolvedPath, "/sas") != NULL)
        return "SAS";

    return "non identificato";
}

static int IsVirtualBlockDeviceName(const char *name)
{
    return strncmp(name, "loop", 4) == 0 ||
           strncmp(name, "ram", 3) == 0 ||
           strncmp(name, "dm-", 3) == 0 ||
           strncmp(name, "md", 2) == 0 ||
           strncmp(name, "zram", 4) == 0;
}

static void PrintLinuxStorageInfo(void)
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    wprintf(L"\nUnità di memorizzazione (informazioni sysfs):\n");

    dir = opendir("/sys/block");
    if (dir == NULL)
    {
        wprintf(L"  Informazioni sui dispositivi non disponibili.\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char path[PATH_MAX];
        char model[256] = "";
        char vendor[128] = "";
        char sizeText[64] = "";
        char rotationalText[16] = "";
        char devicePath[PATH_MAX];
        char *endPtr;
        unsigned long long sectors = 0;
        int rotational = -1;
        double sizeGiB;

        if (entry->d_name[0] == '.' || IsVirtualBlockDeviceName(entry->d_name))
            continue;

        (void)snprintf(path, sizeof(path), "/sys/block/%s/size", entry->d_name);
        if (!ReadSysfsText(path, sizeText, sizeof(sizeText)))
            continue;

        errno = 0;
        sectors = strtoull(sizeText, &endPtr, 10);
        if (errno != 0 || endPtr == sizeText)
            sectors = 0;

        (void)snprintf(path, sizeof(path), "/sys/block/%s/device/model", entry->d_name);
        (void)ReadSysfsText(path, model, sizeof(model));
        (void)snprintf(path, sizeof(path), "/sys/block/%s/device/vendor", entry->d_name);
        (void)ReadSysfsText(path, vendor, sizeof(vendor));
        (void)snprintf(path, sizeof(path), "/sys/block/%s/queue/rotational", entry->d_name);
        if (ReadSysfsText(path, rotationalText, sizeof(rotationalText)))
            rotational = atoi(rotationalText);

        (void)snprintf(devicePath, sizeof(devicePath), "/sys/block/%s/device", entry->d_name);
        sizeGiB = ((double)sectors * 512.0) / (1024.0 * 1024.0 * 1024.0);

        wprintf(L"  Dispositivo        : /dev/%hs\n", entry->d_name);
        if (vendor[0] != '\0')
            wprintf(L"    Produttore       : %hs\n", vendor);
        if (model[0] != '\0')
            wprintf(L"    Modello          : %hs\n", model);
        wprintf(L"    Interfaccia      : %hs\n", LinuxStorageTransport(devicePath));

        if (sectors == 0)
            wprintf(L"    Capacità         : non disponibile (nessun supporto o capacità zero)\n");
        else
            wprintf(L"    Capacità         : %.2f GiB\n", sizeGiB);

        if (strncmp(entry->d_name, "sr", 2) == 0)
            wprintf(L"    Tipo             : unità ottica\n");
        else if (rotational == 1)
            wprintf(L"    Supporto         : rotazionale (il kernel segnala ROTA=1; tipico HDD)\n");
        else if (rotational == 0)
            wprintf(L"    Supporto         : non rotazionale (SSD/flash o lettore)\n");
        else
            wprintf(L"    Supporto         : non determinato\n");

        count++;
    }

    closedir(dir);

    if (count == 0)
        wprintf(L"  Nessuna unità fisica rilevata.\n");
}

static const char *PciVendorName(unsigned long vendorId)
{
    switch (vendorId)
    {
        case 0x1002: return "AMD/ATI";
        case 0x8086: return "Intel";
        case 0x10DE: return "NVIDIA";
        case 0x1022: return "AMD";
        case 0x1000: return "Broadcom/LSI";
        default: return "produttore non mappato";
    }
}

static void PrintLinuxGraphicsInfo(void)
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    wprintf(L"\nGPU e controller grafici (PCI/sysfs):\n");

    dir = opendir("/sys/bus/pci/devices");
    if (dir == NULL)
    {
        wprintf(L"  Informazioni PCI non disponibili.\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char path[PATH_MAX];
        char classText[32] = "";
        char vendorText[32] = "";
        char deviceText[32] = "";
        char subVendorText[32] = "";
        char subDeviceText[32] = "";
        char driverPath[PATH_MAX];
        char driverName[128] = "non rilevato";
        unsigned long classCode;
        unsigned long vendorId;
        unsigned long deviceId;
        unsigned long subVendorId = 0;
        unsigned long subDeviceId = 0;
        ssize_t linkLength;
        char *slash;

        if (entry->d_name[0] == '.')
            continue;

        (void)snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/class", entry->d_name);
        if (!ReadSysfsText(path, classText, sizeof(classText)))
            continue;

        classCode = strtoul(classText, NULL, 0);
        if (((classCode >> 16) & 0xFFUL) != 0x03UL)
            continue;

        (void)snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/vendor", entry->d_name);
        if (!ReadSysfsText(path, vendorText, sizeof(vendorText)))
            continue;
        (void)snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/device", entry->d_name);
        if (!ReadSysfsText(path, deviceText, sizeof(deviceText)))
            continue;
        (void)snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/subsystem_vendor", entry->d_name);
        (void)ReadSysfsText(path, subVendorText, sizeof(subVendorText));
        (void)snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/subsystem_device", entry->d_name);
        (void)ReadSysfsText(path, subDeviceText, sizeof(subDeviceText));

        vendorId = strtoul(vendorText, NULL, 0);
        deviceId = strtoul(deviceText, NULL, 0);
        if (subVendorText[0] != '\0')
            subVendorId = strtoul(subVendorText, NULL, 0);
        if (subDeviceText[0] != '\0')
            subDeviceId = strtoul(subDeviceText, NULL, 0);

        (void)snprintf(driverPath, sizeof(driverPath), "/sys/bus/pci/devices/%s/driver", entry->d_name);
        linkLength = readlink(driverPath, driverName, sizeof(driverName) - 1);
        if (linkLength >= 0)
        {
            driverName[linkLength] = '\0';
            slash = strrchr(driverName, '/');
            if (slash != NULL)
                memmove(driverName, slash + 1, strlen(slash + 1) + 1);
        }

        wprintf(L"  PCI address       : %hs\n", entry->d_name);
        wprintf(L"    Classe          : ");
        if (((classCode >> 8) & 0xFFUL) == 0x00UL)
            wprintf(L"controller VGA\n");
        else if (((classCode >> 8) & 0xFFUL) == 0x02UL)
            wprintf(L"controller 3D\n");
        else
            wprintf(L"controller display\n");
        wprintf(L"    Produttore      : %hs (PCI %04lx)\n", PciVendorName(vendorId), vendorId);
        wprintf(L"    Device ID       : %04lx\n", deviceId);
        if (vendorId == 0x1002 && deviceId == 0x675D)
            wprintf(L"    Modello noto    : Turks PRO (Radeon HD 7570)\n");
        if (subVendorText[0] != '\0' && subDeviceText[0] != '\0')
            wprintf(L"    Subsystem ID    : %04lx:%04lx\n", subVendorId, subDeviceId);
        wprintf(L"    Driver kernel   : %hs\n", driverName);
        count++;
    }

    closedir(dir);

    if (count == 0)
        wprintf(L"  Nessun controller grafico PCI rilevato.\n");
}
#endif // __linux__ 

void PrintHardwareInfo(void)
{
    wprintf(L"Informazioni hardware e software:\n");

    // Informazioni sul compilatore e sulla configurazione di build. 
#if defined(__clang__)
    wprintf(L"  Compilatore        : Clang %hs\n", __VERSION__);
#elif defined(__GNUC__)
    wprintf(L"  Compilatore        : GCC %hs\n", __VERSION__);
#elif defined(_MSC_VER)
    wprintf(L"  Compilatore        : Microsoft Visual C++ (%d)\n", _MSC_VER);
#else
    wprintf(L"  Compilatore        : non identificato\n");
#endif

#if defined(MYPDFSEARCH_BUILD_DEBUG)
    wprintf(L"  Build              : DEBUG (-O0 -g)\n");
#elif defined(MYPDFSEARCH_BUILD_RELEASE)
    wprintf(L"  Build              : RELEASE (-O3)\n");
#elif defined(__OPTIMIZE__)
    wprintf(L"  Ottimizzazione     : attiva (livello %d)\n", __OPTIMIZE__);
#else
    wprintf(L"  Ottimizzazione     : non rilevata/disattivata\n");
#endif

#if defined(__STDC_VERSION__)
#if __STDC_VERSION__ >= 201710L
#ifdef __STRICT_ANSI__
    wprintf(L"  Standard C         : C17\n");
#else
    wprintf(L"  Standard C         : GNU C17\n");
#endif
#elif __STDC_VERSION__ >= 201112L
#ifdef __STRICT_ANSI__
    wprintf(L"  Standard C         : C11\n");
#else
    wprintf(L"  Standard C         : GNU C11\n");
#endif
#elif __STDC_VERSION__ >= 199901L
#ifdef __STRICT_ANSI__
    wprintf(L"  Standard C         : C99\n");
#else
    wprintf(L"  Standard C         : GNU C99\n");
#endif
#else
    wprintf(L"  Standard C         : versione precedente a C99\n");
#endif
#endif

#if defined(_WIN64) || defined(_WIN32)
    SYSTEM_INFO sysInfo;
    MEMORYSTATUSEX memStatus;

    GetNativeSystemInfo(&sysInfo);
    wprintf(L"\nHardware:\n");
    wprintf(L"  CPU logiche online : %u\n", sysInfo.dwNumberOfProcessors);
    wprintf(L"  Architettura OS    : ");
    if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        wprintf(L"x64\n");
    else if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        wprintf(L"x86\n");
    else
        wprintf(L"Sconosciuta\n");
    wprintf(L"  Architettura build : %ld-bit\n", (long)(sizeof(void *) * 8));

    // Il nome CPU viene letto dal Registro di sistema. 
    {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                          0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            wchar_t cpuName[256] = {0};
            DWORD bufferSize = sizeof(cpuName);
            if (RegQueryValueExW(hKey, L"ProcessorNameString", NULL, NULL,
                                 (LPBYTE)cpuName, &bufferSize) == ERROR_SUCCESS)
            {
                wprintf(L"  CPU                : %ls\n", cpuName);
            }
            RegCloseKey(hKey);
        }
    }

    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus))
    {
        double totalRamGiB = (double)memStatus.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
        double availRamGiB = (double)memStatus.ullAvailPhys / (1024.0 * 1024.0 * 1024.0);
        wprintf(L"  RAM totale         : %.2f GiB\n", totalRamGiB);
        wprintf(L"  RAM disponibile    : %.2f GiB (utilizzo sistema: %lu%%)\n",
                availRamGiB, memStatus.dwMemoryLoad);
    }


#else
    // Informazioni sul sistema operativo e sul kernel. 
    struct utsname osInfo;
    if (uname(&osInfo) == 0)
    {
        wprintf(L"\nSistema:\n");
        wprintf(L"  Sistema operativo : %hs\n", osInfo.sysname);
        wprintf(L"  Kernel             : %hs\n", osInfo.release);
        wprintf(L"  Architettura HW    : %hs\n", osInfo.machine);
    }

    // CPU logiche visibili al sistema. 
    {
        long numProcessors = sysconf(_SC_NPROCESSORS_ONLN);
        if (numProcessors > 0)
            wprintf(L"\nHardware:\n  CPU logiche online : %ld\n", numProcessors);
        else
            wprintf(L"\nHardware:\n  CPU logiche online : non rilevate\n");
    }

    wprintf(L"  Architettura build : %ld-bit\n", (long)(sizeof(void *) * 8));

    // CPU logiche consentite al processo secondo la CPU affinity. 
#if defined(__linux__)
    {
        cpu_set_t cpuSet;
        CPU_ZERO(&cpuSet);
        if (sched_getaffinity(0, sizeof(cpuSet), &cpuSet) == 0)
        {
            int allowedCpus = CPU_COUNT(&cpuSet);
            if (allowedCpus > 0)
                wprintf(L"  CPU consentite     : %d\n", allowedCpus);
        }
    }
#endif

    // Modello CPU letto da /proc/cpuinfo. 
    {
        FILE *fpCpu = fopen("/proc/cpuinfo", "r");
        if (fpCpu != NULL)
        {
            char line[512];
            while (fgets(line, sizeof(line), fpCpu) != NULL)
            {
                if (strncmp(line, "model name", 10) == 0)
                {
                    char *colon = strchr(line, ':');
                    if (colon != NULL)
                    {
                        char *cpuName = colon + 1;
                        size_t len;
                        while (*cpuName == ' ' || *cpuName == '\t')
                            cpuName++;
                        len = strlen(cpuName);
                        while (len > 0 && (cpuName[len - 1] == '\n' || cpuName[len - 1] == '\r'))
                            cpuName[--len] = '\0';
                        wprintf(L"  CPU                : %hs\n", cpuName);
                    }
                    break;
                }
            }
            fclose(fpCpu);
        }
    }

    // Frequenza massima dichiarata dal driver cpufreq (non frequenza istantanea). 
    {
        FILE *fpFreq = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
        if (fpFreq != NULL)
        {
            unsigned long maxFreqKHz;
            if (fscanf(fpFreq, "%lu", &maxFreqKHz) == 1)
                wprintf(L"  Frequenza max CPU  : %.2f GHz (valore dichiarato)\n",
                        (double)maxFreqKHz / 1000000.0);
            fclose(fpFreq);
        }
    }

    // Memoria complessiva del sistema: fotografia al termine del test. 
    {
        FILE *fpMem = fopen("/proc/meminfo", "r");
        if (fpMem != NULL)
        {
            char line[256];
            unsigned long long memTotal = 0;
            unsigned long long memAvailable = 0;

            while (fgets(line, sizeof(line), fpMem) != NULL)
            {
                if (strncmp(line, "MemTotal:", 9) == 0)
                    (void)sscanf(line, "MemTotal: %llu", &memTotal);
                else if (strncmp(line, "MemAvailable:", 13) == 0)
                    (void)sscanf(line, "MemAvailable: %llu", &memAvailable);

                if (memTotal > 0 && memAvailable > 0)
                    break;
            }
            fclose(fpMem);

            if (memTotal > 0)
            {
                double totalRamGiB = (double)memTotal / (1024.0 * 1024.0);
                double availRamGiB = (double)memAvailable / (1024.0 * 1024.0);
                double usedPercent = 0.0;
                if (memAvailable <= memTotal)
                    usedPercent = ((double)(memTotal - memAvailable) / (double)memTotal) * 100.0;

                wprintf(L"  RAM totale         : %.2f GiB\n", totalRamGiB);
                wprintf(L"  RAM disponibile    : %.2f GiB (utilizzo sistema stimato: %.1f%%)\n",
                        availRamGiB, usedPercent);
            }
        }
    }

    // Picco di memoria residente (RSS) del solo processo corrente. 
    {
        struct rusage processUsage;
        if (getrusage(RUSAGE_SELF, &processUsage) == 0)
        {
            // Linux esprime ru_maxrss in KiB; macOS lo esprime in byte. 
#if defined(__APPLE__)
            wprintf(L"  Picco RSS processo : %.2f MiB\n",
                    (double)processUsage.ru_maxrss / (1024.0 * 1024.0));
#else
            wprintf(L"  Picco RSS processo : %.2f MiB\n",
                    (double)processUsage.ru_maxrss / 1024.0);
#endif
        }
    }

#if defined(__linux__)
    PrintLinuxStorageInfo();
    PrintLinuxGraphicsInfo();
#endif
#endif

// I dati sui worker saranno aggiunti quando verrà introdotto il thread pool.
// Non stampiamo il numero di thread attivi a fine main(): a quel punto
// eventuali worker sono già stati terminati e uniti.
}


/* ------------------------------------------------------------------------------------------------ */
/* Callback di uscita (stesso output di prima: con -o il testo va sia nel file sia a video,          */
/* intestazioni e "Key found" vanno nel file se c'e' -o, altrimenti a video).                          */
/* ------------------------------------------------------------------------------------------------ */

void MYPDF_CALL CliOnFileStart(void *pUser, const char *pszFile)
{
	CliOutput *pOut = (CliOutput*)pUser;
	FILE *fp = (NULL != pOut) ? pOut->fpOutput : NULL;

	#if !defined(_WIN64) && !defined(_WIN32)
	if ( NULL != fp )
		fwprintf(fp, L"File: '%s'\n", pszFile);
	else
		wprintf(L"File: '%s'\n", pszFile);
	#else
	{
		size_t lung = strnlen(pszFile, PATH_MAX);
		size_t idx;

		if ( NULL != fp )
		{
			fwprintf(fp, L"FILE = <");
			for ( idx = 0; idx < lung; idx++ )
				fwprintf(fp, L"%c", pszFile[idx]);
			fwprintf(fp, L">\n");
		}
		else
		{
			wprintf(L"FILE = <");
			for ( idx = 0; idx < lung; idx++ )
				wprintf(L"%c", pszFile[idx]);
			wprintf(L">\n");
		}
	}
	#endif
}

void MYPDF_CALL CliOnPageStart(void *pUser, const char *pszFile, int32_t nPage)
{
	CliOutput *pOut = (CliOutput*)pUser;
	FILE *fp = (NULL != pOut) ? pOut->fpOutput : NULL;

	UNUSED(pszFile);

	if ( NULL != fp )
		fwprintf(fp, L"\n\nTEXT EXTRACTED FROM PAGE %d:\n\n", (int)nPage);
	else
		wprintf(L"\n\nTEXT EXTRACTED FROM PAGE %d:\n\n", (int)nPage);
}

int MYPDF_CALL CliOnPageText(void *pUser, const char *pszFile, int32_t nPage, const char *pszText, size_t nLen)
{
	CliOutput *pOut = (CliOutput*)pUser;
	FILE *fp = (NULL != pOut) ? pOut->fpOutput : NULL;

	UNUSED(pszFile);
	UNUSED(nPage);
	UNUSED(nLen);

	if ( NULL != fp )
		fwprintf(fp, L"%s", pszText);
	wprintf(L"%s", pszText);

	return 0;
}

int MYPDF_CALL CliOnWordFound(void *pUser, const char *pszFile, int32_t nPage, const char *pszWord)
{
	CliOutput *pOut = (CliOutput*)pUser;
	FILE *fp = (NULL != pOut) ? pOut->fpOutput : NULL;

	UNUSED(pszFile);

	if ( NULL != fp )
		fwprintf(fp, L"\tKey '%s' found on page %d\n", pszWord, (int)nPage);
	else
		wprintf(L"\tKey '%s' found on page %d\n", pszWord, (int)nPage);

	return 0;
}
