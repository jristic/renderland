
namespace rlf
{
	RenderDescription* ParseBuffer(
		char* buffer,
		int buffer_len);

	void ReleaseData(RenderDescription* data);
}
