
namespace rlf
{

/******************************** Parser notes /********************************
	Parse code happens entirely in a separate thread, so that we can abort at 
	any time due to parse errors. That means the stack isn't cleaned up in 
	case of an error, so make sure dynamic allocations are never contained
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

#define RLF_KEYWORD_TUPLE \
	RLF_KEYWORD_ENTRY(ComputeShader) \
	RLF_KEYWORD_ENTRY(VertexShader) \
	RLF_KEYWORD_ENTRY(PixelShader) \
	RLF_KEYWORD_ENTRY(Buffer) \
	RLF_KEYWORD_ENTRY(Texture) \
	RLF_KEYWORD_ENTRY(Sampler) \
	RLF_KEYWORD_ENTRY(Dispatch) \
	RLF_KEYWORD_ENTRY(Draw) \
	RLF_KEYWORD_ENTRY(DrawIndexed) \
	RLF_KEYWORD_ENTRY(Passes) \
	RLF_KEYWORD_ENTRY(BackBuffer) \
	RLF_KEYWORD_ENTRY(Point) \
	RLF_KEYWORD_ENTRY(Linear) \
	RLF_KEYWORD_ENTRY(Aniso) \
	RLF_KEYWORD_ENTRY(All) \
	RLF_KEYWORD_ENTRY(Min) \
	RLF_KEYWORD_ENTRY(Mag) \
	RLF_KEYWORD_ENTRY(Mip) \
	RLF_KEYWORD_ENTRY(Wrap) \
	RLF_KEYWORD_ENTRY(Mirror) \
	RLF_KEYWORD_ENTRY(MirrorOnce) \
	RLF_KEYWORD_ENTRY(Clamp) \
	RLF_KEYWORD_ENTRY(Border) \
	RLF_KEYWORD_ENTRY(ShaderPath) \
	RLF_KEYWORD_ENTRY(EntryPoint) \
	RLF_KEYWORD_ENTRY(ElementSize) \
	RLF_KEYWORD_ENTRY(ElementCount) \
	RLF_KEYWORD_ENTRY(Flags) \
	RLF_KEYWORD_ENTRY(InitToZero) \
	RLF_KEYWORD_ENTRY(InitData) \
	RLF_KEYWORD_ENTRY(Size) \
	RLF_KEYWORD_ENTRY(Format) \
	RLF_KEYWORD_ENTRY(Filter) \
	RLF_KEYWORD_ENTRY(AddressMode) \
	RLF_KEYWORD_ENTRY(MipLODBias) \
	RLF_KEYWORD_ENTRY(MaxAnisotropy) \
	RLF_KEYWORD_ENTRY(BorderColor) \
	RLF_KEYWORD_ENTRY(MinLOD) \
	RLF_KEYWORD_ENTRY(MaxLOD) \
	RLF_KEYWORD_ENTRY(Shader) \
	RLF_KEYWORD_ENTRY(ThreadPerPixel) \
	RLF_KEYWORD_ENTRY(Groups) \
	RLF_KEYWORD_ENTRY(Bind) \
	RLF_KEYWORD_ENTRY(Topology) \
	RLF_KEYWORD_ENTRY(VShader) \
	RLF_KEYWORD_ENTRY(PShader) \
	RLF_KEYWORD_ENTRY(VertexBuffer) \
	RLF_KEYWORD_ENTRY(IndexBuffer) \
	RLF_KEYWORD_ENTRY(VertexCount) \
	RLF_KEYWORD_ENTRY(RenderTarget) \
	RLF_KEYWORD_ENTRY(BindVS) \
	RLF_KEYWORD_ENTRY(BindPS) \
	RLF_KEYWORD_ENTRY(True) \
	RLF_KEYWORD_ENTRY(False) \
	RLF_KEYWORD_ENTRY(Vertex) \
	RLF_KEYWORD_ENTRY(Index) \
	RLF_KEYWORD_ENTRY(Float) \
	RLF_KEYWORD_ENTRY(U16) \
	RLF_KEYWORD_ENTRY(PointList) \
	RLF_KEYWORD_ENTRY(LineList) \
	RLF_KEYWORD_ENTRY(LineStrip) \
	RLF_KEYWORD_ENTRY(TriList) \
	RLF_KEYWORD_ENTRY(TriStrip) \

#define RLF_KEYWORD_ENTRY(name) name,
enum class Keyword
{
	Invalid,
	RLF_KEYWORD_TUPLE
	_Count
};
#undef RLF_KEYWORD_ENTRY

#define RLF_KEYWORD_ENTRY(name) #name,
const char* KeywordString[] = 
{
	"<Invalid>",
	RLF_KEYWORD_TUPLE
};
#undef RLF_KEYWORD_ENTRY

#undef RLF_KEYWORD_TUPLE

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
		unsigned long h = 5381;
		unsigned const char* us = (unsigned const char *) s.base;
		unsigned const char* end = (unsigned const char *) s.base + s.len;
		while(us < end) {
			h = ((h << 5) + h) + *us;
			us++;
		}
		return h; 
	}
};

u32 LowerHash(const char* str, size_t len)
{
	unsigned long h = 5381;
	unsigned const char* us = (unsigned const char *) str;
	unsigned const char* end = (unsigned const char *) str + len;
	while(us < end) {
		h = ((h << 5) + h) + tolower(*us);
		us++;
	}
	return h; 
}

struct BufferIter
{
	char* next;
	char* end;
};

struct ParseState 
{
	BufferIter b;
	RenderDescription* rd;
	std::unordered_map<BufferString, Pass, BufferStringHash> passMap;
	std::unordered_map<BufferString, ComputeShader*, BufferStringHash> csMap;
	std::unordered_map<BufferString, VertexShader*, BufferStringHash> vsMap;
	std::unordered_map<BufferString, PixelShader*, BufferStringHash> psMap;
	struct Resource {
		BindType type;
		void* m;
	};
	std::unordered_map<BufferString, Resource, BufferStringHash> resMap;
	std::unordered_map<BufferString, TextureFormat, BufferStringHash> fmtMap;
	std::unordered_map<u32, Keyword> keyMap;
	ParseErrorState* es;

	const char* filename;
	char* bufferStart;
	Token fcLUT[128];
} *GPS;

struct ParseException {};

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
	ps->es->ErrorMessage += "\n";

	// This if statement is meaningless (rd will always be non-null here), but
	//	the compiler doesn't know that. Otherwise it will either generate 
	// 	C4702 (unreachable code) for any code that comes after a ParserError in 
	// 	optimized builds, or will generate C4715 (not all paths return value)
	// 	in non-optimized builds if you remove the unreachable code. 
	if (GPS->rd) 
		throw ParseException();
}

#define ParserAssert(expression, message, ...) 	\
do {											\
	if (!(expression)) {						\
		ParserError(message, ##__VA_ARGS__);	\
	}											\
} while (0);									\

Keyword LookupKeyword(
	BufferString id)
{
	u32 hash = LowerHash(id.base, id.len);
	if (GPS->keyMap.count(hash) > 0)
		return GPS->keyMap[hash];
	else
		return Keyword::Invalid;
}

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
	for (u32 i = 0 ; i < (u32)Keyword::_Count ; ++i)
	{
		Keyword key = (Keyword)i;
		const char* str = KeywordString[i];
		u32 hash = LowerHash(str, strlen(str));
		Assert(ps->keyMap.count(hash) == 0, "hash collision");
		ps->keyMap[hash] = key;
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
	Keyword key = LookupKeyword(value);
	if (key == Keyword::True)
		return true;
	else if (key == Keyword::False)
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

void AddMemToDescriptionData(void* mem, RenderDescription* rd)
{
	rd->Mems.push_back(mem);
}

SystemValue ConsumeSystemValue(BufferIter& b)
{
	BufferString sysId = ConsumeIdentifier(b);
	if (LookupKeyword(sysId) == Keyword::BackBuffer)
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
	Keyword key = LookupKeyword(filterId);
	switch (key)
	{
	case Keyword::Point:
		f = Filter::Point;
		break;
	case Keyword::Linear:
		f = Filter::Linear;
		break;
	case Keyword::Aniso:
		f = Filter::Aniso;
		break;
	default:
		ParserError("unexpected filter %.*s", filterId.len, filterId.base);
	}
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
		switch (LookupKeyword(fieldId))
		{
		case Keyword::All:
			fm.Min = fm.Mag = fm.Mip = ConsumeFilter(b);
			break;
		case Keyword::Min:
			fm.Min = ConsumeFilter(b);
			break;
		case Keyword::Mag:
			fm.Mag = ConsumeFilter(b);
			break;
		case Keyword::Mip:
			fm.Mip = ConsumeFilter(b);
			break;
		default:
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

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
	switch (LookupKeyword(adrId))
	{
	case Keyword::Wrap:
		addr = AddressMode::Wrap;
		break;
	case Keyword::Mirror:
		addr = AddressMode::Mirror;
		break;
	case Keyword::MirrorOnce:
		addr = AddressMode::MirrorOnce;
		break;
	case Keyword::Clamp:
		addr = AddressMode::Clamp;
		break;
	case Keyword::Border:
		addr = AddressMode::Border;
		break;
	default:
		ParserError("unexpected address mode %.*s", adrId.len, adrId.base);
	}
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

BufferFlags ConsumeBufferFlags(BufferIter& b)
{
	ConsumeToken(Token::LBrace, b);

	BufferFlags flags = (BufferFlags)0;
	while (true)
	{
		BufferString id = ConsumeIdentifier(b);
		switch (LookupKeyword(id))
		{
		case Keyword::Vertex:
			flags = (BufferFlags)(flags | BufferFlags_Vertex);
			break;
		case Keyword::Index:
			flags = (BufferFlags)(flags | BufferFlags_Index);
			break;
		default:
			ParserError("unexpected buffer flag %.*s", id.len, id.base);
		}

		if (TryConsumeToken(Token::RBrace, b))
			break;
		else 
			ConsumeToken(Token::Comma, b);
	}

	return flags;
}

Topology ConsumeTopology(BufferIter& b)
{
	Topology topo = Topology::TriList;
	BufferString id = ConsumeIdentifier(b);
	switch (LookupKeyword(id))
	{
	case Keyword::PointList:
		topo = Topology::PointList;
		break;
	case Keyword::LineList:
		topo = Topology::LineList;
		break;
	case Keyword::LineStrip:
		topo = Topology::LineStrip;
		break;
	case Keyword::TriList:
		topo = Topology::TriList;
		break;
	case Keyword::TriStrip:
		topo = Topology::TriStrip;
		break;
	default:
		ParserError("unexpected topology %.*s", id.len, id.base);
	}
	return topo;
}

Bind ConsumeBind(BufferIter& b, ParseState& ps)
{
	BufferString bindName = ConsumeIdentifier(b);
	ConsumeToken(Token::Equals, b);
	Bind bind;
	bind.BindTarget = AddStringToDescriptionData(bindName, ps.rd);
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
	return bind;
}

ComputeShader* ConsumeComputeShaderDef(
	BufferIter& b,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(Token::LBrace, b);

	ComputeShader* cs = new ComputeShader();
	rd->CShaders.push_back(cs);

	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		Keyword key = LookupKeyword(fieldId);
		if (key == Keyword::ShaderPath)
		{
			ConsumeToken(Token::Equals, b);
			BufferString value = ConsumeString(b);
			cs->ShaderPath = AddStringToDescriptionData(value, rd);
		}
		else if (key == Keyword::EntryPoint)
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
	Keyword key = LookupKeyword(id);
	if (key == Keyword::ComputeShader)
	{
		return ConsumeComputeShaderDef(b, ps);
	}
	else
	{
		ParserAssert(ps.csMap.count(id) != 0, "couldn't find shader %.*s", 
			id.len, id.base);
		return ps.csMap[id];
	}
}

VertexShader* ConsumeVertexShaderDef(
	BufferIter& b,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(Token::LBrace, b);

	VertexShader* vs = new VertexShader();
	rd->VShaders.push_back(vs);

	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		Keyword key = LookupKeyword(fieldId);
		if (key == Keyword::ShaderPath)
		{
			ConsumeToken(Token::Equals, b);
			BufferString value = ConsumeString(b);
			vs->ShaderPath = AddStringToDescriptionData(value, rd);
		}
		else if (key == Keyword::EntryPoint)
		{
			ConsumeToken(Token::Equals, b);
			BufferString value = ConsumeString(b);
			vs->EntryPoint = AddStringToDescriptionData(value, rd);
		}
		else
		{
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

		ConsumeToken(Token::Semicolon, b);

		if (TryConsumeToken(Token::RBrace, b))
			break;
	}

	return vs;
}

VertexShader* ConsumeVertexShaderRefOrDef(
	BufferIter& b,
	ParseState& ps)
{
	BufferString id = ConsumeIdentifier(b);
	Keyword key = LookupKeyword(id);
	if (key == Keyword::VertexShader)
	{
		return ConsumeVertexShaderDef(b, ps);
	}
	else
	{
		ParserAssert(ps.vsMap.count(id) != 0, "couldn't find shader %.*s", 
			id.len, id.base);
		return ps.vsMap[id];
	}
}

PixelShader* ConsumePixelShaderDef(
	BufferIter& b,
	ParseState& state)
{
	RenderDescription* rd = state.rd;

	ConsumeToken(Token::LBrace, b);

	PixelShader* ps = new PixelShader();
	rd->PShaders.push_back(ps);

	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		Keyword key = LookupKeyword(fieldId);
		if (key == Keyword::ShaderPath)
		{
			ConsumeToken(Token::Equals, b);
			BufferString value = ConsumeString(b);
			ps->ShaderPath = AddStringToDescriptionData(value, rd);
		}
		else if (key == Keyword::EntryPoint)
		{
			ConsumeToken(Token::Equals, b);
			BufferString value = ConsumeString(b);
			ps->EntryPoint = AddStringToDescriptionData(value, rd);
		}
		else
		{
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

		ConsumeToken(Token::Semicolon, b);

		if (TryConsumeToken(Token::RBrace, b))
			break;
	}

	return ps;
}

PixelShader* ConsumePixelShaderRefOrDef(
	BufferIter& b,
	ParseState& ps)
{
	BufferString id = ConsumeIdentifier(b);
	Keyword key = LookupKeyword(id);
	if (key == Keyword::PixelShader)
	{
		return ConsumePixelShaderDef(b, ps);
	}
	else
	{
		ParserAssert(ps.psMap.count(id) != 0, "couldn't find shader %.*s", 
			id.len, id.base);
		return ps.psMap[id];
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
		switch (LookupKeyword(fieldId))
		{
		case Keyword::ElementSize:
		{
			ConsumeToken(Token::Equals, b);
			buf->ElementSize = ConsumeUintLiteral(b);
			break;
		}
		case Keyword::ElementCount:
		{
			ConsumeToken(Token::Equals, b);
			buf->ElementCount = ConsumeUintLiteral(b);
			break;
		}
		case Keyword::Flags:
		{
			ConsumeToken(Token::Equals, b);
			buf->Flags = ConsumeBufferFlags(b);
			break;
		}
		case Keyword::InitToZero:
		{
			ConsumeToken(Token::Equals, b);
			buf->InitToZero = ConsumeBool(b);
			break;
		}
		case Keyword::InitData:
		{
			ConsumeToken(Token::Equals, b);
			BufferString id = ConsumeIdentifier(b);
			Keyword key = LookupKeyword(id);
			if (key == Keyword::Float)
			{
				ConsumeToken(Token::LBrace, b);

				u32 bufSize = buf->ElementSize * buf->ElementCount;
				ParserAssert(bufSize > 0, "Buffer size must be given before init");
				float* data = (float*)malloc(bufSize);
				AddMemToDescriptionData(data, rd);

				float* next = data;
				while (true)
				{
					float f = ConsumeFloatLiteral(b);
					ParserAssert((char*)next - (char*)data + sizeof(float) <= bufSize,
						"Too many elements for buffer size.");

					*next = f;
					++next;

					if (TryConsumeToken(Token::RBrace, b))
						break;
					else 
						ConsumeToken(Token::Comma, b);
				}
				buf->InitData = data;
			}
			else if (key == Keyword::U16)
			{
				ConsumeToken(Token::LBrace, b);

				u32 bufSize = buf->ElementSize * buf->ElementCount;
				ParserAssert(bufSize > 0, "Buffer size must be given before init");
				u16* data = (u16*)malloc(bufSize);
				AddMemToDescriptionData(data, rd);

				u16* next = data;
				while (true)
				{
					u16 l = (u16)ConsumeUintLiteral(b);
					ParserAssert((char*)next - (char*)data + sizeof(u16) <= bufSize,
						"Too many elements for buffer size.");

					*next = l;
					++next;

					if (TryConsumeToken(Token::RBrace, b))
						break;
					else 
						ConsumeToken(Token::Comma, b);
				}
				buf->InitData = data;
			}
			else
			{
				ParserError("unexpected data type %.*s", id.len, id.base);
			}
			break;
		}
		default:
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
		switch (LookupKeyword(fieldId))
		{
		case Keyword::Size:
		{
			ConsumeToken(Token::Equals, b);
			ConsumeToken(Token::LBrace, b);
			tex->Size.x = ConsumeUintLiteral(b);
			ConsumeToken(Token::Comma, b);
			tex->Size.y = ConsumeUintLiteral(b);
			ConsumeToken(Token::RBrace, b);
			break;
		}
		case Keyword::Format:
		{
			ConsumeToken(Token::Equals, b);
			BufferString formatId = ConsumeIdentifier(b);
			ParserAssert(ps.fmtMap.count(formatId) != 0, "Couldn't find format %.*s", 
				formatId.len, formatId.base);
			tex->Format = ps.fmtMap[formatId];
			break;
		}
		default:
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
		switch (LookupKeyword(fieldId))
		{
		case Keyword::Filter:
		{
			ConsumeToken(Token::Equals, b);
			s->Filter = ConsumeFilterMode(b);
			break;
		}
		case Keyword::AddressMode:
		{
			ConsumeToken(Token::Equals, b);
			s->Address = ConsumeAddressModeUVW(b);
			break;
		}
		case Keyword::MipLODBias:
		{
			ConsumeToken(Token::Equals, b);
			s->MipLODBias = ConsumeFloatLiteral(b);
			break;
		}
		case Keyword::MaxAnisotropy:
		{
			ConsumeToken(Token::Equals, b);
			s->MaxAnisotropy = ConsumeUintLiteral(b);
			break;
		}
		case Keyword::BorderColor:
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
			break;
		}
		case Keyword::MinLOD:
		{
			ConsumeToken(Token::Equals, b);
			s->MinLOD = ConsumeFloatLiteral(b);
			break;
		}
		case Keyword::MaxLOD:
		{
			ConsumeToken(Token::Equals, b);
			s->MaxLOD = ConsumeFloatLiteral(b);
			break;
		}
		default:
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
		switch (LookupKeyword(fieldId))
		{
		case Keyword::Shader:
		{
			ConsumeToken(Token::Equals, b);
			ComputeShader* cs = ConsumeComputeShaderRefOrDef(b, ps);
			dc->Shader = cs;
			break;
		}
		case Keyword::ThreadPerPixel:
		{
			ConsumeToken(Token::Equals, b);
			dc->ThreadPerPixel = ConsumeBool(b);
			break;
		}
		case Keyword::Groups:
		{
			ConsumeToken(Token::Equals, b);
			ConsumeToken(Token::LBrace, b);
			dc->Groups.x = ConsumeUintLiteral(b);
			ConsumeToken(Token::Comma, b);
			dc->Groups.y = ConsumeUintLiteral(b);
			ConsumeToken(Token::Comma, b);
			dc->Groups.z = ConsumeUintLiteral(b);
			ConsumeToken(Token::RBrace, b);
			break;
		}
		case Keyword::Bind:
		{
			Bind bind = ConsumeBind(b, ps);
			dc->Binds.push_back(bind);
			break;
		}
		default:
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

		ConsumeToken(Token::Semicolon, b);

		if (TryConsumeToken(Token::RBrace, b))
			break;
	}

	return dc;
}

Draw* ConsumeDrawDef(
	BufferIter& b,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(Token::LBrace, b);

	Draw* draw = new Draw();
	rd->Draws.push_back(draw);

	// non-zero defaults
	draw->Topology = Topology::TriList;

	while (true)
	{
		BufferString fieldId = ConsumeIdentifier(b);
		switch (LookupKeyword(fieldId))
		{
		case Keyword::Topology:
		{
			ConsumeToken(Token::Equals, b);
			draw->Topology = ConsumeTopology(b);
			break;
		}
		case Keyword::VShader:
		{
			ConsumeToken(Token::Equals, b);
			draw->VShader = ConsumeVertexShaderRefOrDef(b, ps);
			break;
		}
		case Keyword::PShader:
		{
			ConsumeToken(Token::Equals, b);
			draw->PShader = ConsumePixelShaderRefOrDef(b, ps);
			break;
		}
		case Keyword::VertexBuffer:
		{
			ConsumeToken(Token::Equals, b);
			BufferString id = ConsumeIdentifier(b);
			ParserAssert(ps.resMap.count(id) != 0, "Couldn't find resource %.*s", 
				id.len, id.base);
			ParseState::Resource& res = ps.resMap[id];
			ParserAssert(res.type == BindType::Buffer, "Resourse must be a buffer");
			draw->VertexBuffer = reinterpret_cast<Buffer*>(res.m);
			break;
		}
		case Keyword::IndexBuffer:
		{
			ConsumeToken(Token::Equals, b);
			BufferString id = ConsumeIdentifier(b);
			ParserAssert(ps.resMap.count(id) != 0, "Couldn't find resource %.*s", 
				id.len, id.base);
			ParseState::Resource& res = ps.resMap[id];
			ParserAssert(res.type == BindType::Buffer, "Resourse must be a buffer");
			draw->IndexBuffer = reinterpret_cast<Buffer*>(res.m);
			break;
		}
		case Keyword::VertexCount:
		{
			ConsumeToken(Token::Equals, b);
			draw->VertexCount = ConsumeUintLiteral(b);
			break;
		}
		case Keyword::RenderTarget:
		{
			ConsumeToken(Token::Equals, b);
			if (TryConsumeToken(Token::At, b))
			{
				draw->RenderTarget = ConsumeSystemValue(b);
			}
			else
			{
				Unimplemented();
			}
			break;
		}
		case Keyword::BindVS:
		{
			Bind bind = ConsumeBind(b, ps);
			draw->VSBinds.push_back(bind);
			break;
		}
		case Keyword::BindPS:
		{
			Bind bind = ConsumeBind(b, ps);
			draw->PSBinds.push_back(bind);
			break;
		}
		default:
			ParserError("unexpected field %.*s", fieldId.len, fieldId.base);
		}

		ConsumeToken(Token::Semicolon, b);

		if (TryConsumeToken(Token::RBrace, b))
			break;
	}

	return draw;
}


Pass ConsumePassRefOrDef(
	BufferIter& b,
	ParseState& ps)
{
	BufferString id = ConsumeIdentifier(b);
	Keyword key = LookupKeyword(id);
	Pass pass;
	if (key == Keyword::Dispatch)
	{
		pass.Type = PassType::Dispatch;
		pass.Dispatch = ConsumeDispatchDef(b, ps);
	}
	else if (key == Keyword::Draw)
	{
		pass.Type = PassType::Draw;
		pass.Draw = ConsumeDrawDef(b, ps);
	}
	else
	{
		ParserAssert(ps.passMap.count(id) != 0, "couldn't find pass %.*s", 
			id.len, id.base);
		pass = ps.passMap[id];
	}
	return pass;
}

void ParseMain()
{
	ParseState& ps = *GPS;
	RenderDescription* rd = ps.rd;
	BufferIter& b = ps.b;

	while (b.next != b.end)
	{
		BufferString id = ConsumeIdentifier(b);
		Keyword key = LookupKeyword(id);
		switch (key)
		{
		case Keyword::ComputeShader:
		{
			ComputeShader* cs = ConsumeComputeShaderDef(b, ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.csMap.count(nameId) == 0, "Shader %.*s already defined", 
				nameId.len, nameId.base);
			ps.csMap[nameId] = cs;
			break;
		}
		case Keyword::VertexShader:
		{
			VertexShader* vs = ConsumeVertexShaderDef(b, ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.vsMap.count(nameId) == 0, "Shader %.*s already defined", 
				nameId.len, nameId.base);
			ps.vsMap[nameId] = vs;
			break;
		}
		case Keyword::PixelShader:
		{
			PixelShader* pis = ConsumePixelShaderDef(b, ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.psMap.count(nameId) == 0, "Shader %.*s already defined", 
				nameId.len, nameId.base);
			ps.psMap[nameId] = pis;
			break;
		}
		case Keyword::Buffer:
		{
			Buffer* buf = ConsumeBufferDef(b, ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.resMap.count(nameId) == 0, "Resource %.*s already defined", 
				nameId.len, nameId.base);
			ParseState::Resource& res = ps.resMap[nameId];
			res.type = BindType::Buffer;
			res.m = buf;
			break;
		}
		case Keyword::Texture:
		{
			Texture* tex = ConsumeTextureDef(b,ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.resMap.count(nameId) == 0, "Resource %.*s already defined",
				nameId.len, nameId.base);
			ParseState::Resource& res = ps.resMap[nameId];
			res.type = BindType::Texture;
			res.m = tex;
			break;
		}
		case Keyword::Sampler:
		{
			Sampler* s = ConsumeSamplerDef(b,ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.resMap.count(nameId) == 0, "Resource %.*s already defined",
				nameId.len, nameId.base);
			ParseState::Resource& res = ps.resMap[nameId];
			res.type = BindType::Sampler;
			res.m = s;
			break;
		}
		case Keyword::Dispatch:
		{
			Dispatch* dc = ConsumeDispatchDef(b, ps);
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.passMap.count(nameId) == 0, "Pass %.*s already defined", 
				nameId.len, nameId.base);
			Pass pass;
			pass.Type = PassType::Dispatch;
			pass.Dispatch = dc;
			ps.passMap[nameId] = pass;
			break;
		}
		case Keyword::Draw:
		case Keyword::DrawIndexed:
		{
			Draw* draw = ConsumeDrawDef(b, ps);
			draw->Type = (key == Keyword::Draw) ? DrawType::Draw : DrawType::DrawIndexed;
			BufferString nameId = ConsumeIdentifier(b);
			ParserAssert(ps.passMap.count(nameId) == 0, "Pass %.*s already defined", 
				nameId.len, nameId.base);
			Pass pass;
			pass.Type = PassType::Draw;
			pass.Draw = draw;
			ps.passMap[nameId] = pass;
			break;
		}
		case Keyword::Passes:
		{
			ConsumeToken(Token::LBrace, b);

			while (true)
			{
				Pass pass = ConsumePassRefOrDef(b, ps);
				rd->Passes.push_back(pass);

				if (TryConsumeToken(Token::RBrace, b))
					break;
				else 
					ConsumeToken(Token::Comma, b);
			}
			break;
		}
		default:
			ParserError("Unexpected structure: %.*s", id.len, id.base);
		}

		// Skip trailing whitespace so we don't miss the exit condition for this loop. 
		SkipWhitespace(b);
	}
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

	try {
		ParseMain();
	}
	catch (ParseException pe)
	{
		(void)pe;
		Assert(ps.es->ParseSuccess == false, "Error not set");
	}

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
	for (Draw* draw : data->Draws)
		delete draw;
	for (ComputeShader* cs : data->CShaders)
		delete cs;
	for (VertexShader* vs : data->VShaders)
		delete vs;
	for (PixelShader* ps : data->PShaders)
		delete ps;
	for (Buffer* buf : data->Buffers)
		delete buf;
	for (Texture* tex : data->Textures)
		delete tex;
	for (Sampler* s : data->Samplers)
		delete s;
	for (void* mem : data->Mems)
		free(mem);
	delete data;
}

#undef ParserAssert

} // namespace rlf
