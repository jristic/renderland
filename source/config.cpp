
namespace config
{


void LoadConfig(const char* configPath, Parameters* outConfig)
{
	Assert(outConfig, "Null not valid.");
	HANDLE file = fileio::OpenFileOptional(configPath, GENERIC_READ);
	if (file == INVALID_HANDLE_VALUE)
	{
		// No config file exists, default to no file for now
		outConfig->ShaderPath[0] = '\0';
		return;
	}
	
	u32 fileSize = fileio::GetFileSize(file);
	char* buffer = (char*)malloc(fileSize);	
	fileio::ReadFile(file, buffer, fileSize);

	std::string contents(buffer, fileSize);

	auto ProcessLine = [](std::string& line, Parameters* outConfig)
	{
		size_t split = line.find("=");
		if (split != std::string::npos)
		{
			std::string label = line.substr(0, split);
			std::string value = line.substr(split+1);
			if (label == "ShaderPath")
			{
				memcpy(outConfig->ShaderPath, value.c_str(), value.length());
				outConfig->ShaderPath[value.length()] = '\0';
			}
			else
			{
				// TODO: Unhandled, should we error?
			}
		}
	};

	size_t last = 0;
	size_t next = 0;
	while ((next = contents.find("\n", last)) != std::string::npos)
	{
		std::string line = contents.substr(last, next-last);
		ProcessLine(line, outConfig);
		last = next + 1;
	}
	std::string line = contents.substr(last);
	ProcessLine(line, outConfig);

	free(buffer);
	CloseHandle(file);
}

void SaveConfig(const char* configPath, const Parameters* config)
{
	HANDLE file = fileio::CreateFileOverwrite(configPath, GENERIC_WRITE);
	std::string contents = std::string("ShaderPath=") + config->ShaderPath + "\n";
	fileio::WriteFile(file, contents.c_str(), (u32)contents.length());
	CloseHandle(file);
}


}// namespace config
