
namespace fileio {

void MakeDirectory(const char* directory)
{
	BOOL success = ::CreateDirectory(directory, nullptr);
	if (!success)
	{
		DWORD lastError = GetLastError();
		Assert(lastError == ERROR_ALREADY_EXISTS, 
			"failed to create directory %s, error=%d",
			directory, lastError);
	}
}

HANDLE CreateFileOverwrite(const char* fileName, u32 desiredAccess)
{
	HANDLE handle = ::CreateFileA(fileName, desiredAccess, 0, nullptr, 
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	Assert(handle != INVALID_HANDLE_VALUE, "Failed to create file %s, error=%d",
		fileName, GetLastError());
	return handle;
}

HANDLE CreateFileTryNew(const char* fileName, u32 desiredAccess)
{
	HANDLE handle = ::CreateFileA(fileName, desiredAccess, 0, nullptr, 
		CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
	{
		DWORD lastError = GetLastError();
		Assert(lastError == ERROR_FILE_EXISTS,
			"Creating new file %s failed unexpectedly, error=%d",
			fileName, lastError);
	}
	return handle;
}

HANDLE OpenFileAlways(const char* fileName, u32 desiredAccess)
{
	HANDLE handle = ::CreateFileA(fileName, desiredAccess, 0, nullptr, 
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	Assert(handle != INVALID_HANDLE_VALUE, "Failed to open existing file %s, error=%d",
		fileName, GetLastError());
	return handle;
}

HANDLE OpenFileOptional(const char* fileName, u32 desiredAccess)
{
	HANDLE handle = ::CreateFileA(fileName, desiredAccess, 0, nullptr, 
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
	{
		DWORD lastError = GetLastError();
		Assert(lastError == ERROR_FILE_NOT_FOUND,
			"Opening file %s failed unexpectedly, error=%d",
			fileName, lastError);
	}
	return handle;
}

void DeleteFile(const char* fileName)
{
	bool success = ::DeleteFile(fileName);
	DWORD lastError = GetLastError();
	Assert(success || lastError == ERROR_FILE_NOT_FOUND, 
		"Failed to delete %s, error=%d",
		fileName, lastError);
}

u32 GetFileSize(HANDLE file)
{
	LARGE_INTEGER large;
	BOOL success = ::GetFileSizeEx(file, &large);
	Assert(success, "Failed to get file size, error=%d", GetLastError());
	Assert(large.QuadPart < UINT_MAX, "File is too large, not supported");
	return large.LowPart;
}

void WriteFile(HANDLE file, const void* payload, u32 payloadSize)
{
	DWORD bytesWritten;
	BOOL success = ::WriteFile(file, payload, payloadSize, &bytesWritten, NULL);
	Assert(success, "Failed to write file");
	Assert(bytesWritten == payloadSize, "Failed to write full amount");
}

void ReadFile(HANDLE file, void* readBuffer, u32 bytesToRead)
{
	DWORD bytesRead;
	BOOL success = ::ReadFile(file, readBuffer, bytesToRead, &bytesRead,
		nullptr);
	Assert(success, "Failed to read file, error=%d", GetLastError());
	Assert(bytesRead == bytesToRead, "Didn't read full file, error=%d",
		GetLastError());
}

void ReadFileAtOffset(HANDLE file, void* readBuffer, u32 readOffset, u32 bytesToRead)
{
	OVERLAPPED ovr = {};
	ovr.Offset = readOffset;
	DWORD bytesRead;
	BOOL success = ::ReadFile(file, readBuffer, bytesToRead, &bytesRead, &ovr);
	Assert(success, "Failed to read file, error=%d", GetLastError());
	Assert(bytesRead == bytesToRead, "Didn't read full file, error=%d",
		GetLastError());
}

void ResetFilePointer(HANDLE file)
{
	DWORD result = ::SetFilePointer(file, 0, NULL, FILE_BEGIN);
	Assert(result != INVALID_SET_FILE_POINTER, "Failed to set file pointer, error=%d",
		GetLastError());
}

void GetCurrentDirectory(char* outDirectoryBuffer, u32 bufferSize)
{
	DWORD copiedBytes = ::GetCurrentDirectory(bufferSize, outDirectoryBuffer);
	Assert(copiedBytes > 0, "Failed to get current directory, error=%d", GetLastError());
	Assert(copiedBytes <= bufferSize, "Current directory path too long, %d", copiedBytes);
}

void GetModuleFileName(HMODULE module, char* outFileNameBuffer, u32 bufferSize)
{
	DWORD copiedSize = ::GetModuleFileName(module, outFileNameBuffer, bufferSize);
	if (copiedSize == 0)
	{
		Assert(false, "Error: failed to get module path. \n");
	}
	else if (copiedSize == bufferSize &&
		GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		Assert(false, "Error: buffer too short for module path. \n");
	}
}

} // namespace fileio
