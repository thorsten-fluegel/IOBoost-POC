#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>
#include <iterator>
#include <iostream>
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


std::chrono::duration<double> time(std::function<void ()> f)
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

	auto list_files = [](const std::list<std::wstring>& file_list) {
		for (auto& f : file_list)
		{
			std::wcout << f << std::endl;
		}
	};

	std::wcout << L"slept for " << time(sleep_1s).count() << L"s" << std::endl;
	std::wcout << L"listing files took " << time(std::bind(list_files, files)).count() << L"s" << std::endl;
}
