/* GTK4 example: graphical front end for libmypdfsearch (public API only).
 *
 * Type the path of a PDF (or of a folder) and, optionally, the words to search (separated by spaces);
 * "Start" shows the text of every page, or where the words are found. "Stop" cancels: the page callback
 * returns nonzero and the library answers MYPDF_ERR_CANCELLED.
 *
 * The processing runs in the main thread: while it runs, the callbacks give control back to the
 * GTK main loop, so the window stays responsive. (A real application would use a worker thread: one
 * mypdf_Context can be used by one thread at a time.)
 *
 * Standalone build with the installed library:
 *   cc main.c $(pkg-config --cflags --libs gtk4 mypdfsearch) -o mypdfsearch-gtk4
 */
#include <gtk/gtk.h>
#include <string.h>

#include "mypdfsearch.h"

#if GLIB_CHECK_VERSION(2, 74, 0)
#define APP_FLAGS G_APPLICATION_DEFAULT_FLAGS
#else
#define APP_FLAGS G_APPLICATION_FLAGS_NONE
#endif

typedef struct
{
	GtkWidget *window;
	GtkWidget *entryPath;
	GtkWidget *entryWords;
	GtkWidget *btnStart;
	GtkWidget *btnStop;
	GtkWidget *labelStatus;
	GtkTextBuffer *buffer;
	mypdf_Context *ctx;
	gboolean bRunning;
	gboolean bStop;
} App;

static void AppendText(App *app, const char *text, size_t len)
{
	GtkTextIter end;

	gtk_text_buffer_get_end_iter(app->buffer, &end);
	gtk_text_buffer_insert(app->buffer, &end, text, (gint)len);   /* the library gives valid UTF-8 */
}

static void AppendString(App *app, const char *text)
{
	AppendText(app, text, strlen(text));
}

/* Lets GTK handle the pending events (clicks on "Stop", repaint). */
static void PumpEvents(void)
{
	while (g_main_context_pending(NULL))
		g_main_context_iteration(NULL, FALSE);
}

static void MYPDF_CALL OnFileStart(void *user, const char *file)
{
	App *app = (App*)user;
	char *line = g_strdup_printf("\n=== %s ===\n", file);

	AppendString(app, line);
	g_free(line);
}

static int MYPDF_CALL OnPageText(void *user, const char *file, int32_t page, const char *text, size_t len)
{
	App *app = (App*)user;
	char *head = g_strdup_printf("\n--- page %d ---\n", (int)page);

	(void)file;
	AppendString(app, head);
	g_free(head);
	AppendText(app, text, len);

	PumpEvents();
	return app->bStop ? 1 : 0;      /* nonzero: stop */
}

static int MYPDF_CALL OnWordFound(void *user, const char *file, int32_t page, const char *word)
{
	App *app = (App*)user;
	char *line = g_strdup_printf("  '%s' found on page %d\n", word, (int)page);

	(void)file;
	AppendString(app, line);
	g_free(line);

	PumpEvents();
	return app->bStop ? 1 : 0;
}

static void MYPDF_CALL OnMessage(void *user, mypdf_MessageLevel level, const char *file, const char *message)
{
	App *app = (App*)user;
	const char *tag = (MYPDF_MSG_ERROR == level) ? "ERROR" : (MYPDF_MSG_WARNING == level) ? "WARNING" : "INFO";
	char *line = g_strdup_printf("[%s] %s%s%s\n", tag, file ? file : "", (file && file[0]) ? ": " : "", message);

	AppendString(app, line);
	g_free(line);
}

/* Encrypted PDFs: this example does not ask for the password (returning 0 skips the file). */
static int MYPDF_CALL OnPassword(void *user, const char *file, const char *reason, char *out, size_t max, size_t *outLen)
{
	(void)user; (void)file; (void)reason; (void)out; (void)max; (void)outLen;
	return 0;
}

static void SetRunning(App *app, gboolean bRunning)
{
	app->bRunning = bRunning;
	gtk_widget_set_sensitive(app->btnStart, !bRunning);
	gtk_widget_set_sensitive(app->btnStop, bRunning);
}

