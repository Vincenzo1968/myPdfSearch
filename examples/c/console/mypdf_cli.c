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
	wprintf(L"'--prefix' (optional)                Partial search: with '-w', find every word of the text that STARTS WITH one of the specified words\n");
	wprintf(L"\n");
	wprintf(L"'--suffix' (optional)                Partial search: with '-w', find every word of the text that ENDS WITH one of the specified words\n");
	wprintf(L"\n");
	wprintf(L"'--contains' (optional)              Partial search: with '-w', find every word of the text that CONTAINS one of the specified words\n");
	wprintf(L"   '--prefix', '--suffix' and '--contains' are mutually exclusive. The words found are printed as they appear in the text (lowercase).\n");
	wprintf(L"\n");
	wprintf(L"'--show_errors' (optional)           For each file, print on screen (stderr) the number of parsing errors written to the errors log file\n");
	wprintf(L"\n");
	wprintf(L"'--show_warnings' (optional)         For each file, print on screen (stderr) the number of parsing warnings written to the errors log file\n");
	wprintf(L"\n");
}

void PrintVersionInfo(void)
{
	//wprintf(L"\n   mypdfsearch version 1.0.0\n");
	wprintf(L"\n   mypdfsearch version %s\n", mypdf_GetVersion());

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

// Aggiunge il carattere wide 'c' (convertito in UTF-8 dalla locale corrente) a dst[*pLen]; 0 se non converte o se non c'e' spazio.
static int CliAppendWide(char *dst, size_t cap, int *pLen, wchar_t c)
{
	char tmp[MB_LEN_MAX + 1];
	int nLen = wctomb(tmp, c);

	if ( nLen < 0 || (size_t)(*pLen + nLen) + 1 > cap )
		return 0;

	memcpy(dst + *pLen, tmp, (size_t)nLen);
	*pLen += nLen;
	dst[*pLen] = '\0';

	return 1;
}

int myParseOption(CliOptions *pParams, char *pszOption)
{
	int retValue = 1;

	int bLongName = 0;
	int bEqualCharFound = 0;

	wchar_t c;
	int x;
	int y;

	int n;

	wchar_t szName[CLI_MAX_STR + 1];

	wchar_t *pwszOption = NULL;

	szName[0] = '\0';

	x = 0;
	y = 0;
	n = 0;

	//wprintf(L"pszOption = <%s>\n", pszOption);

	n = strnlen(pszOption, CLI_MAX_STR);
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
			pParams->bSortGlyphs = 1;
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

			if ( y > CLI_MAX_STR )
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


		if ( wcsncmp(szName, L"path", CLI_MAX_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"words", CLI_MAX_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"nosubdirs", CLI_MAX_STR) == 0 )
		{
			pParams->bNoSubDirs = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"sortglyphsbyrow", CLI_MAX_STR) == 0 )
		{
			pParams->bSortGlyphs = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"prefix", CLI_MAX_STR) == 0 )
		{
			pParams->bPrefix = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"suffix", CLI_MAX_STR) == 0 )
		{
			pParams->bSuffix = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"contains", CLI_MAX_STR) == 0 )
		{
			pParams->bContains = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"show_errors", CLI_MAX_STR) == 0 )
		{
			pParams->bShowErrors = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"show_warnings", CLI_MAX_STR) == 0 )
		{
			pParams->bShowWarnings = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"extracttextfrom", CLI_MAX_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"frompage", CLI_MAX_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"topage", CLI_MAX_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"outputfile", CLI_MAX_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"nosubdirs", CLI_MAX_STR) == 0 )
		{
			pParams->bNoSubDirs = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"sortglyphsbyrow", CLI_MAX_STR) == 0 )
		{
			pParams->bSortGlyphs = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"version", CLI_MAX_STR) == 0 )
		{
			pParams->bOptVersionOrHelp = 1;
			PrintVersionInfo();
			goto uscita;
		}
		else if ( wcsncmp(szName, L"help", CLI_MAX_STR) == 0 )
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
				if ( !CliAppendWide(pParams->szPath, sizeof(pParams->szPath), &y, c) )
				{
					retValue = 0;
					wprintf(L"\n\nError: invalid or too long value in option '%s'\n\n", pszOption);
					goto uscita;
				}
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
				if ( !CliAppendWide(pParams->szWords, sizeof(pParams->szWords), &y, c) )
				{
					retValue = 0;
					wprintf(L"\n\nError: invalid or too long value in option '%s'\n\n", pszOption);
					goto uscita;
				}
				if ( c == L'=' )
				{
					y = 0;
					pParams->szWords[0] = '\0';
				}
				x++;
				c = pwszOption[x];
			}
			pParams->szWords[y] = '\0';

			break;
		case L'e':   // extracttextfrom
			c = pwszOption[x];
			while ( c != '\0' )
			{
				if ( !CliAppendWide(pParams->szFilePdf, sizeof(pParams->szFilePdf), &y, c) )
				{
					retValue = 0;
					wprintf(L"\n\nError: invalid or too long value in option '%s'\n\n", pszOption);
					goto uscita;
				}
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
				if ( !CliAppendWide(pParams->szOutputFile, sizeof(pParams->szOutputFile), &y, c) )
				{
					retValue = 0;
					wprintf(L"\n\nError: invalid or too long value in option '%s'\n\n", pszOption);
					goto uscita;
				}
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

int myParseCommandLine(CliOptions *pParams, int argc, char **argv)
{
	int retValue = 1;

	int x;

	memset(pParams, 0, sizeof(*pParams));

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
		wprintf(L"\n\nError: exactly one (not both) of the two options must be specified: '-p (--path)' or '-e(--extracttextfrom)'\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->szFilePdf[0] != '\0' && pParams->szPath[0] != '\0') )
	{
		wprintf(L"\n\nError: only one of the two mutually exclusive options must be specified: '-p (--path)' or '-e(--extracttextfrom)'\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->toPage > 0 && pParams->toPage < pParams->fromPage) )
	{
		wprintf(L"\n\nError: the value specified for the topage option is less than the value specified for the frompage option. It must be greater than or equal.\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->szPath[0] != '\0' && pParams->szWords[0] == '\0') )
	{
		wprintf(L"\n\nError: the words to search must be specified with the option '-w (--words)'\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->bPrefix + pParams->bSuffix + pParams->bContains) > 1 )
	{
		wprintf(L"\n\nError: the options '--prefix', '--suffix' and '--contains' are mutually exclusive: specify only one of them\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->bPrefix || pParams->bSuffix || pParams->bContains) && pParams->szWords[0] == '\0' )
	{
		wprintf(L"\n\nError: '--prefix', '--suffix' and '--contains' require the words to search ('-w (--words)') and the folder ('-p (--path)')\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->szPath[0] == '\0' && pParams->szWords[0] != '\0') )
	{
		wprintf(L"\n\nError: the folder containing the pdf files must be specified with the option '-p (--path)'\n\n");
		return 0;
	}

uscita:

	return retValue;
}

int MakeAndOpenOutputFile(const char *pszOutputFile, FILE **ppFile)
{
	int retValue = 1;

	*ppFile = NULL;

	#if !defined(_WIN64) && !defined(_WIN32)
	unsigned char szBOM[21];
	// UTF-8 BOM -> EF BB BF
	szBOM[0] = 0xEF;
	szBOM[1] = 0xBB;
	szBOM[2] = 0xBF;
	szBOM[3] = '\0';
	#endif

	(*ppFile) = fopen(pszOutputFile, "rb");
	if ( (*ppFile) != NULL )
	{
		fclose(*ppFile);
		*ppFile = NULL;
		wprintf(L"\n\nERROR: the specified output file already exists.\nPlease specify the name of a file that does not exist.\n\n");
		retValue = 0;
		goto uscita;
	}

	(*ppFile) = fopen(pszOutputFile, "wb");
	if ( (*ppFile) == NULL )
	{
		wprintf(L"\n\nERROR: unable to create the specified output file: '%s'.\n\n", pszOutputFile);
		retValue = 0;
		goto uscita;
	}

	#if defined(_WIN64) || defined(_WIN32)
	_setmode(_fileno((*ppFile)), _O_U8TEXT);
	#endif

	#if !defined(_WIN64) && !defined(_WIN32)
	// UTF-8 BOM -> EF BB BF
	fwprintf((*ppFile), L"%s", szBOM);
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
        return "not detected";

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

    return "not identified";
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

    wprintf(L"\nStorage units (sysfs information):\n");

    dir = opendir("/sys/block");
    if (dir == NULL)
    {
        wprintf(L"  Device information not available.\n");
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

        wprintf(L"  Device             : /dev/%hs\n", entry->d_name);
        if (vendor[0] != '\0')
            wprintf(L"    Vendor           : %hs\n", vendor);
        if (model[0] != '\0')
            wprintf(L"    Model            : %hs\n", model);
        wprintf(L"    Interface        : %hs\n", LinuxStorageTransport(devicePath));

        if (sectors == 0)
            wprintf(L"    Capacity         : not available (no media or zero capacity)\n");
        else
            wprintf(L"    Capacity         : %.2f GiB\n", sizeGiB);

        if (strncmp(entry->d_name, "sr", 2) == 0)
            wprintf(L"    Type             : optical drive\n");
        else if (rotational == 1)
            wprintf(L"    Media            : rotational (the kernel reports ROTA=1; typical HDD)\n");
        else if (rotational == 0)
            wprintf(L"    Media            : non-rotational (SSD/flash or reader)\n");
        else
            wprintf(L"    Media            : undetermined\n");

        count++;
    }

    closedir(dir);

    if (count == 0)
        wprintf(L"  No physical unit detected.\n");
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
        default: return "vendor not mapped";
    }
}

static void PrintLinuxGraphicsInfo(void)
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    wprintf(L"\nGPUs and graphics controllers (PCI/sysfs):\n");

    dir = opendir("/sys/bus/pci/devices");
    if (dir == NULL)
    {
        wprintf(L"  PCI information not available.\n");
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
        char driverName[128] = "not detected";
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
        wprintf(L"    Class           : ");
        if (((classCode >> 8) & 0xFFUL) == 0x00UL)
            wprintf(L"VGA controller\n");
        else if (((classCode >> 8) & 0xFFUL) == 0x02UL)
            wprintf(L"3D controller\n");
        else
            wprintf(L"display controller\n");
        wprintf(L"    Vendor          : %hs (PCI %04lx)\n", PciVendorName(vendorId), vendorId);
        wprintf(L"    Device ID       : %04lx\n", deviceId);
        if (vendorId == 0x1002 && deviceId == 0x675D)
            wprintf(L"    Known model     : Turks PRO (Radeon HD 7570)\n");
        if (subVendorText[0] != '\0' && subDeviceText[0] != '\0')
            wprintf(L"    Subsystem ID    : %04lx:%04lx\n", subVendorId, subDeviceId);
        wprintf(L"    Kernel driver   : %hs\n", driverName);
        count++;
    }

    closedir(dir);

    if (count == 0)
        wprintf(L"  No PCI graphics controller detected.\n");
}
#endif // __linux__ 

void PrintHardwareInfo(void)
{
    wprintf(L"Hardware and software information:\n");

    // Informazioni sul compilatore e sulla configurazione di build. 
#if defined(__clang__)
    wprintf(L"  Compiler           : Clang %hs\n", __VERSION__);
#elif defined(__GNUC__)
    wprintf(L"  Compiler           : GCC %hs\n", __VERSION__);
#elif defined(_MSC_VER)
    wprintf(L"  Compiler           : Microsoft Visual C++ (%d)\n", _MSC_VER);
#else
    wprintf(L"  Compiler           : not identified\n");
#endif

#if defined(MYPDFSEARCH_BUILD_DEBUG)
    wprintf(L"  Build              : DEBUG (-O0 -g)\n");
#elif defined(MYPDFSEARCH_BUILD_RELEASE)
    wprintf(L"  Build              : RELEASE (-O3)\n");
#elif defined(__OPTIMIZE__)
    wprintf(L"  Optimization       : enabled (level %d)\n", __OPTIMIZE__);
#else
    wprintf(L"  Optimization       : not detected/disabled\n");
#endif

#if defined(__STDC_VERSION__)
#if __STDC_VERSION__ >= 201710L
#ifdef __STRICT_ANSI__
    wprintf(L"  C standard         : C17\n");
#else
    wprintf(L"  C standard         : GNU C17\n");
#endif
#elif __STDC_VERSION__ >= 201112L
#ifdef __STRICT_ANSI__
    wprintf(L"  C standard         : C11\n");
#else
    wprintf(L"  C standard         : GNU C11\n");
#endif
#elif __STDC_VERSION__ >= 199901L
#ifdef __STRICT_ANSI__
    wprintf(L"  C standard         : C99\n");
#else
    wprintf(L"  C standard         : GNU C99\n");
#endif
#else
    wprintf(L"  C standard         : pre-C99 version\n");
#endif
#endif

#if defined(_WIN64) || defined(_WIN32)
    SYSTEM_INFO sysInfo;
    MEMORYSTATUSEX memStatus;

    GetNativeSystemInfo(&sysInfo);
    wprintf(L"\nHardware:\n");
    wprintf(L"  Online CPUs        : %u\n", sysInfo.dwNumberOfProcessors);
    wprintf(L"  OS architecture    : ");
    if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        wprintf(L"x64\n");
    else if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        wprintf(L"x86\n");
    else
        wprintf(L"Unknown\n");
    wprintf(L"  Build architecture : %ld-bit\n", (long)(sizeof(void *) * 8));

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
        wprintf(L"  Total RAM          : %.2f GiB\n", totalRamGiB);
        wprintf(L"  Available RAM      : %.2f GiB (system usage: %lu%%)\n",
                availRamGiB, memStatus.dwMemoryLoad);
    }


#else
    // Informazioni sul sistema operativo e sul kernel. 
    struct utsname osInfo;
    if (uname(&osInfo) == 0)
    {
        wprintf(L"\nSystem:\n");
        wprintf(L"  Operating system  : %hs\n", osInfo.sysname);
        wprintf(L"  Kernel             : %hs\n", osInfo.release);
        wprintf(L"  HW architecture    : %hs\n", osInfo.machine);
    }

    // CPU logiche visibili al sistema. 
    {
        long numProcessors = sysconf(_SC_NPROCESSORS_ONLN);
        if (numProcessors > 0)
            wprintf(L"\nHardware:\n  Online CPUs        : %ld\n", numProcessors);
        else
            wprintf(L"\nHardware:\n  Online CPUs        : not detected\n");
    }

    wprintf(L"  Build architecture : %ld-bit\n", (long)(sizeof(void *) * 8));

    // CPU logiche consentite al processo secondo la CPU affinity. 
#if defined(__linux__)
    {
        cpu_set_t cpuSet;
        CPU_ZERO(&cpuSet);
        if (sched_getaffinity(0, sizeof(cpuSet), &cpuSet) == 0)
        {
            int allowedCpus = CPU_COUNT(&cpuSet);
            if (allowedCpus > 0)
                wprintf(L"  Allowed CPUs       : %d\n", allowedCpus);
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
                wprintf(L"  Max CPU frequency  : %.2f GHz (declared value)\n",
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

                wprintf(L"  Total RAM          : %.2f GiB\n", totalRamGiB);
                wprintf(L"  Available RAM      : %.2f GiB (estimated system usage: %.1f%%)\n",
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
            wprintf(L"  Process peak RSS   : %.2f MiB\n",
                    (double)processUsage.ru_maxrss / (1024.0 * 1024.0));
#else
            wprintf(L"  Process peak RSS   : %.2f MiB\n",
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

	CLI_UNUSED(pszFile);

	if ( NULL != fp )
		fwprintf(fp, L"\n\nTEXT EXTRACTED FROM PAGE %d:\n\n", (int)nPage);
	else
		wprintf(L"\n\nTEXT EXTRACTED FROM PAGE %d:\n\n", (int)nPage);
}

int MYPDF_CALL CliOnPageText(void *pUser, const char *pszFile, int32_t nPage, const char *pszText, size_t nLen)
{
	CliOutput *pOut = (CliOutput*)pUser;
	FILE *fp = (NULL != pOut) ? pOut->fpOutput : NULL;

	CLI_UNUSED(pszFile);
	CLI_UNUSED(nPage);
	CLI_UNUSED(nLen);

	if ( NULL != fp )
		fwprintf(fp, L"%s", pszText);
	wprintf(L"%s", pszText);

	return 0;
}

int MYPDF_CALL CliOnWordFound(void *pUser, const char *pszFile, int32_t nPage, const char *pszWord)
{
	CliOutput *pOut = (CliOutput*)pUser;
	FILE *fp = (NULL != pOut) ? pOut->fpOutput : NULL;

	CLI_UNUSED(pszFile);

	if ( NULL != fp )
		fwprintf(fp, L"\tKey '%s' found on page %d\n", pszWord, (int)nPage);
	else
		wprintf(L"\tKey '%s' found on page %d\n", pszWord, (int)nPage);

	return 0;
}

wchar_t* myMultibyteToWidechar(const char *psz, size_t mbslen)
{
	wchar_t *wcs = NULL;

	wcs = calloc(mbslen + 1, sizeof(wchar_t));
	if (wcs == NULL)
	{
		//perror("calloc");
		wprintf(L"ERROR myMultibyteToWidechar calloc: %s \n", strerror(errno));

		return NULL;
	}

	if (mbstowcs(wcs, (char*)psz, mbslen + 1) == (size_t) -1)
	{
		//perror("mbstowcs");
		wprintf(L"ERROR myMultibyteToWidechar mbstowcs: %s \n", strerror(errno));

		free(wcs);
		wcs = NULL;
		return NULL;
	}

	return wcs;
}


// Messaggi della libreria:
//   - ERROR / WARNING relativi a un file  -> file di log "AAA_mypdfsearch_parsing_errors.txt" (aperto al primo messaggio);
//   - messaggi senza file (opzioni, elenco dei file) -> stderr;
//   - INFO (riepiloghi richiesti con --show_errors / --show_warnings) -> stderr.
void MYPDF_CALL CliOnMessage(void *pUser, mypdf_MessageLevel level, const char *pszFile, const char *pszMessage)
{
	CliOutput *pOut = (CliOutput*)pUser;

	if ( NULL == pOut )
	{
		fprintf(stderr, "%s\n", pszMessage);
		return;
	}

	if ( MYPDF_MSG_INFO == level )
	{
		if ( 0 == strncmp(pszMessage, "ERRORS (", 8) || 0 == strncmp(pszMessage, "WARNING (", 9) )
			fprintf(stderr, "%s (dettagli in AAA_mypdfsearch_parsing_errors.txt)\n", pszMessage);
		else
			fprintf(stderr, "%s\n", pszMessage);
		return;
	}

	if ( NULL == pszFile || '\0' == pszFile[0] )
	{
		fprintf(stderr, "%s\n", pszMessage);
		return;
	}

	// Gli avvisi (es. PDF saltato per la password) si vedono anche a video, su stderr: il testo su stdout resta pulito.
	if ( MYPDF_MSG_WARNING == level )
		fprintf(stderr, "File '%s': %s\n", pszFile, pszMessage);

	if ( NULL == pOut->fpErrors )
	{
		pOut->fpErrors = fopen("AAA_mypdfsearch_parsing_errors.txt", "a");
		if ( NULL == pOut->fpErrors )
		{
			fwprintf(stderr, L"\n\nERROR: unable to create the specified error messages file.\n\n");
			fprintf(stderr, "%s\n", pszMessage);
			return;
		}

		#if defined(_WIN64) || defined(_WIN32)
		_setmode(_fileno(pOut->fpErrors), _O_U8TEXT);
		#else
		// BOM UTF-8
		fwprintf(pOut->fpErrors, L"%s", "\xEF\xBB\xBF");
		#endif
	}

	#if !defined(_WIN64) && !defined(_WIN32)
	fwprintf(pOut->fpErrors, L"\nFile: '%s'\n", pszFile);
	#else
	{
		size_t lung = strnlen(pszFile, PATH_MAX);
		size_t idx;

		fwprintf(pOut->fpErrors, L"\nFILE = <");
		for ( idx = 0; idx < lung; idx++ )
			fwprintf(pOut->fpErrors, L"%c", pszFile[idx]);
		fwprintf(pOut->fpErrors, L">\n");
	}
	#endif

	fwprintf(pOut->fpErrors, L"\t%s\n", pszMessage);
}
