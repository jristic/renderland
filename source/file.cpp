
namespace File
{

void ReadFile(
	const char* filename,
	char* out_buffer,
	int* out_len,
	int max_len)
{
	FILE* file = fopen(filename, "r");
	Assert(file, "couldn't open file: %s", filename);

	size_t bytes_read = fread(out_buffer, 1, max_len, file);
	Assert(
		feof(file),
		"file %s is too large, max is %d bytes",
		filename, max_len);

	out_buffer[bytes_read] = '\0';

	fclose(file);

	*out_len = (int)bytes_read;
}

}