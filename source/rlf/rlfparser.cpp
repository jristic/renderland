
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

#define RLF_TOKEN_TUPLE \
	RLF_TOKEN_ENTRY(Invalid) \
	RLF_TOKEN_ENTRY(LParen) \
	RLF_TOKEN_ENTRY(RParen) \
	RLF_TOKEN_ENTRY(LBrace) \
	RLF_TOKEN_ENTRY(RBrace) \
	RLF_TOKEN_ENTRY(Comma) \
	RLF_TOKEN_ENTRY(Equals) \
	RLF_TOKEN_ENTRY(Minus) \
	RLF_TOKEN_ENTRY(Semicolon) \
	RLF_TOKEN_ENTRY(At) \
	RLF_TOKEN_ENTRY(IntegerLiteral) \
	RLF_TOKEN_ENTRY(FloatLiteral) \
	RLF_TOKEN_ENTRY(Identifier) \
	RLF_TOKEN_ENTRY(String) \

#define RLF_TOKEN_ENTRY(name) name,
enum class Token
{
	RLF_TOKEN_TUPLE
};
#undef RLF_TOKEN_ENTRY

#define RLF_TOKEN_ENTRY(name) #name,
const char* TokenNames[] =
{
	RLF_TOKEN_TUPLE
};
#undef RLF_TOKEN_ENTRY

#undef RLF_TOKEN_TUPLE

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
		BindType type;
		void* m;
	};
	std::unordered_map<BufferString, Resource, BufferStringHash> resMap;
	std::unordered_map<BufferString, TextureFormat, BufferStringHash> fmtMap;
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
		fcLUT[fc] = Token::Invalid;
	fcLUT['('] = Token::LParen;
	fcLUT[')'] = Token::RParen;
	fcLUT['{'] = Token::LBrace;
	fcLUT['}'] = Token::RBrace;
	fcLUT[','] = Token::Comma;
	fcLUT['='] = Token::Equals;
	fcLUT['-'] = Token::Minus;
	fcLUT[';'] = Token::Semicolon;
	fcLUT['@'] = Token::At;
	fcLUT['"'] = Token::String;
	for (u32 fc = 0 ; fc < 128 ; ++fc)
	{
		if (isalpha(fc))
			fcLUT[fc] = Token::Identifier;
		else if (isdigit(fc))
			fcLUT[fc] = Token::IntegerLiteral;
	} 
	fcLUT['_'] = Token::Identifier;

	for (u32 i = 0 ; i < (u32)TextureFormat::_Count ; ++i)
	{
		TextureFormat fmt = (TextureFormat)i;
		const char* str = TextureFormatName[i];
		BufferString bstr = { str, strlen(str) };
		Assert(ps->fmtMap.count(bstr) == 0, "hash collision");
		ps->fmtMap[bstr] = fmt;
	}
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
	case Token::LParen:
	case Token::RParen:
	case Token::LBrace:
	case Token::RBrace:
	case Token::Comma:
	case Token::Equals:
	case Token::Minus:
	case Token::Semicolon:
	case Token::At:
		break;
	case Token::IntegerLiteral:
		while(b.next < b.end && isdigit(*b.next)) {
			++b.next;
		}
		if (b.next < b.end && *b.next == '.')
		{
			tok = Token::FloatLiteral;
			++b.next;
			while(b.next < b.end && isdigit(*b.next)) {
				++b.next;
			}
		}
		break;
	case Token::Identifier:
		// identifiers have to start with a letter, but can contain numbers
		while (b.next < b.end && (isalpha(*b.next) || isdigit(*b.next) || 
			*b.next == '_')) 
		{
			++b.next;
		}
		break;
	case Token::String:
		while (b.next < b.end && *b.next != '"') {
			++b.next;
		}
		ParserAssert(b.next < b.end, "End-of-buffer before closing parenthesis.");
		++b.next; // pass the closing quotes
		break;
	case Token::Invalid:
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
		TokenNames[(u32)tok], TokenNames[(u32)foundTok]);
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
	ParserAssert(tok == Token::Identifier, "unexpected %s (wanted identifier)", 
		TokenNames[(u32)tok]);

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
	ParserAssert(tok == Token::String, "unexpected %s (wanted string)", 
		TokenNames[(u32)tok]);

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
	if (TryConsumeToken(Token::Minus, b))
	{
		negative = true;
		SkipWhitespace(b);
	}
	BufferIter nb = b;
	Token tok = PeekNextToken(b);
	ParserAssert(tok == Token::IntegerLiteral, "unexpected %s (wanted integer literal)",
		TokenNames[(u32)tok]);

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
	if (TryConsumeToken(Token::Minus, b))
	{
		ParserError("Unsigned int expected, '-' invalid here");
	}
	BufferIter nb = b;
	Token tok = PeekNextToken(b);
	ParserAssert(tok == Token::IntegerLiteral, "unexpected %s (wanted integer literal)",
		TokenNames[(u32)tok]);

	u32 val = 0;
	do 
	{
		val *= 10;
		val += (*nb.next - '0');
		++nb.next;
	} while (nb.next < b.next);

	return val;
}

