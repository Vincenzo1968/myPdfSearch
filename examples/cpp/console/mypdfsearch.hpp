// mypdfsearch.hpp -- thin C++17 wrapper (header only) around the C API of libmypdfsearch.
//
// It only uses mypdfsearch.h: exceptions for fatal errors, RAII for the context and std::function
// for the callbacks. All strings are UTF-8 std::string. A callback returns true to continue,
// false to stop the processing (the C API then returns MYPDF_ERR_CANCELLED).
// An exception thrown by a callback is caught, stops the processing and is rethrown by
// processFile()/processDirectory() (exceptions must never cross the C library).
#ifndef MYPDFSEARCH_HPP
#define MYPDFSEARCH_HPP

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

#include "mypdfsearch.h"

namespace mypdf {

class Error : public std::runtime_error
{
public:
	Error(mypdf_Status status, const std::string &what) : std::runtime_error(what), status_(status) {}
	mypdf_Status status() const noexcept { return status_; }
private:
	mypdf_Status status_;
};

// One per process: initializes and releases the global state of the library (OpenSSL).
class Library
{
public:
	Library()
	{
		mypdf_Status st = mypdf_Init();
		if (st != MYPDF_OK)
			throw Error(st, std::string("mypdf_Init: ") + mypdf_StatusString(st));
	}
	~Library() { mypdf_Shutdown(); }
	Library(const Library &) = delete;
	Library &operator=(const Library &) = delete;

	static std::string version() { return mypdf_GetVersion(); }
};

class Context
{
public:
	using PageTextFn  = std::function<bool(const std::string &file, int page, const std::string &text)>;
	using WordFoundFn = std::function<bool(const std::string &file, int page, const std::string &word)>;
	using MessageFn   = std::function<void(mypdf_MessageLevel level, const std::string &file, const std::string &message)>;
	using FileStartFn = std::function<void(const std::string &file)>;

	Context() : ctx_(mypdf_CreateContext())
	{
		if (!ctx_)
			throw Error(MYPDF_ERR_OUT_OF_MEMORY, "mypdf_CreateContext failed");
	}
	~Context() { mypdf_DestroyContext(ctx_); }
	Context(const Context &) = delete;
	Context &operator=(const Context &) = delete;

	// ---- options ---------------------------------------------------------------------------
	void setSearchWords(const std::string &words) { check(mypdf_SetSearchWords(ctx_, words.c_str())); }
	void setPageRange(int first, int last)        { check(mypdf_SetPageRange(ctx_, first, last)); }
	void setSortGlyphsByRow(bool on)              { check(mypdf_SetOptionInt(ctx_, MYPDF_OPT_SORT_GLYPHS, on ? 1 : 0)); }

	// ---- callbacks (the std::function objects live as long as the Context) -----------------
	void onFileStart(FileStartFn fn) { fileStart_ = std::move(fn); mypdf_SetFileStartCallback(ctx_, fileStart_ ? &fileStartTramp : nullptr, this); }
	void onPageText(PageTextFn fn)   { pageText_ = std::move(fn);  mypdf_SetPageTextCallback(ctx_, pageText_ ? &pageTextTramp : nullptr, this); }
	void onWordFound(WordFoundFn fn) { wordFound_ = std::move(fn); mypdf_SetWordFoundCallback(ctx_, wordFound_ ? &wordFoundTramp : nullptr, this); }
	void onMessage(MessageFn fn)     { message_ = std::move(fn);   mypdf_SetMessageCallback(ctx_, message_ ? &messageTramp : nullptr, this); }

	// ---- processing: return the status of the C API (MYPDF_OK, MYPDF_ERR_FILE_ERRORS, ...) --
	mypdf_Status processFile(const std::string &path)                       { return run(mypdf_ProcessFile(ctx_, path.c_str())); }
	mypdf_Status processDirectory(const std::string &path, bool recursive)  { return run(mypdf_ProcessDirectory(ctx_, path.c_str(), recursive ? 1 : 0)); }

	std::string lastError() const { return mypdf_GetLastError(ctx_); }

	void stats(std::int64_t &processed, std::int64_t &withErrors, std::int64_t &withWarnings) const
	{
		mypdf_GetStats(ctx_, &processed, &withErrors, &withWarnings);
	}

private:
	static Context *self(void *u) { return static_cast<Context *>(u); }
	static std::string str(const char *s) { return s ? std::string(s) : std::string(); }

	void check(mypdf_Status st)
	{
		if (st != MYPDF_OK)
			throw Error(st, std::string(mypdf_StatusString(st)) + ": " + lastError());
	}

	mypdf_Status run(mypdf_Status st)
	{
		if (pending_)
		{
			std::exception_ptr e = pending_;
			pending_ = nullptr;
			std::rethrow_exception(e);
		}
		return st;
	}

	// The trampolines must not let exceptions escape into the C library.
	static void MYPDF_CALL fileStartTramp(void *u, const char *file)
	{
		Context *c = self(u);
		try { c->fileStart_(str(file)); } catch (...) { c->pending_ = std::current_exception(); }
	}
	static int MYPDF_CALL pageTextTramp(void *u, const char *file, std::int32_t page, const char *text, std::size_t len)
	{
		Context *c = self(u);
		try { return c->pageText_(str(file), page, std::string(text, len)) ? 0 : 1; }
		catch (...) { c->pending_ = std::current_exception(); return 1; }
	}
	static int MYPDF_CALL wordFoundTramp(void *u, const char *file, std::int32_t page, const char *word)
	{
		Context *c = self(u);
		try { return c->wordFound_(str(file), page, str(word)) ? 0 : 1; }
		catch (...) { c->pending_ = std::current_exception(); return 1; }
	}
	static void MYPDF_CALL messageTramp(void *u, mypdf_MessageLevel level, const char *file, const char *msg)
	{
		Context *c = self(u);
		try { c->message_(level, str(file), str(msg)); } catch (...) { c->pending_ = std::current_exception(); }
	}

	mypdf_Context *ctx_;
	FileStartFn fileStart_;
	PageTextFn pageText_;
	WordFoundFn wordFound_;
	MessageFn message_;
	std::exception_ptr pending_;
};

} // namespace mypdf

#endif // MYPDFSEARCH_HPP