static void OnStart(GtkButton *button, gpointer data)
{
	App *app = (App*)data;
	const char *path = gtk_editable_get_text(GTK_EDITABLE(app->entryPath));
	const char *words = gtk_editable_get_text(GTK_EDITABLE(app->entryWords));
	mypdf_Status st;
	char *status;

	(void)button;

	if (app->bRunning)
		return;

	if (NULL == path || '\0' == path[0])
	{
		gtk_label_set_text(GTK_LABEL(app->labelStatus), "Enter the path of a PDF file or of a folder.");
		return;
	}

	gtk_text_buffer_set_text(app->buffer, "", 0);
	app->bStop = FALSE;
	SetRunning(app, TRUE);
	gtk_label_set_text(GTK_LABEL(app->labelStatus), "Working...");
	PumpEvents();

	if (MYPDF_OK != mypdf_SetSearchWords(app->ctx, words))
	{
		gtk_label_set_text(GTK_LABEL(app->labelStatus), mypdf_GetLastError(app->ctx));
		SetRunning(app, FALSE);
		return;
	}

	/* Callbacks: a text request and a search request are mutually exclusive in this example. */
	mypdf_SetFileStartCallback(app->ctx, OnFileStart, app);
	mypdf_SetPageTextCallback(app->ctx, (words && words[0]) ? NULL : OnPageText, app);
	mypdf_SetWordFoundCallback(app->ctx, (words && words[0]) ? OnWordFound : NULL, app);

	/* ProcessDirectory accepts a folder or a single file. */
	st = mypdf_ProcessDirectory(app->ctx, path, 1);

	{
		int64_t done = 0, withErrors = 0, withWarnings = 0;

		mypdf_GetStats(app->ctx, &done, &withErrors, &withWarnings);
		status = g_strdup_printf("%s - %lld file(s), %lld with errors, %lld with warnings",
		                         mypdf_StatusString(st), (long long)done, (long long)withErrors, (long long)withWarnings);
	}
	gtk_label_set_text(GTK_LABEL(app->labelStatus), status);
	g_free(status);

	SetRunning(app, FALSE);
}

static void OnStop(GtkButton *button, gpointer data)
{
	(void)button;
	((App*)data)->bStop = TRUE;
}

static void OnActivate(GtkApplication *gtkApp, gpointer data)
{
	App *app = (App*)data;
	GtkWidget *box, *row, *scrolled, *textView;

	app->window = gtk_application_window_new(gtkApp);
	gtk_window_set_title(GTK_WINDOW(app->window), "mypdfsearch");
	gtk_window_set_default_size(GTK_WINDOW(app->window), 800, 600);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_set_margin_start(box, 8);
	gtk_widget_set_margin_end(box, 8);
	gtk_widget_set_margin_top(box, 8);
	gtk_widget_set_margin_bottom(box, 8);
	gtk_window_set_child(GTK_WINDOW(app->window), box);

	app->entryPath = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(app->entryPath), "PDF file or folder");
	gtk_box_append(GTK_BOX(box), app->entryPath);

	app->entryWords = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(app->entryWords), "Words to search, separated by spaces (empty: extract the text)");
	gtk_box_append(GTK_BOX(box), app->entryWords);

	row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	app->btnStart = gtk_button_new_with_label("Start");
	app->btnStop = gtk_button_new_with_label("Stop");
	gtk_widget_set_sensitive(app->btnStop, FALSE);
	gtk_box_append(GTK_BOX(row), app->btnStart);
	gtk_box_append(GTK_BOX(row), app->btnStop);
	gtk_box_append(GTK_BOX(box), row);

	scrolled = gtk_scrolled_window_new();
	gtk_widget_set_vexpand(scrolled, TRUE);
	textView = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);
	gtk_text_view_set_monospace(GTK_TEXT_VIEW(textView), TRUE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textView), GTK_WRAP_WORD_CHAR);
	app->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), textView);
	gtk_box_append(GTK_BOX(box), scrolled);

	app->labelStatus = gtk_label_new("");
	gtk_label_set_xalign(GTK_LABEL(app->labelStatus), 0.0f);
	gtk_box_append(GTK_BOX(box), app->labelStatus);

	g_signal_connect(app->btnStart, "clicked", G_CALLBACK(OnStart), app);
	g_signal_connect(app->btnStop, "clicked", G_CALLBACK(OnStop), app);
	g_signal_connect(app->entryPath, "activate", G_CALLBACK(OnStart), app);
	g_signal_connect(app->entryWords, "activate", G_CALLBACK(OnStart), app);

	gtk_window_present(GTK_WINDOW(app->window));
}

int main(int argc, char **argv)
{
	App app;
	GtkApplication *gtkApp;
	int status;

	memset(&app, 0, sizeof(app));

	if (MYPDF_OK != mypdf_Init())
	{
		g_printerr("mypdf_Init failed\n");
		return 1;
	}

	app.ctx = mypdf_CreateContext();
	if (NULL == app.ctx)
	{
		g_printerr("mypdf_CreateContext failed\n");
		mypdf_Shutdown();
		return 1;
	}
	mypdf_SetMessageCallback(app.ctx, OnMessage, &app);
	mypdf_SetPasswordCallback(app.ctx, OnPassword, &app);

	gtkApp = gtk_application_new("org.mypdfsearch.example", APP_FLAGS);
	g_signal_connect(gtkApp, "activate", G_CALLBACK(OnActivate), &app);
	status = g_application_run(G_APPLICATION(gtkApp), argc, argv);
	g_object_unref(gtkApp);

	mypdf_DestroyContext(app.ctx);
	mypdf_Shutdown();

	return status;
}
