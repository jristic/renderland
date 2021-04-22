
namespace rlf
{
	struct ParseErrorState 
	{
		bool ParseSuccess;
		std::string ErrorMessage;
	};

	RenderDescription* ParseFile(
		const char* filename,
		ParseErrorState* errorState);

	void ReleaseData(RenderDescription* data);
}
