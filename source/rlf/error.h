
namespace rlf
{
	struct ErrorInfo
	{
		const char* Location = nullptr;
		std::string Message = "";
	};
	struct ErrorState
	{
		bool Success = true;
		bool Warning = false;
		ErrorInfo Info;
	};
}
