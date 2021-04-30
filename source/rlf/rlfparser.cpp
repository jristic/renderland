
namespace rlf
{

/******************************** Parser notes /********************************
	Parse code happens entirely in a separate thread, so that we can abort at 
	any time due to parse errors. That means the stack isn't cleaned up in 
	case of an error, so make sure no dynamic allocations are never contained
	on the stack. Any allocations which are used for the RLF representation 
	should be immediately recorded in the RenderDescription, which will be 
	freed in ReleaseData. Immediately in this context means before any further
	parsing occurs. Any STL containers used for intermediate state tracking 
	should live in the ParseState and not on the stack so that they	are 
	cleaned up properly. 
	Also watch out for dangling iterators from STL containers. On destruction
	of the container you'll likely get an access violation from it trying to 
	orphan what it thinks is a still-existing iterator. 
*******************************************************************************/

enum Token
{
	TOKEN_INVALID,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_COMMA,
	TOKEN_EQUALS,
	TOKEN_MINUS,
	TOKEN_SEMICOLON,
	TOKEN_AT,
	TOKEN_INTEGER_LITERAL,
	TOKEN_IDENTIFIER,
	TOKEN_STRING,
};

const char* TokenNames[] =
{
	"invalid",
	"lparen",
	"rparen",
	"lbrace",
	"rbrace",
	"comma",
	"equals",
	"minus",
	"semicolon",
	"integer literal",
	"identifier",
	"string",
};

struct BufferString
{
	const char* base;
	size_t len;
};

bool operator==(const BufferString& l, const BufferString& r)
{
	if (l.len != r.len)
		return false;
	for (size_t i = 0 ; i < l.len ; ++i)
	{
		if (l.base[i] != r.base[i])
			return false;
	}
	return true;
}

struct BufferStringHash
{
	size_t operator()(const BufferString& s) const
	{
		unsigned long h;
		unsigned const char* us;
		us = (unsigned const char *) s.base;
		unsigned const char* end;
		end = (unsigned const char *) s.base + s.len;
		h = 0;
		while(us < end) {
			h = h * 97 + *us;
			us++;
		}
		return h; 
	}
};

struct BufferIter
{
	char* next;
	char* end;
};

bool operator==(const BufferString id, const char* string)
{
	if (id.len != strlen(string))
		return false;
	for (int i = 0 ; i < id.len ; ++i)
		if (id.base[i] != string[i])
			return false;
	return true;
}

struct ParseState 
{
	BufferIter b;
	RenderDescription* rd;
	std::unordered_map<BufferString, Dispatch*, BufferStringHash> dcMap;
	std::unordered_map<BufferString, ComputeShader*, BufferStringHash> shaderMap;
	struct Resource {
		bool isTexture;
		void* m;
	};
	std::unordered_map<BufferString, Resource, BufferStringHash> resMap;
	ParseErrorState* es;

	const char* filename;
	char* bufferStart;
	Token fcLUT[128];
} *GPS;

void ParserError(const char* str, ...)
{
	char buf[512];
	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,512,str,ptr);
	va_end(ptr);

	ParseState* ps = GPS;
	ps->es->ParseSuccess = false;
	ps->es->ErrorMessage = buf;

	u32 line = 1;
	u32 chr = 0;
	char* lineStart = ps->bufferStart;
	for (char* b = ps->bufferStart; b < ps->b.next ; ++b)
	{
		if (*b == '\n')
		{
			lineStart = b+1;
			++line;
			chr = 1;
		}
		else
		{
			++chr;
		}
	}

	sprintf_s(buf, 512, "%s(%u,%u): error: ", ps->filename, line, chr);
	ps->es->ErrorMessage = std::string(buf) + ps->es->ErrorMessage + "\n";

	char* lineEnd = lineStart;
	while (lineEnd < ps->b.end && *lineEnd != '\n')
		++lineEnd;

	sprintf_s(buf, 512, "line: %.*s", (u32)(lineEnd-lineStart), lineStart);
	ps->es->ErrorMessage += buf;

	// This if statement is meaningless (rd will always be non-null here), but
	//	the compiler doesn't know that. Otherwise it will either generate 
	// 	C4702 (unreachable code) for any code that comes after a ParserError in 
	// 	optimized builds, or will generate C4715 (not all paths return value)
	// 	in non-optimized builds if you remove the unreachable code. 
	if (GPS->rd) 
		ExitThread(1);
}

