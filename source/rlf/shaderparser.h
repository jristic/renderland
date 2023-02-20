
namespace rlf {
namespace shader {

	void ParseBuffer(
		const char* buffer,
		u32 bufferSize,
		std::unordered_map<std::string, u32>& structSizes,
		ErrorState* es);

}
}
