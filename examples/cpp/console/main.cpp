// C++17 example: text extraction and word search with libmypdfsearch (console only).
//
//   mypdfsearch-cpp <file.pdf | folder> [words]
//
// Without 'words' prints the text of every page; with 'words' (separated by spaces) prints where they
// are found. Standalone build with the installed library:
//   c++ -std=c++17 main.cpp $(pkg-config --cflags --libs mypdfsearch) -o mypdfsearch-cpp
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/stat.h>

#include "mypdfsearch.hpp"

static bool isDirectory(const std::string &path)
{
	struct stat st;
	return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

int main(int argc, char **argv)
{
	if (argc < 2 || argc > 3)
	{
		std::cerr << "usage: " << argv[0] << " <file.pdf | folder> [words]\n";
		return EXIT_FAILURE;
	}

	const std::string path = argv[1];
	const std::string words = (argc == 3) ? argv[2] : "";

	try
	{
		mypdf::Library lib;                 // mypdf_Init / mypdf_Shutdown
		mypdf::Context ctx;

		std::cerr << "libmypdfsearch " << mypdf::Library::version() << "\n";

		ctx.onMessage([](mypdf_MessageLevel level, const std::string &file, const std::string &msg)
		{
			const char *tag = (level == MYPDF_MSG_ERROR) ? "ERROR" : (level == MYPDF_MSG_WARNING) ? "WARNING" : "INFO";
			std::cerr << "[" << tag << "] " << (file.empty() ? "" : file + ": ") << msg << "\n";
		});

		ctx.onFileStart([](const std::string &file) { std::cout << "== " << file << "\n"; });

		if (words.empty())
		{
			ctx.onPageText([](const std::string &, int page, const std::string &text)
			{
				std::cout << "--- page " << page << " ---\n" << text << "\n";
				return true;                // false would stop the processing
			});
		}
		else
		{
			ctx.setSearchWords(words);
			ctx.onWordFound([](const std::string &, int page, const std::string &word)
			{
				std::cout << "  '" << word << "' found on page " << page << "\n";
				return true;
			});
		}


		mypdf_Status st = isDirectory(path) ? ctx.processDirectory(path, true) : ctx.processFile(path);

		std::int64_t done = 0, withErrors = 0, withWarnings = 0;
		ctx.stats(done, withErrors, withWarnings);
		std::cerr << done << " file(s), " << withErrors << " with errors, " << withWarnings << " with warnings\n";

		if (st != MYPDF_OK && st != MYPDF_ERR_ENCRYPTED)
		{
			std::cerr << mypdf_StatusString(st) << ": " << ctx.lastError() << "\n";
			return (st == MYPDF_ERR_FILE_ERRORS) ? 2 : EXIT_FAILURE;
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
