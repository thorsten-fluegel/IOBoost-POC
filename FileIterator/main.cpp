#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>
#include <iterator>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <string>
#include <list>


std::list<std::wstring> filesInFolder(const std::wstring& path)
{
	std::list<std::wstring> files;

	std::wstring searchMask(path + L"\\*");
	WIN32_FIND_DATA findData;
	HANDLE findHandle = FindFirstFile(searchMask.c_str(), &findData);
	while (findHandle != INVALID_HANDLE_VALUE)
	{
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
			std::wstring(L".") != findData.cFileName &&
			std::wstring(L"..") != findData.cFileName)
		{
			std::wstring dir(path + L"\\" + findData.cFileName);
			auto folder_content = filesInFolder(dir);
			std::copy(folder_content.cbegin(), folder_content.cend(), std::back_inserter(files));
		}
		else if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			std::wstring filepath(path + L"\\" + findData.cFileName);
			files.push_back(filepath);
		}

		if (!FindNextFile(findHandle, &findData))
		{
			findHandle = INVALID_HANDLE_VALUE;
		}
	}
	FindClose(findHandle);

	return files;
}


uint8_t filehash(const std::wstring& filename)
{
	uint8_t hash = 0;
	std::ifstream fileStream(filename, std::ios::binary);

	if (fileStream)
	{
		fileStream.seekg(0, fileStream.end);
		size_t size = static_cast<size_t>(fileStream.tellg());
		fileStream.seekg(0, fileStream.beg);

		char *buffer = new char[size];
		fileStream.read(buffer, size);

		for (size_t i = 0; i < size; ++i)
		{
			hash ^= buffer[i];
		}

		delete[] buffer;
	}

	return hash;
}


std::chrono::duration<double> time(const std::function<void ()>& f)
{
	auto t_start = std::chrono::high_resolution_clock::now();

	f();

	auto t_end = std::chrono::high_resolution_clock::now();
	auto duration_s = std::chrono::duration_cast<std::chrono::duration<double>>(t_end - t_start);

	return duration_s;
}


void wmain(int argc, wchar_t** argv)
{
	auto sleep_1s = []() {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	};

	std::list<std::wstring> files;
	for (auto i = 1; i < argc; ++i)
	{
		auto _files = filesInFolder(argv[i]);
		for (auto& f : _files)
		{
			files.push_back(f);
		}
	}

	uint8_t dummy_hash = 0;
	auto hash_files = [&dummy_hash](const std::list<std::wstring>& file_list) {
		for (auto& f : file_list)
		{
			dummy_hash ^= filehash(f);
		}
	};

	std::wcout << L"slept for " << time(sleep_1s).count() << L"s" << std::endl;
	std::wcout << L"hashing files took " << time(std::bind(hash_files, files)).count() << L"s" << std::endl;
}
