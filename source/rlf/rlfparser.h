
namespace rlf
{
	struct ParseErrorState 
	{
		bool ParseSuccess;
		ErrorInfo Info;
	};

	RenderDescription* ParseBuffer(
		const char* buffer,
		u32 bufferSize,
		const char* workingDir,
		ParseErrorState* errorState);

	void ReleaseData(RenderDescription* data);
}
