
namespace rlf
{
	struct ParseErrorState 
	{
		bool ParseSuccess;
		std::string ErrorMessage;
	};

	RenderDescription* ParseFile(
		const char* filename,
		const char* workingDir,
		ParseErrorState* errorState);

	void ReleaseData(RenderDescription* data);
}