float ConsumeFloatLiteral(
	BufferIter& b)
{
	SkipWhitespace(b);
	bool negative = false;
	if (TryConsumeToken(Token::Minus, b))
	{
		negative = true;
		SkipWhitespace(b);
	}
	BufferIter nb = b;
	Token tok = PeekNextToken(b);
	ParserAssert(tok == Token::IntegerLiteral || tok == Token::FloatLiteral, 
		"unexpected %s (wanted float literal)", TokenNames[(u32)tok]);

	bool fracPart = (tok == Token::FloatLiteral);

	double val = 0;
	do 
	{
		val *= 10;
		val += (*nb.next - '0');
		++nb.next;
	} while (nb.next < b.next && *nb.next != '.');

	if (fracPart)
	{
		Assert(nb.next < b.next && *nb.next == '.', "something went wrong, char=%c", 
			*nb.next);
		++nb.next;

		double sub = 0.1;
		while (nb.next < b.next) 
		{
			val += (*nb.next - '0') * sub;
			sub *= 0.1;
			++nb.next;
		}
	}

	return (float)(negative ? -val : val);
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

Filter ConsumeFilter(BufferIter& b)
{
	Filter f = Filter::Point;
	BufferString filterId = ConsumeIdentifier(b);
	if (filterId == "Point")
		f = Filter::Point;
	else if (filterId == "Linear")
		f = Filter::Linear;
	else if (filterId == "Aniso")
		f = Filter::Aniso;
	else
		ParserError("unexpected filter %.*s", filterId.len, filterId.base);
	return f;
}

FilterMode ConsumeFilterMode(BufferIter& b)
{
	FilterMode fm = {};
	ConsumeToken(Token::LBrace, b);
	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		ConsumeToken(Token::Equals, b);
		if (fieldId == "All")
			fm.Min = fm.Mag = fm.Mip = ConsumeFilter(b);
		else if (fieldId == "Min")
			fm.Min = ConsumeFilter(b);
		else if (fieldId == "Mag")
			fm.Mag = ConsumeFilter(b);
		else if (fieldId == "Mip")
			fm.Mip = ConsumeFilter(b);
		else
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);

		if (TryConsumeToken(Token::RBrace, b))
			break;
		else 
			ConsumeToken(Token::Comma, b);
	}
	return fm;
}

AddressMode ConsumeAddressMode(BufferIter& b)
{
	AddressMode addr = AddressMode::Wrap;
	BufferString adrId = ConsumeIdentifier(b);
	if (adrId == "Wrap")
		addr = AddressMode::Wrap;
	else if (adrId == "Mirror")
		addr = AddressMode::Mirror;
	else if (adrId == "MirrorOnce")
		addr = AddressMode::MirrorOnce;
	else if (adrId == "Clamp")
		addr = AddressMode::Clamp;
	else if (adrId == "Border")
		addr = AddressMode::Border;
	else
		ParserError("unexpected address mode %.*s", adrId.len, adrId.base);
	return addr;
}

