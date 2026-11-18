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

#include "mypdfsearch.h"
#include "scanner.h"

void PrintHelpCommandLine(void);
void PrintVersionInfo(void);

int myParseOption(Params *pParams, char *pszOption);
int myParseCommandLine(Params *pParams, int argc, char **argv);

int MakeAndOpenOutputFile(Params *pParams);

void PrintHardwareInfo(void);

#endif /* MYPDF_CLI_INCLUDED */
