
namespace config
{

struct Parameters
{
	char ShaderPath[256];
};

void LoadConfig(const char* configPath, Parameters* outConfig);
void SaveConfig(const char* configPath, const Parameters* config);

}// namespace config