AddressModeUVW ConsumeAddressModeUVW(BufferIter& b)
{
	AddressModeUVW addr = {};
	ConsumeToken(Token::LBrace,b);
	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		ConsumeToken(Token::Equals, b);
		AddressMode mode = ConsumeAddressMode(b);

		ParserAssert(fieldId.len <= 3, "Invalid [U?V?W?], %.*s", fieldId.len, 
			fieldId.base);

		const char* curr = fieldId.base;
		const char* end = fieldId.base + fieldId.len;
		while (curr < end)
		{
			if (*curr == 'U')
				addr.U = mode;
			else if (*curr == 'V')
				addr.V = mode;
			else if (*curr == 'W')
				addr.W = mode;
			else
				ParserError("unexpected %c, wanted [U|V|W]", *curr);
			++curr;
		}

		if (TryConsumeToken(Token::RBrace, b))
			break;
		else 
			ConsumeToken(Token::Comma, b);
	}
	return addr;
}

ComputeShader* ConsumeComputeShaderDef(
	BufferIter& b,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(Token::LBrace, b);

	ComputeShader* cs = new ComputeShader();
	rd->Shaders.push_back(cs);

	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		if (fieldId == "ShaderPath")
		{
			ConsumeToken(Token::Equals, b);
			BufferString value = ConsumeString(b);
			cs->ShaderPath = AddStringToDescriptionData(value, rd);
		}
		else if (fieldId == "EntryPoint")
		{
			ConsumeToken(Token::Equals, b);
			BufferString value = ConsumeString(b);
			cs->EntryPoint = AddStringToDescriptionData(value, rd);
		}
		else
		{
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

		ConsumeToken(Token::Semicolon, b);

		if (TryConsumeToken(Token::RBrace, b))
			break;
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

	ConsumeToken(Token::LBrace, b);

	Buffer* buf = new Buffer();
	rd->Buffers.push_back(buf);

	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		if (fieldId == "ElementSize")
		{
			ConsumeToken(Token::Equals, b);
			buf->ElementSize = ConsumeUintLiteral(b);
		}
		else if (fieldId == "ElementCount")
		{
			ConsumeToken(Token::Equals, b);
			buf->ElementCount = ConsumeUintLiteral(b);
		}
		else if (fieldId == "InitToZero")
		{
			ConsumeToken(Token::Equals, b);
			buf->InitToZero = ConsumeBool(b);
		}
		else
		{
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

		ConsumeToken(Token::Semicolon, b);

		if (TryConsumeToken(Token::RBrace, b))
			break;
	}

	return buf;
}

Texture* ConsumeTextureDef(
	BufferIter& b,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(Token::LBrace, b);

	Texture* tex = new Texture();
	rd->Textures.push_back(tex);

	// non-zero defaults
	tex->Format = TextureFormat::R8G8B8A8_UNORM;

	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		if (fieldId == "Size")
		{
			ConsumeToken(Token::Equals, b);
			ConsumeToken(Token::LBrace, b);
			tex->Size.x = ConsumeUintLiteral(b);
			ConsumeToken(Token::Comma, b);
			tex->Size.y = ConsumeUintLiteral(b);
			ConsumeToken(Token::RBrace, b);
		}
		else if (fieldId == "Format")
		{
			ConsumeToken(Token::Equals, b);
			BufferString formatId = ConsumeIdentifier(b);
			ParserAssert(ps.fmtMap.count(formatId) != 0, "Couldn't find format %.*s", 
				formatId.len, formatId.base);
			tex->Format = ps.fmtMap[formatId];
		}
		else
		{
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

		ConsumeToken(Token::Semicolon, b);

		if (TryConsumeToken(Token::RBrace, b))
			break;
	}

	return tex;
}

Sampler* ConsumeSamplerDef(
	BufferIter& b,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(Token::LBrace, b);

	Sampler* s = new Sampler();
	rd->Samplers.push_back(s);

	// non-zero defaults
	s->MinLOD = -FLT_MAX;
	s->MaxLOD = FLT_MAX;
	s->MaxAnisotropy = 1;
	s->BorderColor = {1,1,1,1};

	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		if (fieldId == "Filter")
		{
			ConsumeToken(Token::Equals, b);
			s->Filter = ConsumeFilterMode(b);
		}
		else if (fieldId == "AddressMode")
		{
			ConsumeToken(Token::Equals, b);
			s->Address = ConsumeAddressModeUVW(b);
		}
		else if (fieldId == "MipLODBias")
		{
			ConsumeToken(Token::Equals, b);
			s->MipLODBias = ConsumeFloatLiteral(b);
		}
		else if (fieldId == "MaxAnisotropy")
		{
			ConsumeToken(Token::Equals, b);
			s->MaxAnisotropy = ConsumeUintLiteral(b);
		}
		else if (fieldId == "BorderColor")
		{
			ConsumeToken(Token::Equals, b);
			ConsumeToken(Token::LBrace, b);
			s->BorderColor.x = ConsumeFloatLiteral(b);
			ConsumeToken(Token::Comma, b);
			s->BorderColor.y = ConsumeFloatLiteral(b);
			ConsumeToken(Token::Comma, b);
			s->BorderColor.z = ConsumeFloatLiteral(b);
			ConsumeToken(Token::Comma, b);
			s->BorderColor.w = ConsumeFloatLiteral(b);
			ConsumeToken(Token::RBrace, b);
		}
		else if (fieldId == "MinLOD")
		{
			ConsumeToken(Token::Equals, b);
			s->MinLOD = ConsumeFloatLiteral(b);
		}
		else if (fieldId == "MaxLOD")
		{
			ConsumeToken(Token::Equals, b);
			s->MaxLOD = ConsumeFloatLiteral(b);
		}
		else
		{
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

		ConsumeToken(Token::Semicolon, b);

		if (TryConsumeToken(Token::RBrace, b))
			break;
	}

	return s;
}

Dispatch* ConsumeDispatchDef(
	BufferIter& b,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(Token::LBrace, b);

	Dispatch* dc = new Dispatch();
	rd->Dispatches.push_back(dc);

	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		if (fieldId == "Shader")
		{
			ConsumeToken(Token::Equals, b);
			ComputeShader* cs = ConsumeComputeShaderRefOrDef(b, ps);
			dc->Shader = cs;
		}
		else if (fieldId == "ThreadPerPixel")
		{
			ConsumeToken(Token::Equals, b);
			dc->ThreadPerPixel = ConsumeBool(b);
		}
		else if (fieldId == "Groups")
		{
			ConsumeToken(Token::Equals, b);
			ConsumeToken(Token::LBrace, b);
			dc->Groups.x = ConsumeUintLiteral(b);
			ConsumeToken(Token::Comma, b);
			dc->Groups.y = ConsumeUintLiteral(b);
			ConsumeToken(Token::Comma, b);
			dc->Groups.z = ConsumeUintLiteral(b);
			ConsumeToken(Token::RBrace, b);
		}
		else if (fieldId == "Bind")
		{
			BufferString bindName = ConsumeIdentifier(b);
			ConsumeToken(Token::Equals, b);
			Bind bind;
			bind.BindTarget = AddStringToDescriptionData(bindName, rd);
			if (TryConsumeToken(Token::At, b))
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
				bind.Type = res.type;
				bind.BufferBind = reinterpret_cast<Buffer*>(res.m);
			}
			dc->Binds.push_back(bind);
		}
		else
		{
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

		ConsumeToken(Token::Semicolon, b);

		if (TryConsumeToken(Token::RBrace, b))
			break;
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
			res.type = BindType::Buffer;
			res.m = buf;
		}
		else if (structureId == "Texture")
		{
			Texture* tex = ConsumeTextureDef(b,ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.resMap.count(nameId) == 0, "Resource %.*s already defined",
				nameId.len, nameId.base);
			ParseState::Resource& res = ps.resMap[nameId];
			res.type = BindType::Texture;
			res.m = tex;
		}
		else if (structureId == "Sampler")
		{
			Sampler* s = ConsumeSamplerDef(b,ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.resMap.count(nameId) == 0, "Resource %.*s already defined",
				nameId.len, nameId.base);
			ParseState::Resource& res = ps.resMap[nameId];
			res.type = BindType::Sampler;
			res.m = s;
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
			ConsumeToken(Token::LBrace, b);

			while (true)
			{
				Dispatch* dc = ConsumeDispatchRefOrDef(b, ps);
				rd->Passes.push_back(dc);

				if (!TryConsumeToken(Token::Comma, b))
					break;
			}

			ConsumeToken(Token::RBrace, b);
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
	for (Sampler* s : data->Samplers)
		delete s;
	delete data;
}

#undef ParserAssert

} // namespace rlf
