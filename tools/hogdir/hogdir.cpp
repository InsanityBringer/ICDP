/*
Copyright (c) 2019 SaladBadger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//hogdir: takes a directory from the command line and packages it into a hog
//file for usage in ICDP. 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <filesystem>
#include <wchar.h>

constexpr int FILENAME_LEN = 13;
const char* sig = "DHF";

FILE* hogfile;

#ifdef _MSC_VER
void add_file(const wchar_t* filename, FILE* file)
#else
void add_file(const char* filename, FILE* file)
#endif
{
#ifdef _MSC_VER
	if (wcslen(filename) >= FILENAME_LEN)
	{
		printf("Skipping file %ls because the filename is too long.\n", filename);
		return;
	}
#else
	if (strlen(filename) >= FILENAME_LEN)
	{
		printf("Skipping file %s because the filename is too long.\n", filename);
		return;
	}
#endif

	char filenamebuf[FILENAME_LEN];
	memset(filenamebuf, 0, sizeof(filenamebuf)); //remove garbage from the directory entry
#ifdef _MSC_VER
	for (int i = 0; i < wcslen(filename); i++)
		filenamebuf[i] = (char)filename[i]; //DISGUSTING
#else
	strncpy(filenamebuf, filename, FILENAME_LEN);
#endif

	fseek(file, 0, SEEK_END);
	int length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* buffer = (char*)malloc(length);
	if (!buffer)
	{
#ifdef _MSC_VER
		fprintf(stderr, "Failed to malloc buffer for file %ls.\n", filename);
#else
		fprintf(stderr, "Failed to malloc buffer for file %s.\n", filename);
#endif
		return;
	}
	fread(buffer, 1, length, file);

	fwrite(&filenamebuf, 1, sizeof(filenamebuf), hogfile);
	fwrite(&length, sizeof(length), 1, hogfile);
	fwrite(buffer, 1, length, hogfile);

	free(buffer);
}

void add_dir(const char* directory)
{
	if (strlen(directory) == 0)
	{
		fprintf(stderr, "Zero-length search string\n");
		return;
	}

	std::filesystem::path path(directory);
	std::filesystem::directory_iterator it = std::filesystem::directory_iterator(path);

	for (std::filesystem::directory_entry const& entry : it)
	{
		if (entry.is_regular_file())
		{
			std::filesystem::path const& filepath = entry.path();
#ifdef _MSC_VER
			FILE* fp = _wfopen(filepath.c_str(), L"rb");
#else
			FILE* fp = fopen(filepath.c_str(), "rb");
#endif
			if (!fp)
			{
#ifdef _MSC_VER
				fprintf(stderr, "Failed to open file %ls.\n", filepath.c_str());
#else
				fprintf(stderr, "Failed to open file %s.\n", filepath.c_str());
#endif
			}
			else
			{
				add_file(filepath.filename().c_str(), fp);
				fclose(fp);
			}
		}
	}
}

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		printf("usage: hogdir [output file] [source dir]\n");
		return 0;
	}

	hogfile = fopen(argv[1], "wb");
	if (!hogfile)
	{
		fprintf(stderr, "Failed to open output file %s.\n", argv[1]);
		return 1;
	}

	fwrite(sig, 1, 3, hogfile);

	add_dir(argv[2]);

	fclose(hogfile);

	return 0;
}

