
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
	TOKEN_INTEGER_LITERAL,
	TOKEN_IDENTIFIER,
	TOKEN_STRING,
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
	ParseErrorState* es;
} *GPS;

void ParserError(const char* str, ...)
{
	char buf[512];
	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,512,str,ptr);
	va_end(ptr);
	GPS->es->ParseSuccess = false;
	GPS->es->ErrorMessage = buf;
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

Token PeekNextToken(
	BufferIter& b)
{
	ParserAssert(b.next != b.end, "unexpected end-of-buffer.");

	char firstChar = *b.next;
	++b.next;

	switch (firstChar)
	{
	case '(':
		return TOKEN_LPAREN;
	case ')':
		return TOKEN_RPAREN;
	case '{':
		return TOKEN_LBRACE;
	case '}':
		return TOKEN_RBRACE;
	case ',':
		return TOKEN_COMMA;
	case '=':
		return TOKEN_EQUALS;
	case '-':
		return TOKEN_MINUS;
	case ';':
		return TOKEN_SEMICOLON;
	case '"':
		while (b.next < b.end && *b.next != '"') {
			++b.next;
		}
		++b.next; // pass the closing quotes
		return TOKEN_STRING;
	case '0': case '1': case '2': case '3': case '4': 
	case '5': case '6': case '7': case '8': case '9':
		while(b.next < b.end && isdigit(*b.next)) {
			++b.next;
		} 
		return TOKEN_INTEGER_LITERAL;
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
	case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
	case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
	case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
	case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
	case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
	case 'Y': case 'Z':
		// identifiers have to start with a letter, but can contain numbers
		while (b.next < b.end && (isalpha(*b.next) || isdigit(*b.next))) {
			++b.next;
		} 
		return TOKEN_IDENTIFIER;
	default:
		ParserError("unexpected character when reading token: %c", *b.next);
		return TOKEN_INVALID;
	}
}

void ConsumeToken(
	Token tok,
	BufferIter& b)
{
	SkipWhitespace(b);
	Token foundTok;
	foundTok = PeekNextToken(b);
	ParserAssert(foundTok == tok, "unexpected token, expected %d found %d",
		tok, foundTok);
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
	ParserAssert(tok == TOKEN_IDENTIFIER, "unexpected token (wanted identifier)");

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
	ParserAssert(tok == TOKEN_STRING, "unexpected token (wanted string)");

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
	ParserAssert(tok == TOKEN_INTEGER_LITERAL, "unexpected token (wanted string)");

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
	ParserAssert(tok == TOKEN_INTEGER_LITERAL, "unexpected token (wanted string)");

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
		auto iter = ps.shaderMap.find(id);
		ParserAssert(iter != ps.shaderMap.end(), "couldn't find shader %.*s", 
			id.len, id.base);
		return iter->second;
	}
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
		auto iter = ps.dcMap.find(id);
		ParserAssert(iter != ps.dcMap.end(), "couldn't find dispatch %.*s", 
			id.len, id.base);
		return iter->second;
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
			ParserAssert(ps.shaderMap.find(nameId) == ps.shaderMap.end(),
				"%.*s already defined", nameId.len, nameId.base);
			ps.shaderMap[nameId] = cs;
		}
		else if (structureId == "Dispatch")
		{
			Dispatch* dc = ConsumeDispatchDef(b, ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.dcMap.find(nameId) == ps.dcMap.end(), "%.*s already defined", 
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


RenderDescription* ParseBuffer(
	char* buffer,
	int buffer_len,
	ParseErrorState* errorState)
{
	ParseState ps;
	ps.rd = new RenderDescription();
	ps.es = errorState;
	ps.b = { buffer, buffer + buffer_len };

	GPS = &ps;
	errorState->ParseSuccess = true;

	HANDLE parseThread = CreateThread(nullptr, 0, &ParseMain, &ps, 0, nullptr);
	WaitForSingleObject(parseThread, INFINITE);
	CloseHandle(parseThread);

	GPS = nullptr;

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
	{
		delete dc;
	}
	for (ComputeShader* cs : data->Shaders)
	{
		delete cs;
	}
	delete data;
}

#undef ParserAssert

} // namespace rlf
