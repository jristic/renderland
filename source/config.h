
namespace config
{

struct Parameters
{
	char FilePath[256];
	bool Maximized;
	i32 WindowPosX;
	i32 WindowPosY;
	i32 WindowWidth;
	i32 WindowHeight;
	i32 LayoutVersionApplied;
};

void LoadConfig(const char* configPath, Parameters* outConfig);
void SaveConfig(const char* configPath, const Parameters* config);

}// namespace config
