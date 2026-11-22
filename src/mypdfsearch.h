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
 * mypdfsearch.h  --  API PUBBLICA di libmypdfsearch
 *
 * Regole di questa API (valgono per ogni linguaggio: C, C++, Python/ctypes, C#, Rust, Go...):
 *   - Prefissi:  funzioni mypdf_PascalCase, tipi mypdf_PascalCase, costanti/macro MYPDF_MAIUSCOLO.
 *   - Tutte le stringhe sono UTF-8 (char *), terminate da '\0'. Mai wchar_t (16 bit su Windows, 32 su Linux).
 *   - Solo interi a larghezza fissa (int32_t, int64_t, uint32_t) e size_t. Mai long, mai bool C99.
 *   - Nessun FILE*, nessuna struct passata per valore, nessuna struct pubblica con layout "fragile":
 *     il contesto e' un puntatore opaco.
 *   - Ogni funzione che puo' fallire ritorna mypdf_Status (0 = MYPDF_OK).
 *   - La memoria allocata dalla libreria si libera SOLO con le funzioni della libreria.
 *   - La libreria non scrive MAI su stdout/stderr e non crea file nella cwd: tutto l'output passa
 *     dalle callback. (Il CLI, main.c, installa callback che stampano come oggi.)
 *   - Il chiamante imposta la locale UTF-8 (setlocale) solo se usa le funzioni C di stampa: la
 *     libreria non chiama setlocale().
 *   - Thread safety: due mypdf_Context diversi possono essere usati da thread diversi; lo
 *     stesso contesto NON va usato da due thread insieme. mypdf_Init/Shutdown: una volta per processo.
 */

#ifndef MYPDFSEARCH_H
#define MYPDFSEARCH_H

#include <stddef.h>   /* size_t */
#include <stdint.h>   /* int32_t, int64_t */

/* ---------------------------------------------------------------------------
 * 1. Visibilita' / convenzione di chiamata  (UNICO punto in cui compaiono)
 * -------------------------------------------------------------------------*/
#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(MYPDF_STATIC)
#    define MYPDF_API
#  elif defined(MYPDF_BUILDING_DLL)
#    define MYPDF_API __declspec(dllexport)
#  else
#    define MYPDF_API __declspec(dllimport)
#  endif
#  define MYPDF_CALL __cdecl
#else
#  if defined(__GNUC__) && __GNUC__ >= 4
#    define MYPDF_API __attribute__((visibility("default")))
#  else
#    define MYPDF_API
#  endif
#  define MYPDF_CALL
#endif

#ifdef __cplusplus
#  define MYPDF_BEGIN_DECLS extern "C" {
#  define MYPDF_END_DECLS   }
#else
#  define MYPDF_BEGIN_DECLS
#  define MYPDF_END_DECLS
#endif

MYPDF_BEGIN_DECLS

/* ---------------------------------------------------------------------------
 * 2. Versione
 * -------------------------------------------------------------------------*/
#define MYPDF_VERSION_MAJOR 0
#define MYPDF_VERSION_MINOR 1
#define MYPDF_VERSION_PATCH 0

/* Versione a runtime (stringa statica, non liberare), es. "0.1.0".
 * Utile per verificare che la .so/.dll caricata corrisponda agli header usati. */
MYPDF_API const char * MYPDF_CALL mypdf_GetVersion(void);

/* ---------------------------------------------------------------------------
 * 3. Stato / errori
 * -------------------------------------------------------------------------*/
typedef enum mypdf_Status
{
	MYPDF_OK                    = 0,
	MYPDF_ERR_INVALID_ARGUMENT  = 1,   /* puntatore NULL, parametro fuori range */
	MYPDF_ERR_OUT_OF_MEMORY     = 2,
	MYPDF_ERR_FILE_OPEN         = 3,   /* file inesistente / non leggibile */
	MYPDF_ERR_NOT_A_PDF         = 4,
	MYPDF_ERR_CORRUPTED         = 5,   /* struttura PDF danneggiata */
	MYPDF_ERR_ENCRYPTED         = 6,   /* cifrato e password mancante/errata */
	MYPDF_ERR_UNSUPPORTED       = 7,
	MYPDF_ERR_CANCELLED         = 8,   /* una callback ha chiesto di interrompere */
	MYPDF_ERR_FILE_ERRORS       = 9,   /* elaborato, ma con errori non fatali (equivale all'exit code 2 del CLI) */
	MYPDF_ERR_INTERNAL          = 99,
	MYPDF_STATUS_FORCE_INT32    = 0x7fffffff   /* forza enum a 32 bit su tutti i compilatori */
} mypdf_Status;

/* Stringa statica che descrive lo status (non liberare). */
MYPDF_API const char * MYPDF_CALL mypdf_StatusString(mypdf_Status status);

/* Ultimo messaggio d'errore dettagliato del contesto (UTF-8). Valido fino alla
 * prossima chiamata sullo stesso contesto. Mai NULL ("" se nessun errore). */
struct mypdf_Context;
typedef struct mypdf_Context mypdf_Context;
MYPDF_API const char * MYPDF_CALL mypdf_GetLastError(const mypdf_Context *ctx);

/* ---------------------------------------------------------------------------
 * 4. Inizializzazione globale (una volta per processo)
 *    Inizializza OpenSSL (provider per RC4/AES/MD5 dei PDF cifrati).
 * -------------------------------------------------------------------------*/
MYPDF_API mypdf_Status MYPDF_CALL mypdf_Init(void);
MYPDF_API void         MYPDF_CALL mypdf_Shutdown(void);

/* ---------------------------------------------------------------------------
 * 5. Contesto (handle opaco)
 * -------------------------------------------------------------------------*/
MYPDF_API mypdf_Context * MYPDF_CALL mypdf_CreateContext(void);   /* NULL se memoria esaurita */
MYPDF_API void            MYPDF_CALL mypdf_DestroyContext(mypdf_Context *ctx);  /* accetta NULL */

/* ---------------------------------------------------------------------------
 * 6. Callback
 *    Tutte ricevono user_data cosi' come passato in fase di registrazione
 *    (in Python: ctypes.py_object / id; in C#: GCHandle; in Rust: Box::into_raw).
 *    Le stringhe ricevute sono valide SOLO durante la callback: copiarle se servono dopo.
 *    Le callback NON devono sollevare eccezioni/longjmp verso la libreria.
 * -------------------------------------------------------------------------*/

/* Testo di una pagina, chiamata per ogni pagina elaborata (text e' "" se la pagina non ha testo).
 * text e' UTF-8, terminato da '\0' (byte_len non lo include). Ritorno: 0 = continua, != 0 = interrompi (MYPDF_ERR_CANCELLED). */
typedef int (MYPDF_CALL *mypdf_PageTextCallback)(void *user_data,
                                                 const char *file_path,
                                                 int32_t page_number,      /* 1-based */
                                                 const char *text,
                                                 size_t byte_len);

/* Parola trovata in una pagina (una chiamata per parola per pagina).
 * - Modo MYPDF_MATCH_WORD (predefinito): 'word' e' la parola cercata.
 * - Modi MYPDF_MATCH_PREFIX / SUFFIX / CONTAINS: 'word' e' la parola effettivamente presente nel testo
 *   (in minuscolo, normalizzata NFC) che corrisponde, es. cercando "pre" con MYPDF_MATCH_PREFIX si ricevono
 *   "prezzo", "prefisso", ... ciascuna una sola volta per pagina. */
typedef int (MYPDF_CALL *mypdf_WordFoundCallback)(void *user_data,
                                                  const char *file_path,
                                                  int32_t page_number,
                                                  const char *word);

typedef enum mypdf_MessageLevel
{
	MYPDF_MSG_INFO    = 0,
	MYPDF_MSG_WARNING = 1,
	MYPDF_MSG_ERROR   = 2,
	MYPDF_MSG_FORCE_INT32 = 0x7fffffff
} mypdf_MessageLevel;

/* Messaggi diagnostici (oggi: myShowErrorMessage + file di log). */
typedef void (MYPDF_CALL *mypdf_MessageCallback)(void *user_data,
                                                 mypdf_MessageLevel level,
                                                 const char *file_path,
                                                 const char *message);

/* Richiesta password per PDF cifrati (oggi: OnUserPasswordCallback).
 * Scrivere la password UTF-8 in out_password (max_len compreso il '\0'),
 * lunghezza (senza '\0') in *out_len. Ritorno: 1 = password fornita, 0 = rinuncia. */
typedef int (MYPDF_CALL *mypdf_PasswordCallback)(void *user_data,
                                                 const char *file_path,
                                                 const char *reason,        /* es. "password errata" */
                                                 char *out_password,
                                                 size_t max_len,
                                                 size_t *out_len);

/* Inizio elaborazione di un file (una chiamata per file, prima delle callback delle sue pagine). */
typedef void (MYPDF_CALL *mypdf_FileStartCallback)(void *user_data, const char *file_path);

/* Inizio elaborazione di una pagina (prima che ne siano emessi testo e messaggi). */
typedef void (MYPDF_CALL *mypdf_PageStartCallback)(void *user_data, const char *file_path, int32_t page_number);

MYPDF_API void MYPDF_CALL mypdf_SetPageStartCallback(mypdf_Context *ctx, mypdf_PageStartCallback cb, void *user_data);
MYPDF_API void MYPDF_CALL mypdf_SetFileStartCallback(mypdf_Context *ctx, mypdf_FileStartCallback cb, void *user_data);
MYPDF_API void MYPDF_CALL mypdf_SetPageTextCallback(mypdf_Context *ctx, mypdf_PageTextCallback cb, void *user_data);
MYPDF_API void MYPDF_CALL mypdf_SetWordFoundCallback(mypdf_Context *ctx, mypdf_WordFoundCallback cb, void *user_data);
MYPDF_API void MYPDF_CALL mypdf_SetMessageCallback(mypdf_Context *ctx, mypdf_MessageCallback cb, void *user_data);
MYPDF_API void MYPDF_CALL mypdf_SetPasswordCallback(mypdf_Context *ctx, mypdf_PasswordCallback cb, void *user_data);

/* ---------------------------------------------------------------------------
 * 7. Opzioni (setter, non struct pubblica: aggiungere opzioni non rompe l'ABI)
 * -------------------------------------------------------------------------*/

/* Parole da cercare: stringa UTF-8 con parole separate da spazi (come l'opzione CLI).
 * NULL o "" = nessuna ricerca. La libreria ne tiene una copia. */
MYPDF_API mypdf_Status MYPDF_CALL mypdf_SetSearchWords(mypdf_Context *ctx, const char *utf8_words);

/* Intervallo di pagine 1-based, inclusivo. first<=0 = dalla prima; last<=0 = fino all'ultima.
 * Se entrambi > 0 e first > last: MYPDF_ERR_INVALID_ARGUMENT. */
MYPDF_API mypdf_Status MYPDF_CALL mypdf_SetPageRange(mypdf_Context *ctx, int32_t first, int32_t last);

/* Opzioni booleane / numeriche del CLI mappate una per una (elenco definitivo da
 * concordare leggendo myParseOption: -s ordinamento glifi, -w, ecc.). */
typedef enum mypdf_Option
{
	MYPDF_OPT_SORT_GLYPHS   = 1,   /* 1 = ordina i glifi per riga (CLI: -s); 0 = ordine di apparizione (predefinito) */
	MYPDF_OPT_SHOW_ERRORS   = 2,   /* 1 = invia alla callback dei messaggi il riepilogo degli errori per file (CLI: --show_errors) */
	MYPDF_OPT_SHOW_WARNINGS = 3,   /* 1 = idem per i warning (CLI: --show_warnings) */
	MYPDF_OPT_MATCH_MODE    = 4,   /* come si confrontano le parole cercate: un valore di mypdf_MatchMode (CLI: --prefix, --suffix, --contains) */
	MYPDF_OPT_FORCE_INT32 = 0x7fffffff
} mypdf_Option;

/* Modo di confronto delle parole cercate (opzione MYPDF_OPT_MATCH_MODE; valori diversi: MYPDF_ERR_INVALID_ARGUMENT).
 * Le parole cercate che contengono caratteri CJK sono sempre confrontate come parola intera
 * (il testo CJK e' gia' indicizzato per singoli caratteri e coppie di caratteri). */
typedef enum mypdf_MatchMode
{
	MYPDF_MATCH_WORD     = 0,   /* parola intera (predefinito) */
	MYPDF_MATCH_PREFIX   = 1,   /* la parola del testo inizia con la parola cercata */
	MYPDF_MATCH_SUFFIX   = 2,   /* la parola del testo finisce con la parola cercata */
	MYPDF_MATCH_CONTAINS = 3,   /* la parola del testo contiene la parola cercata */
	MYPDF_MATCH_FORCE_INT32 = 0x7fffffff
} mypdf_MatchMode;

MYPDF_API mypdf_Status MYPDF_CALL mypdf_SetOptionInt(mypdf_Context *ctx, mypdf_Option option, int64_t value);

/* ---------------------------------------------------------------------------
 * 8. Elaborazione
 * -------------------------------------------------------------------------*/

/* Elabora un singolo PDF (path UTF-8; su Windows la libreria lo converte in UTF-16 internamente).
 * Per ogni pagina chiama PageTextCallback (se impostata) e, per ogni parola trovata, WordFoundCallback. */
MYPDF_API mypdf_Status MYPDF_CALL mypdf_ProcessFile(mypdf_Context *ctx, const char *utf8_path);

/* Elabora tutti i PDF di una cartella (ricorsivo se recursive != 0); utf8_dir puo' essere anche un
 * singolo file. Prosegue dopo errori sui singoli file; ritorna MYPDF_ERR_FILE_ERRORS se almeno un
 * file ha avuto errori. */
MYPDF_API mypdf_Status MYPDF_CALL mypdf_ProcessDirectory(mypdf_Context *ctx, const char *utf8_dir, int32_t recursive);

/* Contatori dall'ultima mypdf_Process*: file elaborati, con errori, con avvisi. */
MYPDF_API void MYPDF_CALL mypdf_GetStats(const mypdf_Context *ctx,
                                         int64_t *files_processed,
                                         int64_t *files_with_errors,
                                         int64_t *files_with_warnings);

/* ---------------------------------------------------------------------------
 * 9. Memoria
 * -------------------------------------------------------------------------*/
/* Per eventuali funzioni future che restituiscono buffer allocati dalla libreria. */
MYPDF_API void MYPDF_CALL mypdf_Free(void *ptr);

MYPDF_END_DECLS

#endif /* MYPDFSEARCH_H */
