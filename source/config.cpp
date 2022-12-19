
namespace config
{


void LoadConfig(const char* configPath, Parameters* outConfig)
{
	Assert(outConfig, "Null not valid.");
	HANDLE file = fileio::OpenFileOptional(configPath, GENERIC_READ);
	if (file == INVALID_HANDLE_VALUE)
	{
		// No config file exists, assume the config was already filled with defaults.
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
			if (label == "FilePath")
			{
				memcpy(outConfig->FilePath, value.c_str(), value.length());
				outConfig->FilePath[value.length()] = '\0';
			}
			else if (label == "Maximized")
			{
				outConfig->Maximized = value == "true";
			}
			else if (label == "WindowPosX")
			{
				int val = atoi(value.c_str());
				if (val != 0) 
					outConfig->WindowPosX = val;
			}
			else if (label == "WindowPosY")
			{
				int val = atoi(value.c_str());
				if (val != 0) 
					outConfig->WindowPosY = val;
			}
			else if (label == "WindowWidth")
			{
				int val = atoi(value.c_str());
				if (val > 0) 
					outConfig->WindowWidth = val;
			}
			else if (label == "WindowHeight")
			{
				int val = atoi(value.c_str());
				if (val > 0) 
					outConfig->WindowHeight = val;
			}
			else if (label == "LayoutVersionApplied")
			{
				int val = atoi(value.c_str());
				if (val > 0)
					outConfig->LayoutVersionApplied = val;
			}
			else
			{
				Prompt("Invalid config item: %s", label.c_str());
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

void SaveConfig(const char* configPath, const Parameters* cfg)
{
	HANDLE file = fileio::CreateFileOverwrite(configPath, GENERIC_WRITE);
	char buf[2048];
	snprintf(buf, 2048,
		"FilePath=%s\nMaximized=%s\nWindowPosX=%i\nWindowPosY=%i\n"
		"WindowWidth=%i\nWindowHeight=%i\nLayoutVersionApplied=%i\n", 
		cfg->FilePath, cfg->Maximized ? "true" : "false", cfg->WindowPosX, 
		cfg->WindowPosY, cfg->WindowWidth, cfg->WindowHeight, 
		cfg->LayoutVersionApplied);
	fileio::WriteFile(file, buf, (u32)strlen(buf));
	CloseHandle(file);
}


}// namespace config
