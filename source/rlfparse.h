
namespace rlf
{
	struct ParseErrorState 
	{
		bool ParseSuccess;
		std::string ErrorMessage;
	};

	RenderDescription* ParseBuffer(
		char* buffer,
		int buffer_len,
		ParseErrorState* errorState);

	void ReleaseData(RenderDescription* data);
}
