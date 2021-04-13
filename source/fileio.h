
namespace fileio {

void MakeDirectory(const char* directory);

HANDLE CreateFileOverwrite(const char* fileName, u32 desiredAccess);
HANDLE CreateFileTryNew(const char* fileName, u32 desiredAccess);
HANDLE OpenFileAlways(const char* fileName, u32 desiredAccess);
HANDLE OpenFileOptional(const char* fileName, u32 desiredAccess);

void DeleteFile(const char* fileName);

u32 GetFileSize(HANDLE file);

void WriteFile(HANDLE file, const void* payload, u32 payloadSize);
void ReadFile(HANDLE file, void* outBuffer, u32 bytesToRead);
void ReadFileAtOffset(HANDLE file, void* outBuffer, u32 readOffset, u32 bytesToRead);

void ResetFilePointer(HANDLE file);

void GetCurrentDirectory(char* outDirectoryBuffer, u32 bufferSize);
void GetModuleFileName(HMODULE module, char* outFileNameBuffer, u32 bufferSize);

} // namespace fileio