#define ParserAssert(expression, message, ...) 	\
do {											\
	if (!(expression)) {						\
		ParserError(message, ##__VA_ARGS__);	\
	}											\
} while (0);									\


void SkipWhitespace(
	BufferIter& b)
{
	while (b.next < b.end && isspace(*b.next)) {
		++b.next;
	}
}

void ParseStateInit(ParseState* ps)
{
	Token* fcLUT = ps->fcLUT;
	for (u32 fc = 0 ; fc < 128 ; ++fc)
		fcLUT[fc] = TOKEN_INVALID;
	fcLUT['('] = TOKEN_LPAREN;
	fcLUT[')'] = TOKEN_RPAREN;
	fcLUT['{'] = TOKEN_LBRACE;
	fcLUT['}'] = TOKEN_RBRACE;
	fcLUT[','] = TOKEN_COMMA;
	fcLUT['='] = TOKEN_EQUALS;
	fcLUT['-'] = TOKEN_MINUS;
	fcLUT[';'] = TOKEN_SEMICOLON;
	fcLUT['@'] = TOKEN_AT;
	fcLUT['"'] = TOKEN_STRING;
	for (u32 fc = 0 ; fc < 128 ; ++fc)
	{
		if (isalpha(fc))
			fcLUT[fc] = TOKEN_IDENTIFIER;
		else if (isdigit(fc))
			fcLUT[fc] = TOKEN_INTEGER_LITERAL;
	} 
	fcLUT['_'] = TOKEN_IDENTIFIER;
}

Token PeekNextToken(
	BufferIter& b)
{
	ParserAssert(b.next != b.end, "unexpected end-of-buffer.");

	char firstChar = *b.next;
	++b.next;

	Token tok = GPS->fcLUT[firstChar];

	switch (tok)
	{
	case TOKEN_LPAREN:
	case TOKEN_RPAREN:
	case TOKEN_LBRACE:
	case TOKEN_RBRACE:
	case TOKEN_COMMA:
	case TOKEN_EQUALS:
	case TOKEN_MINUS:
	case TOKEN_SEMICOLON:
	case TOKEN_AT:
		break;
	case TOKEN_INTEGER_LITERAL:
		while(b.next < b.end && isdigit(*b.next)) {
			++b.next;
		}
		break;
	case TOKEN_IDENTIFIER:
		// identifiers have to start with a letter, but can contain numbers
		while (b.next < b.end && (isalpha(*b.next) || isdigit(*b.next) || 
			*b.next == '_')) 
		{
			++b.next;
		}
		break;
	case TOKEN_STRING:
		while (b.next < b.end && *b.next != '"') {
			++b.next;
		}
		ParserAssert(b.next < b.end, "End-of-buffer before closing parenthesis.");
		++b.next; // pass the closing quotes
		break;
	case TOKEN_INVALID:
	default:
		ParserError("unexpected character when parsing token: %c", firstChar);
		break;
	}

	return tok;
}

void ConsumeToken(
	Token tok,
	BufferIter& b)
{
	SkipWhitespace(b);
	Token foundTok;
	foundTok = PeekNextToken(b);
	ParserAssert(foundTok == tok, "unexpected token, expected %s but found %s",
		TokenNames[tok], TokenNames[foundTok]);
}

bool TryConsumeToken(
	Token tok,
	BufferIter& b)
{
	BufferIter nb = b;
	SkipWhitespace(nb);
	Token foundTok;
	foundTok = PeekNextToken(nb);
	bool found = (foundTok == tok);
	if (found)
	{
		b = nb;
	}
	return found;
}

BufferString ConsumeIdentifier(
	BufferIter& b)
{
	SkipWhitespace(b);
	BufferIter start = b; 
	Token tok = PeekNextToken(b);
	ParserAssert(tok == TOKEN_IDENTIFIER, "unexpected %s (wanted identifier)", 
		TokenNames[tok]);

	BufferString id;
	id.base = start.next;
	id.len = b.next - start.next;

	return id;
}

BufferString ConsumeString(
	BufferIter& b)
{
	SkipWhitespace(b);
	BufferIter start = b;
	Token tok = PeekNextToken(b);
	ParserAssert(tok == TOKEN_STRING, "unexpected %s (wanted string)", 
		TokenNames[tok]);

	BufferString str;
	str.base = start.next + 1;
	str.len = b.next - start.next - 2;

	return str;	
}

bool ConsumeBool(
	BufferIter& b)
{
	BufferString value = ConsumeIdentifier(b);
	if (value == "true")
		return true;
	else if (value == "false")
		return false;

	ParserError("expected bool (true/false), got: %.*s", value.len, value.base);
	return false;
}

i32 ConsumeIntLiteral(
	BufferIter& b)
{
	SkipWhitespace(b);
	bool negative = false;
	if (TryConsumeToken(TOKEN_MINUS, b))
	{
		negative = true;
		SkipWhitespace(b);
	}
	BufferIter nb = b;
	Token tok = PeekNextToken(b);
	ParserAssert(tok == TOKEN_INTEGER_LITERAL, "unexpected %s (wanted integer literal)",
		TokenNames[tok]);

	i32 val = 0;
	do 
	{
		val *= 10;
		val += (*nb.next - '0');
		++nb.next;
	} while (nb.next < b.next);

	return negative ? -val : val;
}

u32 ConsumeUintLiteral(
	BufferIter& b)
{
	SkipWhitespace(b);
	if (TryConsumeToken(TOKEN_MINUS, b))
	{
		ParserError("Unsigned int expected, '-' invalid here");
	}
	BufferIter nb = b;
	Token tok = PeekNextToken(b);
	ParserAssert(tok == TOKEN_INTEGER_LITERAL, "unexpected %s (wanted integer literal)",
		TokenNames[tok]);

	u32 val = 0;
	do 
	{
		val *= 10;
		val += (*nb.next - '0');
		++nb.next;
	} while (nb.next < b.next);

	return val;
}

const char* AddStringToDescriptionData(BufferString str, RenderDescription* rd)
{
	auto pair = rd->Strings.insert(std::string(str.base, str.len));
	return pair.first->c_str();
}


SystemValue ConsumeSystemValue(BufferIter& b)
{
	BufferString sysId = ConsumeIdentifier(b);
	if (sysId == "BackBuffer")
	{
		return SystemValue::BackBuffer;
	}
	ParserError("Invalid system value: %.*s", sysId.len, sysId.base);
	return SystemValue::Invalid;
}

ComputeShader* ConsumeComputeShaderDef(
	BufferIter& b,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(TOKEN_LBRACE, b);

	ComputeShader* cs = new ComputeShader();
	rd->Shaders.push_back(cs);

	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		if (fieldId == "ShaderPath")
		{
			ConsumeToken(TOKEN_EQUALS, b);
			BufferString value = ConsumeString(b);
			cs->ShaderPath = AddStringToDescriptionData(value, rd);
		}
		else if (fieldId == "EntryPoint")
		{
			ConsumeToken(TOKEN_EQUALS, b);
			BufferString value = ConsumeString(b);
			cs->EntryPoint = AddStringToDescriptionData(value, rd);
		}
		else
		{
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

		ConsumeToken(TOKEN_SEMICOLON, b);

		if (TryConsumeToken(TOKEN_RBRACE, b))
		{
			break;
		}
	}

	return cs;
}

ComputeShader* ConsumeComputeShaderRefOrDef(
	BufferIter& b,
	ParseState& ps)
{
	BufferString id = ConsumeIdentifier(b);
	if (id == "ComputeShader")
	{
		return ConsumeComputeShaderDef(b, ps);
	}
	else
	{
		ParserAssert(ps.shaderMap.count(id) != 0, "couldn't find shader %.*s", 
			id.len, id.base);
		return ps.shaderMap[id];
	}
}

Buffer* ConsumeBufferDef(
	BufferIter& b,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(TOKEN_LBRACE, b);

	Buffer* buf = new Buffer();
	rd->Buffers.push_back(buf);

	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		if (fieldId == "ElementSize")
		{
			ConsumeToken(TOKEN_EQUALS, b);
			buf->ElementSize = ConsumeUintLiteral(b);
		}
		else if (fieldId == "ElementCount")
		{
			ConsumeToken(TOKEN_EQUALS, b);
			buf->ElementCount = ConsumeUintLiteral(b);
		}
		else if (fieldId == "InitToZero")
		{
			ConsumeToken(TOKEN_EQUALS, b);
			buf->InitToZero = ConsumeBool(b);
		}
		else
		{
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

		ConsumeToken(TOKEN_SEMICOLON, b);

		if (TryConsumeToken(TOKEN_RBRACE, b))
		{
			break;
		}
	}

	return buf;
}

Texture* ConsumeTextureDef(
	BufferIter& b,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(TOKEN_LBRACE, b);

	Texture* tex = new Texture();
	rd->Textures.push_back(tex);

	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		if (fieldId == "Size")
		{
			ConsumeToken(TOKEN_EQUALS, b);
			ConsumeToken(TOKEN_LBRACE, b);
			tex->Size.x = ConsumeUintLiteral(b);
			ConsumeToken(TOKEN_COMMA, b);
			tex->Size.y = ConsumeUintLiteral(b);
			ConsumeToken(TOKEN_RBRACE, b);
		}
		else if (fieldId == "InitToZero")
		{
			ConsumeToken(TOKEN_EQUALS, b);
			tex->InitToZero = ConsumeBool(b);
		}
		else
		{
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

		ConsumeToken(TOKEN_SEMICOLON, b);

		if (TryConsumeToken(TOKEN_RBRACE, b))
		{
			break;
		}
	}

	return tex;
}

Dispatch* ConsumeDispatchDef(
	BufferIter& b,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(TOKEN_LBRACE, b);

	Dispatch* dc = new Dispatch();
	rd->Dispatches.push_back(dc);

	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		if (fieldId == "Shader")
		{
			ConsumeToken(TOKEN_EQUALS, b);
			ComputeShader* cs = ConsumeComputeShaderRefOrDef(b, ps);
			dc->Shader = cs;
		}
		else if (fieldId == "ThreadPerPixel")
		{
			ConsumeToken(TOKEN_EQUALS, b);
			dc->ThreadPerPixel = ConsumeBool(b);
		}
		else if (fieldId == "Groups")
		{
			ConsumeToken(TOKEN_EQUALS, b);
			ConsumeToken(TOKEN_LBRACE, b);
			dc->Groups.x = ConsumeUintLiteral(b);
			ConsumeToken(TOKEN_COMMA, b);
			dc->Groups.y = ConsumeUintLiteral(b);
			ConsumeToken(TOKEN_COMMA, b);
			dc->Groups.z = ConsumeUintLiteral(b);
			ConsumeToken(TOKEN_RBRACE, b);
		}
		else if (fieldId == "Bind")
		{
			BufferString bindName = ConsumeIdentifier(b);
			ConsumeToken(TOKEN_EQUALS, b);
			Bind bind;
			bind.BindTarget = AddStringToDescriptionData(bindName, rd);
			if (TryConsumeToken(TOKEN_AT, b))
			{
				bind.Type = BindType::SystemValue;
				bind.SystemBind = ConsumeSystemValue(b);
			}
			else
			{
				BufferString id = ConsumeIdentifier(b);
				ParserAssert(ps.resMap.count(id) != 0, "Couldn't find resource %.*s", 
					id.len, id.base);
				ParseState::Resource& res = ps.resMap[id];
				bind.Type = res.isTexture ? BindType::Texture : BindType::Buffer;
				bind.BufferBind = reinterpret_cast<Buffer*>(res.m);
			}
			dc->Binds.push_back(bind);
		}
		else
		{
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

		ConsumeToken(TOKEN_SEMICOLON, b);

		if (TryConsumeToken(TOKEN_RBRACE, b))
		{
			break;
		}
	}

	return dc;
}

Dispatch* ConsumeDispatchRefOrDef(
	BufferIter& b,
	ParseState& ps)
{
	BufferString id = ConsumeIdentifier(b);
	if (id == "Dispatch")
	{
		return ConsumeDispatchDef(b, ps);
	}
	else
	{
		ParserAssert(ps.dcMap.count(id) != 0, "couldn't find dispatch %.*s", 
			id.len, id.base);
		return ps.dcMap[id];
	}
}

DWORD WINAPI ParseMain(_In_ LPVOID lpParameter)
{
	ParseState& ps = *(ParseState*)lpParameter;
	RenderDescription* rd = ps.rd;
	BufferIter& b = ps.b;

	while (b.next != b.end)
	{
		BufferString structureId = ConsumeIdentifier(b);

		if (structureId == "ComputeShader")
		{
			ComputeShader* cs = ConsumeComputeShaderDef(b, ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.shaderMap.count(nameId) == 0, "Shader %.*s already defined", 
				nameId.len, nameId.base);
			ps.shaderMap[nameId] = cs;
		}
		else if (structureId == "Buffer")
		{
			Buffer* buf = ConsumeBufferDef(b, ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.resMap.count(nameId) == 0,  "Resource %.*s already defined", 
				nameId.len, nameId.base);
			ParseState::Resource& res = ps.resMap[nameId];
			res.isTexture = false;
			res.m = buf;
		}
		else if (structureId == "Texture")
		{
			Texture* tex = ConsumeTextureDef(b,ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.resMap.count(nameId) == 0, "Resource %.*s already defined",
				nameId.len, nameId.base);
			ParseState::Resource& res = ps.resMap[nameId];
			res.isTexture = true;
			res.m = tex;
		}
		else if (structureId == "Dispatch")
		{
			Dispatch* dc = ConsumeDispatchDef(b, ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.dcMap.count(nameId) == 0, "Dispatch %.*s already defined", 
				nameId.len, nameId.base);
			ps.dcMap[nameId] = dc;
		}
		else if (structureId == "Passes")
		{
			ConsumeToken(TOKEN_LBRACE, b);

			while (true)
			{
				Dispatch* dc = ConsumeDispatchRefOrDef(b, ps);
				rd->Passes.push_back(dc);

				if (!TryConsumeToken(TOKEN_COMMA, b))
				{
					break;
				}
			}

			ConsumeToken(TOKEN_RBRACE, b);
		}
		else
		{
			ParserError("Unexpected structure: %.*s", structureId.len, 
				structureId.base);
		}

		// Skip trailing whitespace so we don't miss the exit condition for this loop. 
		SkipWhitespace(b);
	}

	return 0;
}


RenderDescription* ParseFile(
	const char* filename,
	ParseErrorState* errorState)
{
	HANDLE rlf = fileio::OpenFileOptional(filename, GENERIC_READ);

	if (rlf == INVALID_HANDLE_VALUE) // file not found
	{
		errorState->ParseSuccess = false;
		errorState->ErrorMessage = std::string("Couldn't find ") + filename;
		return nullptr;
	}

	u32 rlfSize = fileio::GetFileSize(rlf);

	char* rlfBuffer = (char*)malloc(rlfSize);	
	Assert(rlfBuffer != nullptr, "failed to alloc");

	fileio::ReadFile(rlf, rlfBuffer, rlfSize);

	CloseHandle(rlf);

	ParseState ps;
	ps.rd = new RenderDescription();
	ps.es = errorState;
	ps.b = { rlfBuffer, rlfBuffer + rlfSize };
	ps.bufferStart = rlfBuffer;
	ps.filename = filename;
	ParseStateInit(&ps);

	GPS = &ps;
	errorState->ParseSuccess = true;

	HANDLE parseThread = CreateThread(nullptr, 0, &ParseMain, &ps, 0, nullptr);
	WaitForSingleObject(parseThread, INFINITE);
	CloseHandle(parseThread);

	GPS = nullptr;
	free(rlfBuffer);

	if (ps.es->ParseSuccess == false)
	{
		ReleaseData(ps.rd);
		return nullptr;
	}
	else
	{
		Assert(ps.b.next == ps.b.end, "Didn't consume full buffer.");
		return ps.rd;
	}
}

void ReleaseData(RenderDescription* data)
{
	Assert(data, "Invalid pointer.");

	for (Dispatch* dc : data->Dispatches)
		delete dc;
	for (ComputeShader* cs : data->Shaders)
		delete cs;
	for (Buffer* buf : data->Buffers)
		delete buf;
	for (Texture* tex : data->Textures)
		delete tex;
	delete data;
}

#undef ParserAssert

} // namespace rlf
