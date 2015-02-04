#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winioctl.h>

#include <algorithm>
#include <iterator>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <ctime>
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


uint64_t filehash(const std::wstring& filename, size_t maxSize = std::numeric_limits<size_t>::max())
{
	uint64_t hash = 0;
	std::ifstream fileStream(filename, std::ios::binary);

	if (fileStream)
	{
		fileStream.seekg(0, fileStream.end);
		size_t size = std::min(static_cast<size_t>(fileStream.tellg()), maxSize);
		fileStream.seekg(0, fileStream.beg);

		char *buffer = new char[size];
		fileStream.read(buffer, size);

		// 64-bit loop unrolling
		uint64_t *buffer64 = reinterpret_cast<uint64_t*>(buffer);
		size_t size64 = size / sizeof(uint64_t);
		for (size_t i = 0; i < size64; ++i)
		{
			hash ^= buffer64[i];
		}

		// do the rest with 8-bit operations
		for (size_t i = size64 * sizeof(uint64_t); i < size; ++i)
		{
			hash ^= (buffer[i] << ((i % sizeof(uint64_t))*8)); // calculate the number of bits to shift the 8-bit value from the buffer and xor it with the hash
		}

		delete[] buffer;
	}

	return hash;
}


uint64_t GetStartCluster(const std::wstring& filename)
{
	uint64_t clusterNumber = 0;

	HANDLE fileHandle = CreateFile(filename.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE != fileHandle)
	{
		STARTING_VCN_INPUT_BUFFER input;
		input.StartingVcn.QuadPart = 0;

		// actually we only care about the first cluster number.
		// DeviceIoControl doesn't seem to be able to return the number of number of bytes required, so we have to guess an initial value and increase it until it is big enough.
		size_t maxClusters = 1;
		size_t bufferSize = sizeof(RETRIEVAL_POINTERS_BUFFER) + (2 * (maxClusters - 1) * sizeof(LARGE_INTEGER)); // 2 because pairs of VCN and LCN values are returned in this buffer. The first pair is in the struct at the beginning
		BYTE *output = new BYTE[bufferSize];

		DWORD numBytesReturned = 0;
		BOOL ok = false;
		DWORD error = 0;

		while (bufferSize < std::numeric_limits<DWORD>::max() &&
			(ok = DeviceIoControl(fileHandle, FSCTL_GET_RETRIEVAL_POINTERS, &input, sizeof(input), output, (DWORD)bufferSize, &numBytesReturned, NULL)) == FALSE &&
			(error = GetLastError()) == ERROR_MORE_DATA)
		{
			delete[] output;

			maxClusters *= 2;
			bufferSize = sizeof(RETRIEVAL_POINTERS_BUFFER) + (2 * (maxClusters - 1) * sizeof(LARGE_INTEGER));

			output = new BYTE[bufferSize];
		}

		if (ok)
		{
			RETRIEVAL_POINTERS_BUFFER* p = (RETRIEVAL_POINTERS_BUFFER*)output;
			if (p->ExtentCount > 0 && p->StartingVcn.QuadPart == 0)
			{
				//numFragments = p->ExtentCount;
				clusterNumber = p->Extents[0].Lcn.QuadPart;
			}
		}

		delete[] output;

		CloseHandle(fileHandle);
	}

	return clusterNumber;
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
	std::srand(unsigned(std::time(0)));

	std::list<std::wstring> folders;
	std::list<std::wstring> options;

	std::list<std::wstring> files;
	for (auto i = 1; i < argc; ++i)
	{
		std::wstring argument(argv[i]);

		if (argument[0] == L'-')
		{
			options.push_back(argument.substr(1));
		}
		else
		{
			folders.push_back(argument);
		}
	}

	for (auto& folder : folders)
	{
		auto _files = filesInFolder(folder);
		for (auto& f : _files)
		{
			files.push_back(f);
		}
	}

	if (!files.empty())
	{
		uint8_t dummy_hash = 0;
		auto hash_files = [&dummy_hash](const auto& file_list, size_t maxSize = std::numeric_limits<size_t>::max()) {
			for (auto& f : file_list)
			{
				dummy_hash ^= filehash(f, maxSize);
			}
		};

		typedef std::pair<std::wstring, uint64_t> FileCluster;
		std::vector<FileCluster> files_with_clusters;
		for (auto& f : files)
		{
			files_with_clusters.emplace_back(f, GetStartCluster(f));
		}
		std::stable_sort(files_with_clusters.begin(), files_with_clusters.end(), [](const auto& left, const auto& right) { return left.second < right.second; });

		std::vector<std::wstring> alphabetical_files(files.begin(), files.end());
		std::vector<std::wstring> random_files(files.begin(), files.end());
		std::vector<std::wstring> cluster_files;
		for (auto& f : files_with_clusters)
		{
			cluster_files.push_back(f.first);
		}

		std::sort(alphabetical_files.begin(), alphabetical_files.end());
		std::random_shuffle(random_files.begin(), random_files.end());

		// hashing of first 64k
		size_t _64k = 64 * 1024;
		std::wcout << L"hashing files (64k, alphabetic order) took " << time(std::bind(hash_files, alphabetical_files, _64k)).count() << L"s" << std::endl;
		std::wcout << L"hashing files (64k, random order) took " << time(std::bind(hash_files, random_files, _64k)).count() << L"s" << std::endl;
		std::wcout << L"hashing files (64k, cluster order) took " << time(std::bind(hash_files, files, _64k)).count() << L"s" << std::endl;
		std::wcout << L"hashing files (64k, original order) took " << time(std::bind(hash_files, cluster_files, _64k)).count() << L"s" << std::endl;

		// full file hashing
		std::wcout << L"hashing files (alphabetic order) took " << time(std::bind(hash_files, alphabetical_files)).count() << L"s" << std::endl;
		std::wcout << L"hashing files (random order) took " << time(std::bind(hash_files, random_files)).count() << L"s" << std::endl;
		std::wcout << L"hashing files (cluster order) took " << time(std::bind(hash_files, files)).count() << L"s" << std::endl;
		std::wcout << L"hashing files (original order) took " << time(std::bind(hash_files, cluster_files)).count() << L"s" << std::endl;
	}
	else
	{
		std::wcout << L"no files found in the given folders" << std::endl;
	}
}
