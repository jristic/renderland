
namespace rlf
{
	RenderDescription* ParseBuffer(
		const char* buffer,
		u32 bufferSize,
		const char* workingDir,
		ErrorState* es);

	void ReleaseData(RenderDescription* data);
}
