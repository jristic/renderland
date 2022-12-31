
#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"
#undef TINYOBJLOADER_IMPLEMENTATION

namespace rlf
{

/******************************** Parser notes /********************************
	Parse code can be terminated at any time due to errors. This is done via 
	exceptions, so the stack is cleaned up, but any dynamic allocations 
	expecting to be released later in the code may be leaked. For this reason, 
	any allocations which are used for the RLF representation should be 
	immediately recorded in the RenderDescription, which will be freed in 
	ReleaseData. Immediately in this context means before any further parsing 
	occurs. Any allocations only used for parsing can be recorded in the 
	ParseState to be cleaned up after parsing completion, regardless of failure. 
*******************************************************************************/

#define RLF_TOKEN_TUPLE \
	RLF_TOKEN_ENTRY(Invalid) \
	RLF_TOKEN_ENTRY(LParen) \
	RLF_TOKEN_ENTRY(RParen) \
	RLF_TOKEN_ENTRY(LBrace) \
	RLF_TOKEN_ENTRY(RBrace) \
	RLF_TOKEN_ENTRY(LBracket) \
	RLF_TOKEN_ENTRY(RBracket) \
	RLF_TOKEN_ENTRY(Comma) \
	RLF_TOKEN_ENTRY(Equals) \
	RLF_TOKEN_ENTRY(Plus) \
	RLF_TOKEN_ENTRY(Minus) \
	RLF_TOKEN_ENTRY(Semicolon) \
	RLF_TOKEN_ENTRY(At) \
	RLF_TOKEN_ENTRY(Period) \
	RLF_TOKEN_ENTRY(ForwardSlash) \
	RLF_TOKEN_ENTRY(Asterisk) \
	RLF_TOKEN_ENTRY(IntegerLiteral) \
	RLF_TOKEN_ENTRY(FloatLiteral) \
	RLF_TOKEN_ENTRY(Identifier) \
	RLF_TOKEN_ENTRY(String) \

#define RLF_TOKEN_ENTRY(name) name,
enum class TokenType
{
	RLF_TOKEN_TUPLE
};
#undef RLF_TOKEN_ENTRY

struct Token
{
	TokenType Type;
	const char* Location;
	union {
		const char* String;
		double FloatLiteral;
		u32 IntegerLiteral;
	};
};

#define RLF_TOKEN_ENTRY(name) #name,
const char* TokenNames[] =
{
	RLF_TOKEN_TUPLE
};
#undef RLF_TOKEN_ENTRY

#undef RLF_TOKEN_TUPLE


void TokenizerError(const char* loc, const char* str, ...)
{
	char buf[512];
	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,512,str,ptr);
	va_end(ptr);

	ErrorInfo pe = {};
	pe.Location = loc;
	pe.Message = buf;

	throw pe;
}

#define TokenizerAssert(expression, loc, message, ...) 	\
do {													\
	if (!(expression)) {								\
		TokenizerError(loc, message, ##__VA_ARGS__);	\
	}													\
} while (0);											\

struct TokenizerState {
	std::vector<Token> tokens;
	std::unordered_set<std::string> tokenStrings;
	TokenType fcLUT[128];
};

void TokenizerStateInit(TokenizerState& ts)
{
	TokenType* fcLUT = ts.fcLUT;
	for (u32 fc = 0 ; fc < 128 ; ++fc)
		fcLUT[fc] = TokenType::Invalid;
	fcLUT['('] = TokenType::LParen;
	fcLUT[')'] = TokenType::RParen;
	fcLUT['{'] = TokenType::LBrace;
	fcLUT['}'] = TokenType::RBrace;
	fcLUT['['] = TokenType::LBracket;
	fcLUT[']'] = TokenType::RBracket;
	fcLUT[','] = TokenType::Comma;
	fcLUT['='] = TokenType::Equals;
	fcLUT['+'] = TokenType::Plus;
	fcLUT['-'] = TokenType::Minus;
	fcLUT[';'] = TokenType::Semicolon;
	fcLUT['@'] = TokenType::At;
	fcLUT['.'] = TokenType::Period;
	fcLUT['/'] = TokenType::ForwardSlash;
	fcLUT['*'] = TokenType::Asterisk;
	fcLUT['"'] = TokenType::String;
	for (u32 fc = 0 ; fc < 128 ; ++fc)
	{
		if (isalpha(fc))
			fcLUT[fc] = TokenType::Identifier;
		else if (isdigit(fc))
			fcLUT[fc] = TokenType::IntegerLiteral;
	} 
	fcLUT['_'] = TokenType::Identifier;
}

const char* SkipWhitespace(const char* next, const char* end)
{
	bool checkAgain;
	do {
		checkAgain = false;
		while (next < end && isspace(*next))
		{
			++next;
		}
		if (next + 1 < end && next[0] == '/')
		{
			if (next[1] == '/')
			{
				next += 2;
				while (next < end && *next != '\n')
					++next;
				checkAgain = true;
			}
			else if (next[1] == '*')
			{
				next += 2;
				while (next + 1 < end)
				{
					if (next[0] == '*' && next[1] == '/')
						break;
					++next;
				}
				TokenizerAssert(next[0] == '*' && next[1] == '/', next,
					"Closing of comment block was not found before end of file");
				next += 2;
				checkAgain = true;
			}
		}
	}
	while (checkAgain);
	return next;
}

void Tokenize(const char* start, const char* end, TokenizerState& ts)
{
	const char* next = SkipWhitespace(start, end);

	while (next < end)
	{
		char firstChar = *next;

		TokenType tok = ts.fcLUT[firstChar];

		Token token;
		token.Location = next;

		switch (tok)
		{
		case TokenType::LParen:
		case TokenType::RParen:
		case TokenType::LBrace:
		case TokenType::RBrace:
		case TokenType::LBracket:
		case TokenType::RBracket:
		case TokenType::Comma:
		case TokenType::Equals:
		case TokenType::Plus:
		case TokenType::Minus:
		case TokenType::Semicolon:
		case TokenType::At:
		case TokenType::Period:
		case TokenType::ForwardSlash:
		case TokenType::Asterisk:
			++next;
			break;
		case TokenType::IntegerLiteral:
		{
			u32 val = 0;
			while(next < end && isdigit(*next)) {
				val *= 10;
				val += (*next - '0');
				++next;
			}
			token.IntegerLiteral = val;
			if (next < end && *next == '.') {
				++next;
				tok = TokenType::FloatLiteral;
				double floatVal = val;
				double sub = 0.1;
				while(next < end && isdigit(*next)) {
					floatVal += (*next - '0') * sub;
					sub *= 0.1;
					++next;
				}
				token.FloatLiteral = floatVal;
			}
			break;
		}
		case TokenType::Identifier:
		{
			// identifiers have to start with a letter, but can contain numbers
			const char* id_begin = next;
			while (next < end && (isalpha(*next) || isdigit(*next) || 
				*next == '_')) 
			{
				++next;
			}
			const char* id_end = next;
			std::string id = std::string(id_begin, id_end);
			auto insrt = ts.tokenStrings.insert(id);
			token.String = insrt.first->c_str();
			break;
		}
		case TokenType::String:
		{
			++next; // skip over opening quotes
			const char* str_begin = next;
			while (next < end && *next != '"') {
				++next;
			}
			TokenizerAssert(next < end, next, "End-of-buffer before closing parenthesis.");
			const char* str_end = next;
			++next; // pass the closing quotes
			std::string str = std::string(str_begin, str_end);
			auto insrt = ts.tokenStrings.insert(str);
			token.String = insrt.first->c_str();
			break;
		}
		case TokenType::Invalid:
		default:
			TokenizerError(next, "unexpected character when parsing token: %c", firstChar);
			break;
		}

		token.Type = tok;
		ts.tokens.push_back(token);

		next = SkipWhitespace(next, end);
	}
	Assert(next == end, "Internal inconsistency, passed the end of the buffer");
}


#define RLF_KEYWORD_TUPLE \
	RLF_KEYWORD_ENTRY(ComputeShader) \
	RLF_KEYWORD_ENTRY(VertexShader) \
	RLF_KEYWORD_ENTRY(PixelShader) \
	RLF_KEYWORD_ENTRY(Buffer) \
	RLF_KEYWORD_ENTRY(Texture) \
	RLF_KEYWORD_ENTRY(Sampler) \
	RLF_KEYWORD_ENTRY(RasterizerState) \
	RLF_KEYWORD_ENTRY(DepthStencilState) \
	RLF_KEYWORD_ENTRY(Viewports) \
	RLF_KEYWORD_ENTRY(ObjImport) \
	RLF_KEYWORD_ENTRY(Dispatch) \
	RLF_KEYWORD_ENTRY(Draw) \
	RLF_KEYWORD_ENTRY(ClearColor) \
	RLF_KEYWORD_ENTRY(ClearDepth) \
	RLF_KEYWORD_ENTRY(ClearStencil) \
	RLF_KEYWORD_ENTRY(Passes) \
	RLF_KEYWORD_ENTRY(BackBuffer) \
	RLF_KEYWORD_ENTRY(DefaultDepth) \
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
	RLF_KEYWORD_ENTRY(DDSPath) \
	RLF_KEYWORD_ENTRY(SampleCount) \
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
	RLF_KEYWORD_ENTRY(IndirectArgs) \
	RLF_KEYWORD_ENTRY(IndirectArgsOffset) \
	RLF_KEYWORD_ENTRY(Bind) \
	RLF_KEYWORD_ENTRY(SetConstant) \
	RLF_KEYWORD_ENTRY(Topology) \
	RLF_KEYWORD_ENTRY(RState) \
	RLF_KEYWORD_ENTRY(DSState) \
	RLF_KEYWORD_ENTRY(VShader) \
	RLF_KEYWORD_ENTRY(PShader) \
	RLF_KEYWORD_ENTRY(VertexBuffer) \
	RLF_KEYWORD_ENTRY(IndexBuffer) \
	RLF_KEYWORD_ENTRY(VertexCount) \
	RLF_KEYWORD_ENTRY(InstanceCount) \
	RLF_KEYWORD_ENTRY(StencilRef) \
	RLF_KEYWORD_ENTRY(RenderTarget) \
	RLF_KEYWORD_ENTRY(RenderTargets) \
	RLF_KEYWORD_ENTRY(DepthStencil) \
	RLF_KEYWORD_ENTRY(BindVS) \
	RLF_KEYWORD_ENTRY(BindPS) \
	RLF_KEYWORD_ENTRY(SetConstantVS) \
	RLF_KEYWORD_ENTRY(SetConstantPS) \
	RLF_KEYWORD_ENTRY(True) \
	RLF_KEYWORD_ENTRY(False) \
	RLF_KEYWORD_ENTRY(Vertex) \
	RLF_KEYWORD_ENTRY(Index) \
	RLF_KEYWORD_ENTRY(Structured) \
	RLF_KEYWORD_ENTRY(Raw) \
	RLF_KEYWORD_ENTRY(U16) \
	RLF_KEYWORD_ENTRY(U32) \
	RLF_KEYWORD_ENTRY(Float) \
	RLF_KEYWORD_ENTRY(Float2) \
	RLF_KEYWORD_ENTRY(Float3) \
	RLF_KEYWORD_ENTRY(Float4) \
	RLF_KEYWORD_ENTRY(Float4x4) \
	RLF_KEYWORD_ENTRY(Bool) \
	RLF_KEYWORD_ENTRY(Int) \
	RLF_KEYWORD_ENTRY(Int2) \
	RLF_KEYWORD_ENTRY(Int3) \
	RLF_KEYWORD_ENTRY(Int4) \
	RLF_KEYWORD_ENTRY(Uint) \
	RLF_KEYWORD_ENTRY(Uint2) \
	RLF_KEYWORD_ENTRY(Uint3) \
	RLF_KEYWORD_ENTRY(Uint4) \
	RLF_KEYWORD_ENTRY(PointList) \
	RLF_KEYWORD_ENTRY(LineList) \
	RLF_KEYWORD_ENTRY(LineStrip) \
	RLF_KEYWORD_ENTRY(TriList) \
	RLF_KEYWORD_ENTRY(TriStrip) \
	RLF_KEYWORD_ENTRY(None) \
	RLF_KEYWORD_ENTRY(Front) \
	RLF_KEYWORD_ENTRY(Back) \
	RLF_KEYWORD_ENTRY(Fill) \
	RLF_KEYWORD_ENTRY(CullMode) \
	RLF_KEYWORD_ENTRY(FrontCCW) \
	RLF_KEYWORD_ENTRY(DepthBias) \
	RLF_KEYWORD_ENTRY(SlopeScaledDepthBias) \
	RLF_KEYWORD_ENTRY(DepthBiasClamp) \
	RLF_KEYWORD_ENTRY(DepthClipEnable) \
	RLF_KEYWORD_ENTRY(ScissorEnable) \
	RLF_KEYWORD_ENTRY(MultisampleEnable) \
	RLF_KEYWORD_ENTRY(AntialiasedLineEnable) \
	RLF_KEYWORD_ENTRY(ObjPath) \
	RLF_KEYWORD_ENTRY(Vertices) \
	RLF_KEYWORD_ENTRY(Indices) \
	RLF_KEYWORD_ENTRY(Target) \
	RLF_KEYWORD_ENTRY(Color) \
	RLF_KEYWORD_ENTRY(Depth) \
	RLF_KEYWORD_ENTRY(Stencil) \
	RLF_KEYWORD_ENTRY(SRV) \
	RLF_KEYWORD_ENTRY(UAV) \
	RLF_KEYWORD_ENTRY(RTV) \
	RLF_KEYWORD_ENTRY(DSV) \
	RLF_KEYWORD_ENTRY(Never) \
	RLF_KEYWORD_ENTRY(Less) \
	RLF_KEYWORD_ENTRY(Equal) \
	RLF_KEYWORD_ENTRY(LessEqual) \
	RLF_KEYWORD_ENTRY(Greater) \
	RLF_KEYWORD_ENTRY(NotEqual) \
	RLF_KEYWORD_ENTRY(GreaterEqual) \
	RLF_KEYWORD_ENTRY(Always) \
	RLF_KEYWORD_ENTRY(Keep) \
	RLF_KEYWORD_ENTRY(Zero) \
	RLF_KEYWORD_ENTRY(Replace) \
	RLF_KEYWORD_ENTRY(IncrSat) \
	RLF_KEYWORD_ENTRY(DecrSat) \
	RLF_KEYWORD_ENTRY(Invert) \
	RLF_KEYWORD_ENTRY(Incr) \
	RLF_KEYWORD_ENTRY(Decr) \
	RLF_KEYWORD_ENTRY(StencilFailOp) \
	RLF_KEYWORD_ENTRY(StencilDepthFailOp) \
	RLF_KEYWORD_ENTRY(StencilPassOp) \
	RLF_KEYWORD_ENTRY(StencilFunc) \
	RLF_KEYWORD_ENTRY(DepthEnable) \
	RLF_KEYWORD_ENTRY(DepthWrite) \
	RLF_KEYWORD_ENTRY(DepthFunc) \
	RLF_KEYWORD_ENTRY(StencilEnable) \
	RLF_KEYWORD_ENTRY(StencilReadMask) \
	RLF_KEYWORD_ENTRY(StencilWriteMask) \
	RLF_KEYWORD_ENTRY(FrontFace) \
	RLF_KEYWORD_ENTRY(BackFace) \
	RLF_KEYWORD_ENTRY(Constant) \
	RLF_KEYWORD_ENTRY(Tuneable) \
	RLF_KEYWORD_ENTRY(Resource) \
	RLF_KEYWORD_ENTRY(NumElements) \
	RLF_KEYWORD_ENTRY(Resolve) \
	RLF_KEYWORD_ENTRY(Src) \
	RLF_KEYWORD_ENTRY(Dst) \
	RLF_KEYWORD_ENTRY(Viewport) \
	RLF_KEYWORD_ENTRY(TopLeft) \
	RLF_KEYWORD_ENTRY(DepthRange) \

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

u32 LowerHash(const char* str)
{
	unsigned long h = 5381;
	unsigned const char* us = (unsigned const char *) str;
	while(*us != '\0') {
		h = ((h << 5) + h) + tolower(*us);
		us++;
	}
	return h; 
}

struct TokenIter
{
	const Token* next;
	const Token* end;
};

struct ParseState 
{
	TokenIter t;
	RenderDescription* rd;
	std::unordered_map<const char*, Pass> passMap;
	std::unordered_map<const char*, ComputeShader*> csMap;
	std::unordered_map<const char*, VertexShader*> vsMap;
	std::unordered_map<const char*, PixelShader*> psMap;
	enum class ResType {
		Sampler,
		Buffer,
		Texture,
		View,
	};
	struct Resource {
		ResType type;
		void* m;
	};
	std::unordered_map<const char*, Resource> resMap;
	std::unordered_map<u32, TextureFormat> fmtMap;
	std::unordered_map<const char*, RasterizerState*> rsMap;
	std::unordered_map<const char*, DepthStencilState*> dssMap;
	std::unordered_map<const char*, Viewport*> vpMap;
	std::unordered_map<const char*, ObjImport*> objMap;
	struct Var {
		bool tuneable;
		void* m;
	};
	std::unordered_map<const char*, Var> varMap;
	std::unordered_map<u32, Keyword> keyMap;

	const char* workingDirectory;
} *GPS;

void ParserError(const char* str, ...)
{
	char buf[512];
	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,512,str,ptr);
	va_end(ptr);

	ParseState* ps = GPS;
	ErrorInfo pe = {};
	pe.Location = ps->t.next < ps->t.end ? ps->t.next->Location : nullptr;
	pe.Message = buf;

	throw pe;
}

#define ParserAssert(expression, message, ...) 	\
do {											\
	if (!(expression)) {						\
		ParserError(message, ##__VA_ARGS__);	\
	}											\
} while (0);									\

Keyword LookupKeyword(
	const char* str)
{
	u32 hash = LowerHash(str);
	if (GPS->keyMap.count(hash) > 0)
		return GPS->keyMap[hash];
	else
		return Keyword::Invalid;
}

void ParseStateInit(ParseState* ps)
{
	for (u32 i = (u32)TextureFormat::Invalid+1 ; i < (u32)TextureFormat::_Count ; ++i)
	{
		TextureFormat fmt = (TextureFormat)i;
		const char* str = TextureFormatName[i];
		u32 hash = LowerHash(str);
		Assert(ps->fmtMap.count(hash) == 0, "hash collision");
		ps->fmtMap[hash] = fmt;
	}
	for (u32 i = 0 ; i < (u32)Keyword::_Count ; ++i)
	{
		Keyword key = (Keyword)i;
		const char* str = KeywordString[i];
		u32 hash = LowerHash(str);
		Assert(ps->keyMap.count(hash) == 0, "hash collision");
		ps->keyMap[hash] = key;
	}
}

TokenType PeekNextToken(
	const TokenIter& t)
{
	ParserAssert(t.next != t.end, "unexpected end-of-buffer")
	TokenType type = t.next->Type;
	return type;
}

void ConsumeToken(
	TokenType tok,
	TokenIter& t)
{
	TokenType foundTok = PeekNextToken(t);
	ParserAssert(foundTok == tok, "unexpected token, expected %s but found %s",
		TokenNames[(u32)tok], TokenNames[(u32)foundTok]);
	++t.next;
}

bool TryConsumeToken(
	TokenType tok,
	TokenIter& t)
{
	TokenType foundTok = PeekNextToken(t);
	bool found = (foundTok == tok);
	if (found)
		++t.next;
	return found;
}

const char* ConsumeIdentifier(
	TokenIter& t)
{
	TokenType tok = PeekNextToken(t);
	ParserAssert(tok == TokenType::Identifier, "unexpected %s (wanted identifier)", 
		TokenNames[(u32)tok]);

	const char* str = t.next->String;
	++t.next;
	return str;
}

const char* ConsumeString(
	TokenIter& t)
{
	TokenType tok = PeekNextToken(t);
	ParserAssert(tok == TokenType::String, "unexpected %s (wanted string)", 
		TokenNames[(u32)tok]);

	const char* str = t.next->String;
	++t.next;
	return str;	
}

bool ConsumeBool(
	TokenIter& t)
{
	const char* value = ConsumeIdentifier(t);
	Keyword key = LookupKeyword(value);
	bool ret = false;
	if (key == Keyword::True)
		ret = true;
	else if (key == Keyword::False)
		ret = false;
	else
		ParserError("Expected bool (true/false), got: %s", value);

	return ret;
}

i32 ConsumeIntLiteral(
	TokenIter& t)
{
	bool negative = TryConsumeToken(TokenType::Minus, t);
	TokenType tok = PeekNextToken(t);
	ParserAssert(tok == TokenType::IntegerLiteral, "unexpected %s (wanted integer literal)",
		TokenNames[(u32)tok]);
	i32 val = t.next->IntegerLiteral;
	++t.next;
	return negative ? -val : val;
}

u32 ConsumeUintLiteral(
	TokenIter& t)
{
	if (TryConsumeToken(TokenType::Minus, t))
		ParserError("Unsigned int expected, '-' invalid here");
	TokenType tok = PeekNextToken(t);
	ParserAssert(tok == TokenType::IntegerLiteral, "unexpected %s (wanted integer literal)",
		TokenNames[(u32)tok]);

	u32 val = t.next->IntegerLiteral;
	++t.next;
	return val;
}

u8 ConsumeUcharLiteral(
	TokenIter& t)
{
	u32 u = ConsumeUintLiteral(t);
	ParserAssert(u < 256, "Uchar value too large to fit: %u", u);
	return (u8)u;
}

float ConsumeFloatLiteral(
	TokenIter& t)
{
	bool negative = TryConsumeToken(TokenType::Minus, t);
	TokenType tok = PeekNextToken(t);
	ParserAssert(tok == TokenType::IntegerLiteral || tok == TokenType::FloatLiteral, 
		"unexpected %s (wanted float literal)", TokenNames[(u32)tok]);

	bool fracPart = (tok == TokenType::FloatLiteral);
	double val = fracPart ? t.next->FloatLiteral : t.next->IntegerLiteral;
	++t.next;

	double result = negative ? -val : val;
	ParserAssert(result >= -FLT_MAX && result < FLT_MAX, 
		"Given literal is outside of representable range.");
	return (float)(result);
}

const char* AddStringToDescriptionData(const char* str, RenderDescription* rd)
{
	auto pair = rd->Strings.insert(std::string(str));
	return pair.first->c_str();
}

void AddMemToDescriptionData(void* mem, RenderDescription* rd)
{
	rd->Mems.push_back(mem);
}

// -----------------------------------------------------------------------------
// ------------------------------ ENUMS ----------------------------------------
// -----------------------------------------------------------------------------
template <typename T>
struct EnumEntry
{
	Keyword Key;
	T Value;
};
template <typename T, size_t DefSize>
T ConsumeEnum(TokenIter& ti, EnumEntry<T> (&def)[DefSize], const char* name)
{
	const char* id = ConsumeIdentifier(ti);
	Keyword key = LookupKeyword(id);
	T t = T::Invalid;
	for (size_t i = 0 ; i < DefSize ; ++i)
	{
		if (key == def[i].Key)
		{
			t = def[i].Value;
			break;
		}
	}
	ParserAssert(t != T::Invalid, "Invalid %s enum value: %s", name, id);
	return t;
}

SystemValue ConsumeSystemValue(TokenIter& t)
{
	static EnumEntry<SystemValue> def[] = {
		Keyword::BackBuffer,	SystemValue::BackBuffer,
		Keyword::DefaultDepth,	SystemValue::DefaultDepth,
	};
	return ConsumeEnum(t, def, "SystemValue");
}

Filter ConsumeFilter(TokenIter& t)
{
	static EnumEntry<Filter> def[] = {
		Keyword::Point,		Filter::Point,
		Keyword::Linear,	Filter::Linear,
		Keyword::Aniso,		Filter::Aniso,
	};
	return ConsumeEnum(t, def, "Filter");
}

AddressMode ConsumeAddressMode(TokenIter& t)
{
	static EnumEntry<AddressMode> def[] = {
		Keyword::Wrap,			AddressMode::Wrap,
		Keyword::Mirror,		AddressMode::Mirror,
		Keyword::MirrorOnce,	AddressMode::MirrorOnce,
		Keyword::Clamp,			AddressMode::Clamp,
		Keyword::Border,		AddressMode::Border,
	};
	return ConsumeEnum(t, def, "AddressMode");
}

Topology ConsumeTopology(TokenIter& t)
{
	static EnumEntry<Topology> def[] = {
		Keyword::PointList,		Topology::PointList,
		Keyword::LineList,		Topology::LineList,
		Keyword::LineStrip,		Topology::LineStrip,
		Keyword::TriList,		Topology::TriList,
		Keyword::TriStrip,		Topology::TriStrip,
	};
	return ConsumeEnum(t, def, "Topology");
}

CullMode ConsumeCullMode(TokenIter& t)
{
	static EnumEntry<CullMode> def[] = {
		Keyword::None,	CullMode::None,
		Keyword::Front,	CullMode::Front,
		Keyword::Back,	CullMode::Back,
	};
	return ConsumeEnum(t, def, "CullMode");
}

ComparisonFunc ConsumeComparisonFunc(TokenIter& t)
{
	static EnumEntry<ComparisonFunc> def[] = {
		Keyword::Never,			ComparisonFunc::Never,
		Keyword::Less,			ComparisonFunc::Less,
		Keyword::Equal,			ComparisonFunc::Equal,
		Keyword::LessEqual,		ComparisonFunc::LessEqual,
		Keyword::Greater,		ComparisonFunc::Greater,
		Keyword::NotEqual,		ComparisonFunc::NotEqual,
		Keyword::GreaterEqual,	ComparisonFunc::GreaterEqual,
		Keyword::Always,		ComparisonFunc::Always,
	};
	return ConsumeEnum(t, def, "ComparisonFunc");
}

StencilOp ConsumeStencilOp(TokenIter& t)
{
	static EnumEntry<StencilOp> def[] = {
		Keyword::Keep,		StencilOp::Keep,
		Keyword::Zero,		StencilOp::Zero,
		Keyword::Replace,	StencilOp::Replace,
		Keyword::IncrSat,	StencilOp::IncrSat,
		Keyword::DecrSat,	StencilOp::DecrSat,
		Keyword::Invert,	StencilOp::Invert,
		Keyword::Incr,		StencilOp::Incr,
		Keyword::Decr,		StencilOp::Decr,
	};
	return ConsumeEnum(t, def, "StencilOp");
}

// -----------------------------------------------------------------------------
// ------------------------------ FLAGS ----------------------------------------
// -----------------------------------------------------------------------------
template <typename T>
struct FlagsEntry
{
	Keyword Key;
	T Value;
};
template <typename T, size_t DefSize>
T ConsumeFlags(TokenIter& t, FlagsEntry<T> (&def)[DefSize], const char* name)
{
	ConsumeToken(TokenType::LBrace, t);

	T flags = (T)0;
	while (true)
	{
		if (TryConsumeToken(TokenType::RBrace, t))
			break;

		const char* id = ConsumeIdentifier(t);
		Keyword key = LookupKeyword(id);
		bool found = false;
		for (size_t i = 0 ; i < DefSize ; ++i)
		{
			if (key == def[i].Key)
			{
				flags = (T)(flags | def[i].Value);
				found = true;
				break;
			}
		}
		ParserAssert(found, "Unexpected %s flag: %s", name, id)

		if (TryConsumeToken(TokenType::RBrace, t))
			break;
		else
			ConsumeToken(TokenType::Comma, t);
	}

	return flags;
}

BufferFlag ConsumeBufferFlag(TokenIter& t)
{
	static FlagsEntry<BufferFlag> def[] = {
		Keyword::Vertex,		BufferFlag_Vertex,
		Keyword::Index,			BufferFlag_Index,
		Keyword::Structured,	BufferFlag_Structured,
		Keyword::IndirectArgs,	BufferFlag_IndirectArgs,
		Keyword::Raw,			BufferFlag_Raw,
	};
	return ConsumeFlags(t, def, "BufferFlag");
}
TextureFlag ConsumeTextureFlag(TokenIter& t)
{
	static FlagsEntry<TextureFlag> def[] = {
		Keyword::SRV,	TextureFlag_SRV,
		Keyword::UAV,	TextureFlag_UAV,
		Keyword::RTV,	TextureFlag_RTV,
		Keyword::DSV,	TextureFlag_DSV,
	};
	return ConsumeFlags(t, def, "TextureFlag");
}

// -----------------------------------------------------------------------------
// ------------------------------ SPECIAL --------------------------------------
// -----------------------------------------------------------------------------
FilterMode ConsumeFilterMode(TokenIter& t)
{
	FilterMode fm = {};
	ConsumeToken(TokenType::LBrace, t);
	while (true)
	{
		if (TryConsumeToken(TokenType::RBrace, t))
			break;

		const char* fieldId = ConsumeIdentifier(t);
		ConsumeToken(TokenType::Equals, t);
		switch (LookupKeyword(fieldId))
		{
		case Keyword::All:
			fm.Min = fm.Mag = fm.Mip = ConsumeFilter(t);
			break;
		case Keyword::Min:
			fm.Min = ConsumeFilter(t);
			break;
		case Keyword::Mag:
			fm.Mag = ConsumeFilter(t);
			break;
		case Keyword::Mip:
			fm.Mip = ConsumeFilter(t);
			break;
		default:
			ParserError("unexpected field %s", fieldId);
		}

		if (TryConsumeToken(TokenType::RBrace, t))
			break;
		else 
			ConsumeToken(TokenType::Comma, t);
	}
	return fm;
}

AddressModeUVW ConsumeAddressModeUVW(TokenIter& t)
{
	AddressModeUVW addr; 
	addr.U = addr.V = addr.W = AddressMode::Wrap;
	ConsumeToken(TokenType::LBrace,t);
	while (true)
	{
		if (TryConsumeToken(TokenType::RBrace, t))
			break;

		const char* fieldId = ConsumeIdentifier(t);
		ConsumeToken(TokenType::Equals, t);
		AddressMode mode = ConsumeAddressMode(t);

		ParserAssert(strlen(fieldId) <= 3, "Invalid [U?V?W?], %s", fieldId);

		const char* curr = fieldId;
		while (*curr != '\0')
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

		if (TryConsumeToken(TokenType::RBrace, t))
			break;
		else 
			ConsumeToken(TokenType::Comma, t);
	}
	return addr;
}

View* ConsumeViewDef(TokenIter& t, ParseState& ps, ViewType vt)
{
	View* v = new View();
	ps.rd->Views.push_back(v);
	v->Type = vt;
	ConsumeToken(TokenType::LBrace, t);
	while (true)
	{
		if (TryConsumeToken(TokenType::RBrace, t))
			break;

		const char* id = ConsumeIdentifier(t);
		Keyword key = LookupKeyword(id);
		ConsumeToken(TokenType::Equals, t);
		if (key == Keyword::Resource)
		{
			const char* rid = ConsumeIdentifier(t);
			ParserAssert(ps.resMap.count(rid) != 0, "Couldn't find resource %s", 
				rid);
			ParseState::Resource& res = ps.resMap[rid];
			if (res.type == ParseState::ResType::Buffer)
			{
				v->ResourceType = ResourceType::Buffer;
				v->Buffer = (Buffer*)res.m;
			}
			else if (res.type == ParseState::ResType::Texture)
			{
				v->ResourceType = ResourceType::Texture;
				v->Texture = (Texture*)res.m;
				v->Texture->Views.insert(v);
			}
			else
			{
				ParserError("Referenced resource (%s) must be Texture or Buffer.", 
					rid);
			}
		}
		else if (key == Keyword::Format)
		{
			const char* formatId = ConsumeIdentifier(t);
			u32 hash = LowerHash(formatId);
			ParserAssert(GPS->fmtMap.count(hash) != 0, "Couldn't find format %s", 
				formatId);
			v->Format = GPS->fmtMap[hash];
		}
		else if (key == Keyword::NumElements)
		{
			v->NumElements = ConsumeUintLiteral(t);
		}
		else 
		{
			ParserError("Unexpected field (%s)", id);
		}

		ConsumeToken(TokenType::Semicolon, t);
	}
	ParserAssert(v->ResourceType != ResourceType::Invalid, 
		"Resource on View must be set.");
	return v;
}

View* ConsumeViewRefOrDef(TokenIter& t, ParseState& ps, const char* id)
{
	View* v = nullptr;

	Keyword key = LookupKeyword(id);
	if (key == Keyword::SRV)
	{
		v = ConsumeViewDef(t,ps, ViewType::SRV);
	}
	else if (key == Keyword::UAV)
	{
		v = ConsumeViewDef(t,ps, ViewType::UAV);
	}
	else if (key == Keyword::RTV)
	{
		v = ConsumeViewDef(t,ps, ViewType::RTV);
	}
	else if (key == Keyword::DSV)
	{
		v = ConsumeViewDef(t,ps, ViewType::DSV);
	}
	else
	{
		ParserAssert(ps.resMap.count(id) != 0, "Couldn't find resource %s", id);
		ParseState::Resource& res = ps.resMap[id];
		if (res.type == ParseState::ResType::View)
		{
			v = (View*)res.m;
		}
		else if (res.type == ParseState::ResType::Buffer)
		{
			v = new View();
			ps.rd->Views.push_back(v);
			v->Type = ViewType::Auto;
			v->ResourceType = ResourceType::Buffer;
			v->Buffer = (Buffer*)res.m;
			v->Format = TextureFormat::Invalid;
		}
		else if (res.type == ParseState::ResType::Texture)
		{
			v = new View();
			ps.rd->Views.push_back(v);
			v->Type = ViewType::Auto;
			v->ResourceType = ResourceType::Texture;
			v->Texture = (Texture*)res.m;
			v->Texture->Views.insert(v);
			v->Format = TextureFormat::Invalid;
		}
		else if (res.type == ParseState::ResType::Sampler)
		{
			// NOTE: Valid but not handled here.
		}
		else
		{
			Unimplemented();
		}
	}

	return v;
}

Bind ConsumeBind(TokenIter& t, ParseState& ps)
{
	const char* bindName = ConsumeIdentifier(t);
	ConsumeToken(TokenType::Equals, t);
	Bind bind;
	bind.BindTarget = AddStringToDescriptionData(bindName, ps.rd);
	if (TryConsumeToken(TokenType::At, t))
	{
		bind.Type = BindType::SystemValue;
		bind.SystemBind = ConsumeSystemValue(t);
	}
	else
	{
		const char* id = ConsumeIdentifier(t);
		View* v = ConsumeViewRefOrDef(t, ps, id);
		if (v)
		{
			ParserAssert(v->Type == ViewType::SRV || v->Type == ViewType::UAV ||
				v->Type == ViewType::Auto, "Referenced view (%s) must be SRV/UAV/Auto.",
				id);
			bind.Type = BindType::View;
			bind.ViewBind = v;
		}
		else
		{
			ParserAssert(ps.resMap.count(id) != 0, "Couldn't find resource %s", id);
			ParseState::Resource& res = ps.resMap[id];
			if (res.type == ParseState::ResType::Sampler)
			{
				bind.Type = BindType::Sampler;
				bind.SamplerBind = (Sampler*)res.m;
			}
			else
			{
				ParserError("Referenced resource (%s) must be SRV/UAV/Auto, or sampler.", 
					id);
			}
		}
	}
	return bind;
}

template <typename AstType>
AstType* AllocateAst(RenderDescription* rd)
{
	AstType* ast = new AstType();
	rd->Asts.push_back(ast);
	return ast;
} 

ast::Node* ConsumeAstRecurse(TokenIter& t, ParseState& ps)
{
	ast::Node* ast = nullptr;
	while (true)
	{
		TokenType tok = PeekNextToken(t);
		if (tok == TokenType::Comma || tok == TokenType::RParen || 
			tok == TokenType::Semicolon || tok == TokenType::RBrace)
		{
			break;
		}
		else if (tok == TokenType::Identifier)
		{
			ParserAssert(!ast, "expected op");
			const char* loc = t.next->Location;
			const char* id = ConsumeIdentifier(t);
			if (TryConsumeToken(TokenType::LParen, t))
			{
				ast::Function* func = AllocateAst<ast::Function>(ps.rd);
				func->Name = std::string(id);
				if (!TryConsumeToken(TokenType::RParen, t))
				{
					while (true)
					{
						ast::Node* arg = ConsumeAstRecurse(t, ps);
						func->Args.push_back(arg);
						if (!TryConsumeToken(TokenType::Comma, t))
							break;
					}
					ConsumeToken(TokenType::RParen, t);
				}
				ast = func;
			}
			else
			{
				ParserAssert(ps.varMap.count(id) == 1, "Variable %s not defined.", id);
				ParseState::Var& v = ps.varMap[id];
				ast::VariableRef* vr = AllocateAst<ast::VariableRef>(ps.rd);
				vr->isTuneable = v.tuneable;
				vr->M = v.m;
				ast = vr;
			}
			ast->Location = loc;
		}
		else if (tok == TokenType::Period)
		{
			ParserAssert(ast, "expected value")
			ConsumeToken(TokenType::Period, t);
			ast::Subscript* sub = AllocateAst<ast::Subscript>(ps.rd);
			sub->Subject = ast;
			sub->Location = t.next->Location;
			const char* ss = ConsumeIdentifier(t);
			size_t slen = strlen(ss);
			for (u32 i = 0 ; i < slen ; ++i)
			{
				char s = ss[i];
				if (s == 'x' || s == 'r')
					sub->Index[i] = 0;
				else if (s == 'y' || s == 'g')
					sub->Index[i] = 1;
				else if (s == 'z' || s == 'b')
					sub->Index[i] = 2;
				else if (s == 'w' || s == 'a')
					sub->Index[i] = 3;
				else
					ParserError("Unexpected subscript: %s", ss);
			}
			ast = sub;
		}
		else if (ast && (tok == TokenType::Plus || tok == TokenType::Minus || 
			tok == TokenType::Asterisk || tok == TokenType::ForwardSlash))
		{
			const char* loc = t.next->Location;
			ConsumeToken(tok, t);
			ast::Node* arg2 = ConsumeAstRecurse(t,ps);
			ast::BinaryOp::Type op;
			switch (tok)
			{
			case TokenType::Plus:
				op = ast::BinaryOp::Type::Add; break;
			case TokenType::Minus:
				op = ast::BinaryOp::Type::Subtract; break;
			case TokenType::Asterisk:
				op = ast::BinaryOp::Type::Multiply; break;
			case TokenType::ForwardSlash:
				op = ast::BinaryOp::Type::Divide; break;
			default:
				Unimplemented();
			}
			if (arg2->Spec == ast::Node::Special::Operator)
			{
				ast::BinaryOp* bop = static_cast<ast::BinaryOp*>(arg2);
				bop->Args.push_back(ast);
				bop->Ops.push_back(op);
				bop->Location = loc; // TODO: location per operator
				ast = bop;	
			}
			else if (arg2->Spec == ast::Node::Special::None)
			{
				ast::BinaryOp* bop = AllocateAst<ast::BinaryOp>(ps.rd);
				bop->Args.push_back(arg2);
				bop->Args.push_back(ast);
				bop->Ops.push_back(op);
				bop->Location = loc; // TODO: location per operator
				bop->Spec = ast::Node::Special::Operator;
				ast = bop;
			}
			else
				Unimplemented();
		}
		else if (!ast && tok == TokenType::Minus)
		{
			const char* loc = t.next->Location;
			TokenType tok2 = PeekNextToken(t);
			if (tok2 == TokenType::IntegerLiteral)
			{
				i32 i = ConsumeIntLiteral(t);
				ast::IntLiteral* il = AllocateAst<ast::IntLiteral>(ps.rd);
				il->Val = i;
				il->Location = loc;
				ast = il;
			}
			else if (tok2 == TokenType::FloatLiteral)
			{
				float f = ConsumeFloatLiteral(t);
				ast::FloatLiteral* fl = AllocateAst<ast::FloatLiteral>(ps.rd);
				fl->Val = f;
				fl->Location = loc;
				ast = fl;
			}
			else
			{
				ConsumeToken(TokenType::Minus, t);
				ast::IntLiteral* nl = AllocateAst<ast::IntLiteral>(ps.rd);
				nl->Val = -1;
				nl->Location = loc;
				ast::Node* arg = ConsumeAstRecurse(t,ps);
				if (arg->Spec == ast::Node::Special::Operator)
				{
					ast::BinaryOp* bop = static_cast<ast::BinaryOp*>(arg);
					bop->Args.push_back(nl);
					bop->Ops.push_back(ast::BinaryOp::Type::Multiply);
					bop->Location = loc; // TODO: location per operator
					ast = bop;	
				}
				else if (arg->Spec == ast::Node::Special::None)
				{
					ast::BinaryOp* bop = AllocateAst<ast::BinaryOp>(ps.rd);
					bop->Args.push_back(arg);
					bop->Args.push_back(nl);
					bop->Ops.push_back(ast::BinaryOp::Type::Multiply);
					bop->Location = loc; // TODO: location per operator
					bop->Spec = ast::Node::Special::Operator;
					ast = bop;
				}
				else
					Unimplemented();
			}
		}
		else if (tok == TokenType::IntegerLiteral)
		{
			ParserAssert(!ast, "expected op");
			const char* loc = t.next->Location;
			u32 u = ConsumeUintLiteral(t);
			ast::UintLiteral* ul = AllocateAst<ast::UintLiteral>(ps.rd);
			ul->Val = u;
			ul->Location = loc;
			ast = ul;
		}
		else if (tok == TokenType::FloatLiteral)
		{
			ParserAssert(!ast, "expected op");
			const char* loc = t.next->Location;
			float f = ConsumeFloatLiteral(t);
			ast::FloatLiteral* fl = AllocateAst<ast::FloatLiteral>(ps.rd);
			fl->Val = f;
			fl->Location = loc;
			ast = fl;
		}
		else if (tok == TokenType::LParen)
		{
			ParserAssert(!ast, "expected op")
			ConsumeToken(TokenType::LParen, t);
			ast::Group* grp = AllocateAst<ast::Group>(ps.rd);
			grp->Sub = ConsumeAstRecurse(t, ps);
			ast = grp;
			ConsumeToken(TokenType::RParen, t);
		}
		else if (tok == TokenType::LBrace)
		{
			ParserAssert(!ast, "expected op");
			const char* loc = t.next->Location;
			ConsumeToken(TokenType::LBrace, t);
			ast::Join* join = AllocateAst<ast::Join>(ps.rd);
			while (true)
			{
				ast::Node* arg = ConsumeAstRecurse(t, ps);
				join->Comps.push_back(arg);
				if (!TryConsumeToken(TokenType::Comma, t))
					break;
			}
			join->Location = loc;
			ast = join;
			ConsumeToken(TokenType::RBrace, t);
		}
		else
			ParserError("Unexpected token given: %s", TokenNames[(u32)tok]);
	}
	ParserAssert(ast, "No expression given.");
	return ast;
}

ast::Node* ConsumeAst(TokenIter& t, ParseState& ps)
{
	ast::Node* ast = ConsumeAstRecurse(t, ps);
	ast->GetDependency(ast->Dep);
	return ast;
}

SetConstant ConsumeSetConstant(TokenIter& t, ParseState& ps)
{
	const char* varName = ConsumeIdentifier(t);
	ConsumeToken(TokenType::Equals, t);
	SetConstant sc = {};
	sc.VariableName = AddStringToDescriptionData(varName, ps.rd);
	sc.Value = ConsumeAst(t, ps);
	ConsumeToken(TokenType::Semicolon, t);
	return sc;
}

void ConsumeVariable(VariableType type, Variable& var, TokenIter& t)
{
	for (u32 i = 0 ; i < type.Dim ; ++i)
	{
		if (i != 0)
			ConsumeToken(TokenType::Comma, t);
		switch (type.Fmt)
		{
		case VariableFormat::Float:
			var.Float4Val.m[i] = ConsumeFloatLiteral(t);
			break;
		case VariableFormat::Bool:
			var.Bool4Val.m[i] = ConsumeBool(t);
			break;
		case VariableFormat::Int:
			var.Int4Val.m[i] = ConsumeIntLiteral(t);
			break;
		case VariableFormat::Uint:
			var.Uint4Val.m[i] = ConsumeUintLiteral(t);
			break;
		default:
			Unimplemented();
		}
	}
}

VariableType LookupVariableType(const char* id)
{
	VariableType type = BoolType;
	Keyword key = LookupKeyword(id);
	switch (key)
	{
	case Keyword::Float:
		type = FloatType;
		break;
	case Keyword::Float2:
		type = Float2Type;
		break;
	case Keyword::Float3:
		type = Float3Type;
		break;
	case Keyword::Float4:
		type = Float4Type;
		break;
	case Keyword::Bool:
		type = BoolType;
		break;
	case Keyword::Int:
		type = IntType;
		break;
	case Keyword::Int2:
		type = Int2Type;
		break;
	case Keyword::Int3:
		type = Int3Type;
		break;
	case Keyword::Int4:
		type = Int4Type;
		break;
	case Keyword::Uint:
		type = UintType;
		break;
	case Keyword::Uint2:
		type = Uint2Type;
		break;
	case Keyword::Uint3:
		type = Uint3Type;
		break;
	case Keyword::Uint4:
		type = Uint4Type;
		break;
	case Keyword::Float4x4:
		type = Float4x4Type;
		break;
	default:
		ParserError("Unexpected type: %s", id);
	}
	return type;
}

Tuneable* ConsumeTuneable(TokenIter& t, ParseState& ps)
{
	Tuneable* tune = new Tuneable();
	ps.rd->Tuneables.push_back(tune);

	const char* typeId = ConsumeIdentifier(t);
	tune->Type = LookupVariableType(typeId);

	const char* nameId = ConsumeIdentifier(t);
	tune->Name = AddStringToDescriptionData(nameId, ps.rd);
	ParserAssert(ps.varMap.count(nameId) == 0, "Variable %s already defined", 
		nameId);
	ParseState::Var v;
	v.tuneable = true;
	v.m = tune;
	ps.varMap[nameId] = v;

	bool hasRange = false;

	if (tune->Type != BoolType && TryConsumeToken(TokenType::LBracket,t))
	{
		hasRange = true;
		VariableType mmType = tune->Type;
		mmType.Dim = 1;
		ConsumeVariable(mmType, tune->Min, t);
		ConsumeToken(TokenType::Comma,t);
		ConsumeVariable(mmType, tune->Max, t);
		ConsumeToken(TokenType::RBracket,t);
	}

	ConsumeToken(TokenType::Equals, t);
	ConsumeVariable(tune->Type, tune->Value, t);

	if (hasRange)
	{
		switch (tune->Type.Fmt)
		{
		case VariableFormat::Float:
			ParserAssert(tune->Min.FloatVal < tune->Max.FloatVal && 
				tune->Min.FloatVal <= tune->Value.FloatVal && 
				tune->Value.FloatVal <= tune->Max.FloatVal, 
				"Invalid tuneable range and default, %f <= %f <= %f",
				tune->Min.FloatVal, tune->Value.FloatVal, tune->Max.FloatVal);
			break;
		case VariableFormat::Bool:
			break;
		case VariableFormat::Int:
			ParserAssert(tune->Min.IntVal < tune->Max.IntVal && 
				tune->Min.IntVal <= tune->Value.IntVal && 
				tune->Value.IntVal <= tune->Max.IntVal, 
				"Invalid tuneable range and default, %d <= %d <= %d",
				tune->Min.IntVal, tune->Value.IntVal, tune->Max.IntVal);
			break;
		case VariableFormat::Uint:
			ParserAssert(tune->Min.UintVal < tune->Max.UintVal && 
				tune->Min.UintVal <= tune->Value.UintVal && 
				tune->Value.UintVal <= tune->Max.UintVal, 
				"Invalid tuneable range and default, %u <= %u <= %u",
				tune->Min.UintVal, tune->Value.UintVal, tune->Max.UintVal);
			break;
		default:
			Unimplemented();
		}
	}

	ConsumeToken(TokenType::Semicolon, t);

	return tune;
}

Constant* ConsumeConstant(TokenIter& t, ParseState& ps)
{
	Constant* cnst = new Constant();
	ps.rd->Constants.push_back(cnst);

	const char* typeId = ConsumeIdentifier(t);
	cnst->Type = LookupVariableType(typeId);

	const char* nameId = ConsumeIdentifier(t);
	cnst->Name = AddStringToDescriptionData(nameId, ps.rd);
	ParserAssert(ps.varMap.count(nameId) == 0, "Variable %s already defined", nameId);

	ConsumeToken(TokenType::Equals, t);
	cnst->Expr = ConsumeAst(t,ps);
	
	ParseState::Var v;
	v.tuneable = false;
	v.m = cnst;
	ps.varMap[nameId] = v;

	ConsumeToken(TokenType::Semicolon, t);

	return cnst;
}

// -----------------------------------------------------------------------------
// ------------------------------ STRUCTS --------------------------------------
// -----------------------------------------------------------------------------
enum class ConsumeType
{
	Bool,
	Int,
	Uchar,
	Uint,
	Uint2,
	Float,
	Float4,
	String,
	AddressModeUVW,
	Ast,
	ComparisonFunc,
	CullMode,
	FilterMode,
	StencilOp,
	StencilOpDesc,
	Texture,
	TextureFlag,
	TextureFormat,
};
struct StructEntry
{
	Keyword Key;
	ConsumeType Type;
	size_t Offset;
};
#define StructEntryDef(struc, type, field) Keyword::field, ConsumeType::type, offsetof(struc, field)
#define StructEntryDefEx(struc, type, name, field) Keyword::name, ConsumeType::type, offsetof(struc, field)

StencilOpDesc ConsumeStencilOpDesc(TokenIter& t);

template <typename T>
void ConsumeField(TokenIter& t, T* s, ConsumeType type, size_t offset)
{
	u8* p = (u8*)s;
	p += offset;
	switch (type)
	{
	case ConsumeType::Bool:
		*(bool*)p = ConsumeBool(t);
		break;
	case ConsumeType::Int:
		*(i32*)p = ConsumeIntLiteral(t);
		break;
	case ConsumeType::Uchar:
		*(u8*)p = ConsumeUcharLiteral(t);
		break;
	case ConsumeType::Uint:
		*(u32*)p = ConsumeUintLiteral(t);
		break;
	case ConsumeType::Uint2:
	{
		ConsumeToken(TokenType::LBrace, t);
		uint2& u2 = *(uint2*)p;
		u2.x = ConsumeUintLiteral(t);
		ConsumeToken(TokenType::Comma, t);
		u2.y = ConsumeUintLiteral(t);
		ConsumeToken(TokenType::RBrace, t);
		break;
	}
	case ConsumeType::Float:
		*(float*)p = ConsumeFloatLiteral(t);
		break;
	case ConsumeType::Float4:
	{
		float4& f4 = *(float4*)p;
		ConsumeToken(TokenType::LBrace, t);
		f4.x = ConsumeFloatLiteral(t);
		ConsumeToken(TokenType::Comma, t);
		f4.y = ConsumeFloatLiteral(t);
		ConsumeToken(TokenType::Comma, t);
		f4.z = ConsumeFloatLiteral(t);
		ConsumeToken(TokenType::Comma, t);
		f4.w = ConsumeFloatLiteral(t);
		ConsumeToken(TokenType::RBrace, t);
		break;
	}
	case ConsumeType::String:
	{
		const char* str = ConsumeString(t);
		*(const char**)p = AddStringToDescriptionData(str, GPS->rd);
		break;
	}
	case ConsumeType::AddressModeUVW:
		*(AddressModeUVW*)p = ConsumeAddressModeUVW(t);
		break;
	case ConsumeType::Ast:
		*(ast::Node**)p = ConsumeAst(t, *GPS);
		break;
	case ConsumeType::ComparisonFunc:
		*(ComparisonFunc*)p = ConsumeComparisonFunc(t);
		break;
	case ConsumeType::CullMode:
		*(CullMode*)p = ConsumeCullMode(t);
		break;
	case ConsumeType::FilterMode:
		*(FilterMode*)p = ConsumeFilterMode(t);
		break;
	case ConsumeType::StencilOp:
		*(StencilOp*)p = ConsumeStencilOp(t);
		break;
	case ConsumeType::StencilOpDesc:
		*(StencilOpDesc*)p = ConsumeStencilOpDesc(t);
		break;
	case ConsumeType::Texture:
	{
		const char* id = ConsumeIdentifier(t);
		ParserAssert(GPS->resMap.count(id) != 0, "Couldn't find resource %s", id);
		ParseState::Resource& res = GPS->resMap[id];
		ParserAssert(res.type == ParseState::ResType::Texture, "Resource must be texture");
		*(Texture**)p = reinterpret_cast<Texture*>(res.m);
		break;
	}
	case ConsumeType::TextureFlag:
		*(TextureFlag*)p = ConsumeTextureFlag(t);
		break;
	case ConsumeType::TextureFormat:
	{
		const char* formatId = ConsumeIdentifier(t);
		u32 hash = LowerHash(formatId);
		ParserAssert(GPS->fmtMap.count(hash) != 0, "Couldn't find format %s", 
			formatId);
		*(TextureFormat*)p = GPS->fmtMap[hash];
		break;
	}
	default:
		Unimplemented();
	}
}
template <TokenType Delim, bool TrailingRequired, typename T, size_t DefSize>
void ConsumeStruct(TokenIter& t, T* s, StructEntry (&def)[DefSize], const char* name)
{
	ConsumeToken(TokenType::LBrace, t);

	while (true)
	{
		if (TryConsumeToken(TokenType::RBrace, t))
			break;

		const char* id = ConsumeIdentifier(t);
		Keyword key = LookupKeyword(id);
		ConsumeToken(TokenType::Equals, t);
		bool found = false;
		for (size_t i = 0 ; i < DefSize ; ++i)
		{
			if (key == def[i].Key)
			{
				ConsumeField(t, s, def[i].Type, def[i].Offset);
				found = true;
				break;
			}
		}
		ParserAssert(found, "Unexpected field (%s) in struct %s", id, name)

		if (TrailingRequired)
		{
			ConsumeToken(Delim, t);
		}
		else if (!TryConsumeToken(Delim, t))
		{
			ConsumeToken(TokenType::RBrace, t);
			break;
		}
	}
}


RasterizerState* ConsumeRasterizerStateDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	RasterizerState* rs = new RasterizerState();
	rd->RasterizerStates.push_back(rs);

	// non-zero defaults
	rs->Fill = true;
	rs->CullMode = CullMode::None;
	rs->DepthClipEnable = true;

	static StructEntry def[] = {
		StructEntryDef(RasterizerState, Bool, Fill),
		StructEntryDef(RasterizerState, CullMode, CullMode),
		StructEntryDef(RasterizerState, Bool, FrontCCW),
		StructEntryDef(RasterizerState, Int, DepthBias),
		StructEntryDef(RasterizerState, Float, SlopeScaledDepthBias),
		StructEntryDef(RasterizerState, Float, DepthBiasClamp),
		StructEntryDef(RasterizerState, Float, DepthClipEnable),
		StructEntryDef(RasterizerState, Bool, ScissorEnable),
		StructEntryDef(RasterizerState, Bool, MultisampleEnable),
		StructEntryDef(RasterizerState, Bool, AntialiasedLineEnable),
	};
	constexpr TokenType Delim = TokenType::Semicolon;
	constexpr bool TrailingRequired = true;
	ConsumeStruct<Delim, TrailingRequired>(
		t, rs, def, "RasterizerState");
	return rs;
}

RasterizerState* ConsumeRasterizerStateRefOrDef(
	TokenIter& t,
	ParseState& ps)
{
	const char* id = ConsumeIdentifier(t);
	Keyword key = LookupKeyword(id);
	if (key == Keyword::RasterizerState)
	{
		return ConsumeRasterizerStateDef(t,ps);
	}
	else
	{
		ParserAssert(ps.rsMap.count(id) != 0, "couldn't find rasterizer state %s", id);
		return ps.rsMap[id];
	}
}

StencilOpDesc ConsumeStencilOpDesc(TokenIter& t)
{
	StencilOpDesc desc = {};
	// non-zero defaults
	desc.StencilFailOp = StencilOp::Keep;
	desc.StencilDepthFailOp = StencilOp::Keep;
	desc.StencilPassOp = StencilOp::Keep;
	desc.StencilFunc = ComparisonFunc::Always;
	static StructEntry def[] = {
		StructEntryDef(StencilOpDesc, StencilOp, StencilFailOp),
		StructEntryDef(StencilOpDesc, StencilOp, StencilDepthFailOp),
		StructEntryDef(StencilOpDesc, StencilOp, StencilPassOp),
		StructEntryDef(StencilOpDesc, ComparisonFunc, StencilFunc),
	};
	constexpr TokenType Delim = TokenType::Semicolon;
	constexpr bool TrailingRequired = true;
	ConsumeStruct<Delim, TrailingRequired>(
		t, &desc, def, "StencilOpDesc");
	return desc;
}

DepthStencilState* ConsumeDepthStencilStateDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	DepthStencilState* dss = new DepthStencilState();
	rd->DepthStencilStates.push_back(dss);

	// non-zero defaults
	dss->DepthEnable = true;
	dss->DepthWrite = true;
	dss->DepthFunc = ComparisonFunc::Less;
	dss->StencilEnable = false;
	dss->StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dss->StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	dss->FrontFace.StencilFailOp = dss->BackFace.StencilFailOp = StencilOp::Keep;
	dss->FrontFace.StencilDepthFailOp = dss->BackFace.StencilDepthFailOp = StencilOp::Keep;
	dss->FrontFace.StencilPassOp = dss->BackFace.StencilPassOp = StencilOp::Keep;
	dss->FrontFace.StencilFunc = dss->BackFace.StencilFunc = ComparisonFunc::Always;

	static StructEntry def[] = {
		StructEntryDef(DepthStencilState, Bool, DepthEnable),
		StructEntryDef(DepthStencilState, Bool, DepthWrite),
		StructEntryDef(DepthStencilState, ComparisonFunc, DepthFunc),
		StructEntryDef(DepthStencilState, Bool, StencilEnable),
		StructEntryDef(DepthStencilState, Uchar, StencilReadMask),
		StructEntryDef(DepthStencilState, Uchar, StencilReadMask),
		StructEntryDef(DepthStencilState, StencilOpDesc, FrontFace),
		StructEntryDef(DepthStencilState, StencilOpDesc, BackFace),
	};
	constexpr TokenType Delim = TokenType::Semicolon;
	constexpr bool TrailingRequired = true;
	ConsumeStruct<Delim, TrailingRequired>(
		t, dss, def, "DepthStencilState");
	return dss;
}

DepthStencilState* ConsumeDepthStencilStateRefOrDef(
	TokenIter& t,
	ParseState& ps)
{
	const char* id = ConsumeIdentifier(t);
	Keyword key = LookupKeyword(id);
	if (key == Keyword::DepthStencilState)
	{
		return ConsumeDepthStencilStateDef(t,ps);
	}
	else
	{
		ParserAssert(ps.dssMap.count(id) != 0, "couldn't find depthstencil state %s", id);
		return ps.dssMap[id];
	}
}

Viewport* ConsumeViewportDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	Viewport* vp = new Viewport();
	rd->Viewports.push_back(vp);

	static StructEntry def[] = {
		StructEntryDef(Viewport, Ast, TopLeft),
		StructEntryDef(Viewport, Ast, Size),
		StructEntryDef(Viewport, Ast, DepthRange),
	};
	constexpr TokenType Delim = TokenType::Semicolon;
	constexpr bool TrailingRequired = true;
	ConsumeStruct<Delim, TrailingRequired>(
		t, vp, def, "Viewport");
	return vp;
}

Viewport* ConsumeViewportRefOrDef(
	TokenIter& t,
	ParseState& ps)
{
	const char* id = ConsumeIdentifier(t);
	Keyword key = LookupKeyword(id);
	if (key == Keyword::Viewport)
	{
		return ConsumeViewportDef(t,ps);
	}
	else
	{
		ParserAssert(ps.vpMap.count(id) != 0, "couldn't find viewport %s", id);
		return ps.vpMap[id];
	}
}

ComputeShader* ConsumeComputeShaderDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ComputeShader* cs = new ComputeShader();
	rd->CShaders.push_back(cs);

	static StructEntry def[] = {
		StructEntryDef(ComputeShader, String, ShaderPath),
		StructEntryDef(ComputeShader, String, EntryPoint),
	};
	constexpr TokenType Delim = TokenType::Semicolon;
	constexpr bool TrailingRequired = true;
	ConsumeStruct<Delim, TrailingRequired>(
		t, cs, def, "ComputeShader");
	return cs;
}

ComputeShader* ConsumeComputeShaderRefOrDef(
	TokenIter& t,
	ParseState& ps)
{
	const char* id = ConsumeIdentifier(t);
	Keyword key = LookupKeyword(id);
	if (key == Keyword::ComputeShader)
	{
		return ConsumeComputeShaderDef(t, ps);
	}
	else
	{
		ParserAssert(ps.csMap.count(id) != 0, "couldn't find shader %s", id);
		return ps.csMap[id];
	}
}

VertexShader* ConsumeVertexShaderDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	VertexShader* vs = new VertexShader();
	rd->VShaders.push_back(vs);

	static StructEntry def[] = {
		StructEntryDef(VertexShader, String, ShaderPath),
		StructEntryDef(VertexShader, String, EntryPoint),
	};
	constexpr TokenType Delim = TokenType::Semicolon;
	constexpr bool TrailingRequired = true;
	ConsumeStruct<Delim, TrailingRequired>(
		t, vs, def, "VertexShader");
	return vs;
}

VertexShader* ConsumeVertexShaderRefOrDef(
	TokenIter& t,
	ParseState& ps)
{
	const char* id = ConsumeIdentifier(t);
	Keyword key = LookupKeyword(id);
	if (key == Keyword::VertexShader)
	{
		return ConsumeVertexShaderDef(t, ps);
	}
	else
	{
		ParserAssert(ps.vsMap.count(id) != 0, "couldn't find shader %*s", id);
		return ps.vsMap[id];
	}
}

PixelShader* ConsumePixelShaderDef(
	TokenIter& t,
	ParseState& state)
{
	RenderDescription* rd = state.rd;

	PixelShader* ps = new PixelShader();
	rd->PShaders.push_back(ps);

	static StructEntry def[] = {
		StructEntryDef(PixelShader, String, ShaderPath),
		StructEntryDef(PixelShader, String, EntryPoint),
	};
	constexpr TokenType Delim = TokenType::Semicolon;
	constexpr bool TrailingRequired = true;
	ConsumeStruct<Delim, TrailingRequired>(
		t, ps, def, "PixelShader");
	return ps;
}

PixelShader* ConsumePixelShaderRefOrDef(
	TokenIter& t,
	ParseState& ps)
{
	const char* id = ConsumeIdentifier(t);
	Keyword key = LookupKeyword(id);
	if (key == Keyword::PixelShader)
	{
		return ConsumePixelShaderDef(t, ps);
	}
	else
	{
		ParserAssert(ps.psMap.count(id) != 0, "couldn't find shader %s", id);
		return ps.psMap[id];
	}
}

void ParseOBJ(ObjImport* import, RenderDescription* rd, ParseState& ps)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	std::string path = ps.workingDirectory;
	path += import->ObjPath;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str(),
		"./", true);
	ParserAssert(ret, "failed to load obj file: %s", err.c_str());

	struct IndexEqual
	{
		bool operator()(const tinyobj::index_t& l, const tinyobj::index_t& r) const
		{
			if (l.vertex_index != r.vertex_index)
				return false;
			if (l.normal_index != r.normal_index)
				return false;
			if (l.texcoord_index != r.texcoord_index)
				return false;
			return true;
		}
	};
	struct IndexHash
	{
		size_t operator()(const tinyobj::index_t& i) const
		{
			size_t h = 5381;
			h = ((h << 5) + h) + i.vertex_index;
			h = ((h << 5) + h) + i.normal_index;
			h = ((h << 5) + h) + i.texcoord_index;
			return h; 
		}
	};
	std::unordered_map<tinyobj::index_t, size_t, IndexHash, IndexEqual> map;

	struct Vertex {
		float3 v;
		float3 vn;
		float2 vt;
	};
	std::vector<Vertex> verts;
	std::vector<u32> indices;
	bool isU16 = true;

	for (tinyobj::shape_t& shape : shapes)
	{
		size_t indexOffset = 0;
		tinyobj::mesh_t& mesh = shape.mesh;
		for (size_t fv : mesh.num_face_vertices)
		{
			ParserAssert(fv == 3, "un-triangulated faces not supported.");
			for (size_t v = 0 ; v < fv ; ++v)
			{
				tinyobj::index_t idx = mesh.indices[indexOffset + v];

				auto search = map.find(idx);
				if (search != map.end())
				{
					indices.push_back((u32)search->second);
					continue;
				}

				Vertex vert = {};

				tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
				tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
				tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];
				vert.v = {vx, vy, vz};

				// Check if `normal_index` is zero or positive. negative = no normal data
				if (idx.normal_index >= 0)
				{
					tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
					tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
					tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];
					vert.vn = {nx, ny, nz};
				}

				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
					tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
					vert.vt = {tx, ty};
				}

				size_t index = verts.size();
				if (index > USHRT_MAX)
					isU16 = false;
				verts.push_back(vert);
				indices.push_back((u32)index);
				map[idx] = index;
			}
			indexOffset += fv;
		}
	}

	import->U16 = isU16;
	import->VertexCount = (u32)verts.size();
	import->IndexCount = (u32)indices.size();

	size_t vertsSize = sizeof(Vertex) * verts.size();
	import->Vertices = malloc(vertsSize);
	AddMemToDescriptionData(import->Vertices, rd);
	memcpy(import->Vertices, verts.data(), vertsSize);

	size_t indicesSize = (isU16 ? 2 : 4) * indices.size();
	import->Indices = malloc(indicesSize);
	AddMemToDescriptionData(import->Indices, rd);
	if (isU16)
	{
		u16* shorts = (u16*)import->Indices;
		for (size_t i = 0 ; i < indices.size() ; ++i)
		{
			u32 val = indices[i];
			Assert(val <= USHRT_MAX, "mismatch in expected size");
			shorts[i] = (u16)val;
		}
	}
	else
	{
		memcpy(import->Indices, indices.data(), indicesSize);
	}
}

ObjImport* ConsumeObjImportDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(TokenType::LBrace, t);

	ObjImport* obj = new ObjImport();
	rd->Objs.push_back(obj);

	const char* fieldId = ConsumeIdentifier(t);
	if (LookupKeyword(fieldId) == Keyword::ObjPath)
	{
		ConsumeToken(TokenType::Equals, t);
		const char* path = ConsumeString(t);
		obj->ObjPath = AddStringToDescriptionData(path, rd);
	}

	ConsumeToken(TokenType::Semicolon, t);
	ConsumeToken(TokenType::RBrace, t);

	ParseOBJ(obj, rd, ps);

	return obj;
}


Buffer* ConsumeBufferDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(TokenType::LBrace, t);

	Buffer* buf = new Buffer();
	rd->Buffers.push_back(buf);

	std::vector<u16> initDataU16;
	std::vector<u32> initDataU32;
	std::vector<float> initDataFloat;
	bool initToZero = false;

	ObjImport* obj = nullptr;
	bool objVerts = false;

	while (true)
	{
		const char* fieldId = ConsumeIdentifier(t);
		switch (LookupKeyword(fieldId))
		{
		case Keyword::ElementSize:
		{
			ConsumeToken(TokenType::Equals, t);
			buf->ElementSize = ConsumeUintLiteral(t);
			break;
		}
		case Keyword::ElementCount:
		{
			ConsumeToken(TokenType::Equals, t);
			buf->ElementCount = ConsumeUintLiteral(t);
			break;
		}
		case Keyword::Flags:
		{
			ConsumeToken(TokenType::Equals, t);
			buf->Flags = ConsumeBufferFlag(t);
			break;
		}
		case Keyword::InitToZero:
		{
			ConsumeToken(TokenType::Equals, t);
			initToZero = ConsumeBool(t);
			break;
		}
		case Keyword::InitData:
		{
			ConsumeToken(TokenType::Equals, t);
			const char* id = ConsumeIdentifier(t);
			Keyword key = LookupKeyword(id);
			if (key == Keyword::Float)
			{
				ConsumeToken(TokenType::LBrace, t);

				while (true)
				{
					if (TryConsumeToken(TokenType::RBrace, t))
						break;

					float f = ConsumeFloatLiteral(t);
					initDataFloat.push_back(f);

					if (TryConsumeToken(TokenType::RBrace, t))
						break;
					else 
						ConsumeToken(TokenType::Comma, t);
				}
			}
			else if (key == Keyword::U16)
			{
				ConsumeToken(TokenType::LBrace, t);

				while (true)
				{
					if (TryConsumeToken(TokenType::RBrace, t))
						break;

					u32 val = ConsumeUintLiteral(t);
					ParserAssert(val < 65536, "Given literal is outside of u16 range.");
					u16 l = (u16)val;
					initDataU16.push_back(l);

					if (TryConsumeToken(TokenType::RBrace, t))
						break;
					else 
						ConsumeToken(TokenType::Comma, t);
				}
			}
			else if (key == Keyword::U32)
			{
				ConsumeToken(TokenType::LBrace, t);

				while (true)
				{
					if (TryConsumeToken(TokenType::RBrace, t))
						break;
					
					u32 val = ConsumeUintLiteral(t);
					initDataU32.push_back(val);

					if (TryConsumeToken(TokenType::RBrace, t))
						break;
					else 
						ConsumeToken(TokenType::Comma, t);
				}
			}
			else if (ps.objMap.count(id) > 0)
			{
				obj = ps.objMap[id];
				ConsumeToken(TokenType::Period, t);
				const char* sub = ConsumeIdentifier(t);
				Keyword subkey = LookupKeyword(sub);
				if (subkey == Keyword::Vertices)
				{
					objVerts = true;
				}
				else if (subkey == Keyword::Indices)
				{
					objVerts = false;
				}
				else
				{
					ParserError("unexpected obj subscript %s", id);
				}
			}
			else
			{
				ParserError("unexpected data type or obj import %s", id);
			}
			break;
		}
		default:
			ParserError("unexpected field %s", fieldId);
		}

		ConsumeToken(TokenType::Semicolon, t);

		if (TryConsumeToken(TokenType::RBrace, t))
			break;
	}

	if (obj)
	{
		if (objVerts)
		{
			buf->InitData = obj->Vertices;
			buf->ElementSize = 32;
			buf->ElementCount = obj->VertexCount;
			// TODO: configurable obj outputs
		}
		else
		{
			buf->InitData = obj->Indices;
			buf->ElementSize = obj->U16 ? 2 : 4;
			buf->ElementCount = obj->IndexCount;
		}
	}
	else
	{
		u32 bufSize = buf->ElementSize * buf->ElementCount;
		ParserAssert(bufSize > 0, "Buffer size not given");
		if (initToZero || initDataU16.size() > 0 || initDataU32.size() > 0 || initDataFloat.size() > 0)
		{
			float* data = (float*)malloc(bufSize);
			AddMemToDescriptionData(data, rd);
			buf->InitData = data;
		}

		if (initToZero)
		{
			ZeroMemory(buf->InitData, bufSize);
		}
		else if (initDataFloat.size() > 0)
		{
			ParserAssert(bufSize == initDataFloat.size() * sizeof(float), 
				"Buffer/init-data size mismatch.");
			memcpy(buf->InitData, initDataFloat.data(), bufSize);
		}
		else if (initDataU16.size() > 0)
		{
			ParserAssert(bufSize == initDataU16.size() * sizeof(u16), 
				"Buffer/init-data size mismatch.");
			memcpy(buf->InitData, initDataU16.data(), bufSize);
		}
		else if (initDataU32.size() > 0)
		{
			ParserAssert(bufSize == initDataU32.size() * sizeof(u32), 
				"Buffer/init-data size mismatch.");
			memcpy(buf->InitData, initDataU32.data(), bufSize);
		}
	}

	return buf;
}

Texture* ConsumeTextureDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	Texture* tex = new Texture();
	rd->Textures.push_back(tex);

	// non-zero defaults
	tex->Format = TextureFormat::R8G8B8A8_UNORM;
	tex->SampleCount = 1;

	static StructEntry def[] = {
		StructEntryDef(Texture, TextureFlag, Flags),
		StructEntryDefEx(Texture, Ast, Size, SizeExpr),
		StructEntryDef(Texture, TextureFormat, Format),
		StructEntryDef(Texture, String, DDSPath),
		StructEntryDef(Texture, Uint, SampleCount),
	};
	constexpr TokenType Delim = TokenType::Semicolon;
	constexpr bool TrailingRequired = true;
	ConsumeStruct<Delim, TrailingRequired>(
		t, tex, def, "Texture");

	ParserAssert(tex->DDSPath || tex->SizeExpr, 
		"Texture size must be provided if not populated from DDS");
	ParserAssert(!tex->SizeExpr || !tex->SizeExpr->VariesByTime(), 
		"Texture size may not vary by time.");

	return tex;
}

Sampler* ConsumeSamplerDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	Sampler* s = new Sampler();
	rd->Samplers.push_back(s);

	// non-zero defaults
	s->MinLOD = -FLT_MAX;
	s->MaxLOD = FLT_MAX;
	s->MaxAnisotropy = 1;
	s->BorderColor = {1,1,1,1};

	static StructEntry def[] = {
		StructEntryDef(Sampler, FilterMode, Filter),
		StructEntryDef(Sampler, AddressModeUVW, AddressMode),
		StructEntryDef(Sampler, Float, MipLODBias),
		StructEntryDef(Sampler, Uint, MaxAnisotropy),
		StructEntryDef(Sampler, Float4, BorderColor),
		StructEntryDef(Sampler, Float, MinLOD),
		StructEntryDef(Sampler, Float, MaxLOD),
	};
	constexpr TokenType Delim = TokenType::Semicolon;
	constexpr bool TrailingRequired = true;
	ConsumeStruct<Delim, TrailingRequired>(
		t, s, def, "Sampler");
	return s;
}

Dispatch* ConsumeDispatchDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(TokenType::LBrace, t);

	Dispatch* dc = new Dispatch();
	rd->Dispatches.push_back(dc);

	while (true)
	{
		const char* fieldId = ConsumeIdentifier(t);
		Keyword key = LookupKeyword(fieldId);
		switch (key)
		{
		case Keyword::Shader:
		{
			ConsumeToken(TokenType::Equals, t);
			ComputeShader* cs = ConsumeComputeShaderRefOrDef(t, ps);
			dc->Shader = cs;
			break;
		}
		case Keyword::ThreadPerPixel:
		{
			ConsumeToken(TokenType::Equals, t);
			dc->ThreadPerPixel = ConsumeBool(t);
			break;
		}
		case Keyword::Groups:
		{
			ConsumeToken(TokenType::Equals, t);
			dc->Groups = ConsumeAst(t, ps);
			break;
		}
		case Keyword::IndirectArgs:
		{
			ConsumeToken(TokenType::Equals, t);
			const char* id = ConsumeIdentifier(t);
			ParserAssert(ps.resMap.count(id) != 0, "Couldn't find resource %s", id);
			ParseState::Resource& res = ps.resMap[id];
			ParserAssert(res.type == ParseState::ResType::Buffer, 
				"Resource (%s) must be a buffer", id);
			dc->IndirectArgs = reinterpret_cast<Buffer*>(res.m);
			break;
		}
		case Keyword::IndirectArgsOffset:
		{
			ConsumeToken(TokenType::Equals, t);
			dc->IndirectArgsOffset = ConsumeUintLiteral(t);
			break;
		}
		case Keyword::Bind:
		{
			Bind bind = ConsumeBind(t, ps);
			dc->Binds.push_back(bind);
			break;
		}
		case Keyword::SetConstant:
		{
			SetConstant sc = ConsumeSetConstant(t,ps);
			dc->Constants.push_back(sc);
			break;
		}
		default:
			ParserError("unexpected field %s", fieldId);
		}

		if (key != Keyword::SetConstant)
			ConsumeToken(TokenType::Semicolon, t);

		if (TryConsumeToken(TokenType::RBrace, t))
			break;
	}

	return dc;
}

Draw* ConsumeDrawDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(TokenType::LBrace, t);

	Draw* draw = new Draw();
	rd->Draws.push_back(draw);

	// non-zero defaults
	draw->Topology = Topology::TriList;

	while (true)
	{
		const char* fieldId = ConsumeIdentifier(t);
		Keyword key = LookupKeyword(fieldId);
		switch (key)
		{
		case Keyword::Topology:
		{
			ConsumeToken(TokenType::Equals, t);
			draw->Topology = ConsumeTopology(t);
			break;
		}
		case Keyword::RState:
		{
			ConsumeToken(TokenType::Equals, t);
			draw->RState = ConsumeRasterizerStateRefOrDef(t, ps);
			break;
		}
		case Keyword::DSState:
		{
			ConsumeToken(TokenType::Equals, t);
			draw->DSState = ConsumeDepthStencilStateRefOrDef(t, ps);
			break;
		}
		case Keyword::VShader:
		{
			ConsumeToken(TokenType::Equals, t);
			draw->VShader = ConsumeVertexShaderRefOrDef(t, ps);
			break;
		}
		case Keyword::PShader:
		{
			ConsumeToken(TokenType::Equals, t);
			draw->PShader = ConsumePixelShaderRefOrDef(t, ps);
			break;
		}
		case Keyword::VertexBuffer:
		{
			ConsumeToken(TokenType::Equals, t);
			const char* id = ConsumeIdentifier(t);
			ParserAssert(ps.resMap.count(id) != 0, "Couldn't find resource %s", id);
			ParseState::Resource& res = ps.resMap[id];
			ParserAssert(res.type == ParseState::ResType::Buffer, 
				"Resource (%s) must be a buffer", id);
			draw->VertexBuffer = reinterpret_cast<Buffer*>(res.m);
			break;
		}
		case Keyword::IndexBuffer:
		{
			ConsumeToken(TokenType::Equals, t);
			const char* id = ConsumeIdentifier(t);
			ParserAssert(ps.resMap.count(id) != 0, "Couldn't find resource %s", 
				id);
			ParseState::Resource& res = ps.resMap[id];
			ParserAssert(res.type == ParseState::ResType::Buffer, 
				"Resource (%s) must be a buffer", id);
			draw->IndexBuffer = reinterpret_cast<Buffer*>(res.m);
			break;
		}
		case Keyword::VertexCount:
		{
			ConsumeToken(TokenType::Equals, t);
			draw->VertexCount = ConsumeUintLiteral(t);
			break;
		}
		case Keyword::InstanceCount:
		{
			ConsumeToken(TokenType::Equals, t);
			draw->InstanceCount = ConsumeUintLiteral(t);
			break;
		}
		case Keyword::StencilRef:
		{
			ConsumeToken(TokenType::Equals, t);
			draw->StencilRef = ConsumeUcharLiteral(t);
			break;
		}
		case Keyword::RenderTarget:
		{
			draw->RenderTargets.clear();
			ConsumeToken(TokenType::Equals, t);
			TextureTarget target;
			if (TryConsumeToken(TokenType::At, t))
			{
				target.IsSystem = true;
				target.System = ConsumeSystemValue(t);
			}
			else
			{
				target.IsSystem = false;
				const char* id = ConsumeIdentifier(t);
				View* v = ConsumeViewRefOrDef(t, ps, id);
				ParserAssert(v, "RenderTarget must be a RTV.");
				if (v->Type == ViewType::Auto)
					v->Type = ViewType::RTV;
				ParserAssert(v->Type == ViewType::RTV, "RenderTarget must be a RTV.");
				target.View = v;
			}
			draw->RenderTargets.push_back(target);
			break;
		}
		case Keyword::RenderTargets:
		{
			draw->RenderTargets.clear();
			ConsumeToken(TokenType::Equals, t);
			ConsumeToken(TokenType::LBrace, t);
			while (true)
			{
				if (TryConsumeToken(TokenType::RBrace, t))
					break;
				TextureTarget target;
				if (TryConsumeToken(TokenType::At, t))
				{
					target.IsSystem = true;
					target.System = ConsumeSystemValue(t);
				}
				else
				{
					target.IsSystem = false;
					const char* id = ConsumeIdentifier(t);
					View* v = ConsumeViewRefOrDef(t, ps, id);
					ParserAssert(v, "RenderTarget must be a RTV.");
					if (v->Type == ViewType::Auto)
						v->Type = ViewType::RTV;
					ParserAssert(v->Type == ViewType::RTV, "RenderTarget must be a RTV.");
					target.View = v;
				}
				draw->RenderTargets.push_back(target);
				if (!TryConsumeToken(TokenType::Comma, t))
				{
					ConsumeToken(TokenType::RBrace, t);
					break;
				}
			}
			break;
		}
		case Keyword::Viewports:
		{
			draw->Viewports.clear();
			ConsumeToken(TokenType::Equals, t);
			ConsumeToken(TokenType::LBrace, t);
			while (true)
			{
				if (TryConsumeToken(TokenType::RBrace, t))
					break;
				Viewport* vp = ConsumeViewportRefOrDef(t, ps);
				draw->Viewports.push_back(vp);
				if (!TryConsumeToken(TokenType::Comma, t))
				{
					ConsumeToken(TokenType::RBrace, t);
					break;
				}
			}
			break;
		}
		case Keyword::DepthStencil:
		{
			ConsumeToken(TokenType::Equals, t);
			TextureTarget target;
			if (TryConsumeToken(TokenType::At, t))
			{
				target.IsSystem = true;
				target.System = ConsumeSystemValue(t);
			}
			else
			{
				target.IsSystem = false;
				const char* id = ConsumeIdentifier(t);
				View* v = ConsumeViewRefOrDef(t, ps, id);
				ParserAssert(v, "DepthStencil must be a DSV.");
				if (v->Type == ViewType::Auto)
					v->Type = ViewType::DSV;
				ParserAssert(v->Type == ViewType::DSV, "DepthStencil must be a DSV.");
				target.View = v;
			}
			draw->DepthStencil.push_back(target);
			break;
		}
		case Keyword::BindVS:
		{
			Bind bind = ConsumeBind(t, ps);
			draw->VSBinds.push_back(bind);
			break;
		}
		case Keyword::BindPS:
		{
			Bind bind = ConsumeBind(t, ps);
			draw->PSBinds.push_back(bind);
			break;
		}
		case Keyword::SetConstantVS:
		{
			SetConstant sc = ConsumeSetConstant(t,ps);
			draw->VSConstants.push_back(sc);
			break;
		}
		case Keyword::SetConstantPS:
		{
			SetConstant sc = ConsumeSetConstant(t,ps);
			draw->PSConstants.push_back(sc);
			break;
		}
		default:
			ParserError("unexpected field %s", fieldId);
		}

		if (key != Keyword::SetConstantVS && key != Keyword::SetConstantPS)
			ConsumeToken(TokenType::Semicolon, t);

		if (TryConsumeToken(TokenType::RBrace, t))
			break;
	}

	return draw;
}

ClearColor* ConsumeClearColorDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(TokenType::LBrace, t);

	ClearColor* clear = new ClearColor();
	rd->ClearColors.push_back(clear);

	while (true)
	{
		const char* fieldId = ConsumeIdentifier(t);
		ConsumeToken(TokenType::Equals, t);
		Keyword key = LookupKeyword(fieldId);

		if (key == Keyword::Target)
		{
			const char* id = ConsumeIdentifier(t);
			View* v = ConsumeViewRefOrDef(t, ps, id);
			ParserAssert(v, "Target must be a RTV.");
			if (v->Type == ViewType::Auto)
				v->Type = ViewType::RTV;
			ParserAssert(v->Type == ViewType::RTV, "Target must be a RTV.");
			clear->Target = v;
		}
		else if (key == Keyword::Color)
		{
			ConsumeToken(TokenType::LBrace, t);
			clear->Color.x = ConsumeFloatLiteral(t);
			ConsumeToken(TokenType::Comma, t);
			clear->Color.y = ConsumeFloatLiteral(t);
			ConsumeToken(TokenType::Comma, t);
			clear->Color.z = ConsumeFloatLiteral(t);
			ConsumeToken(TokenType::Comma, t);
			clear->Color.w = ConsumeFloatLiteral(t);
			ConsumeToken(TokenType::RBrace, t);
		}
		else 
		{
			ParserError("unexpected field %s", fieldId);
		}

		ConsumeToken(TokenType::Semicolon, t);
		if (TryConsumeToken(TokenType::RBrace, t))
			break;
	}

	ParserAssert(clear->Target, "Target must be set.");
	return clear;
}

ClearDepth* ConsumeClearDepthDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(TokenType::LBrace, t);

	ClearDepth* clear = new ClearDepth();
	rd->ClearDepths.push_back(clear);

	while (true)
	{
		const char* fieldId = ConsumeIdentifier(t);
		ConsumeToken(TokenType::Equals, t);
		Keyword key = LookupKeyword(fieldId);

		if (key == Keyword::Target)
		{
			const char* id = ConsumeIdentifier(t);
			View* v = ConsumeViewRefOrDef(t, ps, id);
			ParserAssert(v, "Target must be a DSV.");
			if (v->Type == ViewType::Auto)
				v->Type = ViewType::DSV;
			ParserAssert(v->Type == ViewType::DSV, "Target must be a DSV.");
			clear->Target = v;
		}
		else if (key == Keyword::Depth)
		{
			clear->Depth = ConsumeFloatLiteral(t);
		}
		else 
		{
			ParserError("unexpected field %s", fieldId);
		}

		ConsumeToken(TokenType::Semicolon, t);
		if (TryConsumeToken(TokenType::RBrace, t))
			break;
	}

	ParserAssert(clear->Target, "Target must be set.");
	return clear;
}

ClearStencil* ConsumeClearStencilDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(TokenType::LBrace, t);

	ClearStencil* clear = new ClearStencil();
	rd->ClearStencils.push_back(clear);

	while (true)
	{
		const char* fieldId = ConsumeIdentifier(t);
		ConsumeToken(TokenType::Equals, t);
		Keyword key = LookupKeyword(fieldId);

		if (key == Keyword::Target)
		{
			const char* id = ConsumeIdentifier(t);
			View* v = ConsumeViewRefOrDef(t, ps, id);
			ParserAssert(v, "Target must be a DSV.");
			if (v->Type == ViewType::Auto)
				v->Type = ViewType::DSV;
			ParserAssert(v->Type == ViewType::DSV, "Target must be a DSV.");
			clear->Target = v;
		}
		else if (key == Keyword::Stencil)
		{
			clear->Stencil = ConsumeUcharLiteral(t);
		}
		else 
		{
			ParserError("unexpected field %s", fieldId);
		}

		ConsumeToken(TokenType::Semicolon, t);
		if (TryConsumeToken(TokenType::RBrace, t))
			break;
	}

	ParserAssert(clear->Target, "Target must be set.");
	return clear;
}

Resolve* ConsumeResolveDef(
	TokenIter& t,
	ParseState& ps)
{
	RenderDescription* rd = ps.rd;

	ConsumeToken(TokenType::LBrace, t);

	Resolve* resolve = new Resolve();
	rd->Resolves.push_back(resolve);

	while (true)
	{
		const char* fieldId = ConsumeIdentifier(t);
		ConsumeToken(TokenType::Equals, t);
		Keyword key = LookupKeyword(fieldId);

		if (key == Keyword::Src || key == Keyword::Dst)
		{
			const char* rid = ConsumeIdentifier(t);
			ParserAssert(ps.resMap.count(rid) != 0, "Couldn't find resource %s", 
				rid);
			ParseState::Resource& res = ps.resMap[rid];
			ParserAssert(res.type == ParseState::ResType::Texture, 
				"Referenced resource (%s) must be Texture.", rid);
			if (key == Keyword::Src)
				resolve->Src = (Texture*)res.m;
			else
				resolve->Dst = (Texture*)res.m;
		}
		else 
		{
			ParserError("unexpected field %s", fieldId);
		}

		ConsumeToken(TokenType::Semicolon, t);
		if (TryConsumeToken(TokenType::RBrace, t))
			break;
	}

	ParserAssert(resolve->Src, "Target must be set.");
	ParserAssert(resolve->Dst, "Target must be set.");
	return resolve;
}

Pass ConsumePassRefOrDef(
	TokenIter& t,
	ParseState& ps)
{
	const char* id = ConsumeIdentifier(t);
	Keyword key = LookupKeyword(id);
	Pass pass;
	pass.Name = nullptr; // Default to no name for the anonymous passes
	if (key == Keyword::Dispatch)
	{
		pass.Type = PassType::Dispatch;
		pass.Dispatch = ConsumeDispatchDef(t, ps);
	}
	else if (key == Keyword::Draw)
	{
		pass.Type = PassType::Draw;
		pass.Draw = ConsumeDrawDef(t, ps);
	}
	else if (key == Keyword::ClearColor)
	{
		pass.Type = PassType::ClearColor;
		pass.ClearColor = ConsumeClearColorDef(t, ps);
	}
	else if (key == Keyword::ClearDepth)
	{
		pass.Type = PassType::ClearDepth;
		pass.ClearDepth = ConsumeClearDepthDef(t, ps);
	}
	else if (key == Keyword::ClearStencil)
	{
		pass.Type = PassType::ClearStencil;
		pass.ClearStencil = ConsumeClearStencilDef(t, ps);
	}
	else if (key == Keyword::Resolve)
	{
		pass.Type = PassType::Resolve;
		pass.Resolve = ConsumeResolveDef(t, ps);
	}
	else
	{
		ParserAssert(ps.passMap.count(id) != 0, "couldn't find pass %s", id);
		pass = ps.passMap[id];
	}
	return pass;
}

void CheckPassName(const char* name)
{
	Keyword key = LookupKeyword(name);
	switch (key) {
		case Keyword::Dispatch:
		case Keyword::Draw:
		case Keyword::ClearColor:
		case Keyword::ClearDepth:
		case Keyword::ClearStencil:
		case Keyword::Resolve:
			ParserError("Pass types may not be used as identifiers: %s", name);
			break;
		default:
			break;
	} 
}

void ParseMain()
{
	ParseState& ps = *GPS;
	RenderDescription* rd = ps.rd;
	TokenIter& t = ps.t;

	while (t.next != t.end)
	{
		const char* id = ConsumeIdentifier(t);
		Keyword key = LookupKeyword(id);
		switch (key)
		{
		case Keyword::ComputeShader:
		{
			ComputeShader* cs = ConsumeComputeShaderDef(t, ps);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.csMap.count(nameId) == 0, "Shader %s already defined", nameId);
			ps.csMap[nameId] = cs;
			break;
		}
		case Keyword::VertexShader:
		{
			VertexShader* vs = ConsumeVertexShaderDef(t, ps);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.vsMap.count(nameId) == 0, "Shader %s already defined", nameId);
			ps.vsMap[nameId] = vs;
			break;
		}
		case Keyword::PixelShader:
		{
			PixelShader* pis = ConsumePixelShaderDef(t, ps);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.psMap.count(nameId) == 0, "Shader %s already defined", nameId);
			ps.psMap[nameId] = pis;
			break;
		}
		case Keyword::Buffer:
		{
			Buffer* buf = ConsumeBufferDef(t, ps);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.resMap.count(nameId) == 0, "Resource %s already defined", 
				nameId);
			ParseState::Resource& res = ps.resMap[nameId];
			res.type = ParseState::ResType::Buffer;
			res.m = buf;
			break;
		}
		case Keyword::Texture:
		{
			Texture* tex = ConsumeTextureDef(t,ps);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.resMap.count(nameId) == 0, "Resource %s already defined",
				nameId);
			ParseState::Resource& res = ps.resMap[nameId];
			res.type = ParseState::ResType::Texture;
			res.m = tex;
			break;
		}
		case Keyword::Sampler:
		{
			Sampler* s = ConsumeSamplerDef(t,ps);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.resMap.count(nameId) == 0, "Resource %s already defined",
				nameId);
			ParseState::Resource& res = ps.resMap[nameId];
			res.type = ParseState::ResType::Sampler;
			res.m = s;
			break;
		}
		case Keyword::SRV:
		{
			View* v = ConsumeViewDef(t,ps, ViewType::SRV);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.resMap.count(nameId) == 0, "Resource %s already defined",
				nameId);
			ParseState::Resource& res = ps.resMap[nameId];
			res.type = ParseState::ResType::View;
			res.m = v;
			break;
		}
		case Keyword::UAV:
		{
			View* v = ConsumeViewDef(t,ps, ViewType::UAV);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.resMap.count(nameId) == 0, "Resource %s already defined",
				nameId);
			ParseState::Resource& res = ps.resMap[nameId];
			res.type = ParseState::ResType::View;
			res.m = v;
			break;
		}
		case Keyword::RTV:
		{
			View* v = ConsumeViewDef(t,ps, ViewType::RTV);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.resMap.count(nameId) == 0, "Resource %s already defined",
				nameId);
			ParseState::Resource& res = ps.resMap[nameId];
			res.type = ParseState::ResType::View;
			res.m = v;
			break;
		}
		case Keyword::DSV:
		{
			View* v = ConsumeViewDef(t,ps, ViewType::DSV);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.resMap.count(nameId) == 0, "Resource %s already defined",
				nameId);
			ParseState::Resource& res = ps.resMap[nameId];
			res.type = ParseState::ResType::View;
			res.m = v;
			break;
		}
		case Keyword::RasterizerState:
		{
			RasterizerState* rs = ConsumeRasterizerStateDef(t,ps);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.rsMap.count(nameId) == 0, "Rasterizer state %s already defined",
				nameId);
			ps.rsMap[nameId] = rs;
			break;
		}
		case Keyword::DepthStencilState:
		{
			DepthStencilState* rs = ConsumeDepthStencilStateDef(t,ps);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.dssMap.count(nameId) == 0, "Rasterizer state %s already defined",
				nameId);
			ps.dssMap[nameId] = rs;
			break;
		}
		case Keyword::Viewport:
		{
			Viewport* vp = ConsumeViewportDef(t,ps);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.vpMap.count(nameId) == 0, "Viewport %s already defined",
				nameId);
			ps.vpMap[nameId] = vp;
			break;
		}
		case Keyword::ObjImport:
		{
			ObjImport* obj = ConsumeObjImportDef(t,ps);
			const char* nameId = ConsumeIdentifier(t);
			ParserAssert(ps.objMap.count(nameId) == 0, "Obj import %s already defined",
				nameId);
			ps.objMap[nameId] = obj;
			break;
		}
		case Keyword::Dispatch:
		{
			Dispatch* dc = ConsumeDispatchDef(t, ps);
			const char* nameId = ConsumeIdentifier(t);
			CheckPassName(nameId);
			ParserAssert(ps.passMap.count(nameId) == 0, "Pass %s already defined", 
				nameId);
			Pass pass;
			pass.Name = AddStringToDescriptionData(nameId, rd);
			pass.Type = PassType::Dispatch;
			pass.Dispatch = dc;
			ps.passMap[nameId] = pass;
			break;
		}
		case Keyword::Draw:
		{
			Draw* draw = ConsumeDrawDef(t, ps);
			const char* nameId = ConsumeIdentifier(t);
			CheckPassName(nameId);
			ParserAssert(ps.passMap.count(nameId) == 0, "Pass %s already defined", 
				nameId);
			Pass pass;
			pass.Name = AddStringToDescriptionData(nameId, rd);
			pass.Type = PassType::Draw;
			pass.Draw = draw;
			ps.passMap[nameId] = pass;
			break;
		}
		case Keyword::ClearColor:
		{
			ClearColor* clear = ConsumeClearColorDef(t,ps);
			const char* nameId = ConsumeIdentifier(t);
			CheckPassName(nameId);
			ParserAssert(ps.passMap.count(nameId) == 0, "Pass %s already defined", 
				nameId);
			Pass pass;
			pass.Name = AddStringToDescriptionData(nameId, rd);
			pass.Type = PassType::ClearColor;
			pass.ClearColor = clear;
			ps.passMap[nameId] = pass;
			break;
		}
		case Keyword::ClearDepth:
		{
			ClearDepth* clear = ConsumeClearDepthDef(t,ps);
			const char* nameId = ConsumeIdentifier(t);
			CheckPassName(nameId);
			ParserAssert(ps.passMap.count(nameId) == 0, "Pass %s already defined", 
				nameId);
			Pass pass;
			pass.Name = AddStringToDescriptionData(nameId, rd);
			pass.Type = PassType::ClearDepth;
			pass.ClearDepth = clear;
			ps.passMap[nameId] = pass;
			break;
		}
		case Keyword::ClearStencil:
		{
			ClearStencil* clear = ConsumeClearStencilDef(t,ps);
			const char* nameId = ConsumeIdentifier(t);
			CheckPassName(nameId);
			ParserAssert(ps.passMap.count(nameId) == 0, "Pass %s already defined", 
				nameId);
			Pass pass;
			pass.Name = AddStringToDescriptionData(nameId, rd);
			pass.Type = PassType::ClearStencil;
			pass.ClearStencil = clear;
			ps.passMap[nameId] = pass;
			break;
		}
		case Keyword::Passes:
		{
			ConsumeToken(TokenType::LBrace, t);

			while (true)
			{
				Pass pass = ConsumePassRefOrDef(t, ps);
				rd->Passes.push_back(pass);

				if (TryConsumeToken(TokenType::RBrace, t))
					break;
				else 
					ConsumeToken(TokenType::Comma, t);
			}
			break;
		}
		case Keyword::Tuneable:
		{
			ConsumeTuneable(t,ps);
			break;
		}
		case Keyword::Constant:
		{
			ConsumeConstant(t,ps);
			break;
		}
		default:
			ParserError("Unexpected structure: %s", id);
		}
	}
}


RenderDescription* ParseBuffer(
	const char* buffer,
	u32 bufferSize,
	const char* workingDir,
	ErrorState* es)
{
	es->Success = true;

	TokenizerState ts;
	TokenizerStateInit(ts);
	try {
		Tokenize(buffer, buffer+bufferSize, ts);
	}
	catch (ErrorInfo pe)
	{
		es->Success = false;
		es->Info = pe;
	}

	ParseState ps;
	ps.rd = new RenderDescription();
	ps.workingDirectory = workingDir;
	ParseStateInit(&ps);

	GPS = &ps;

	if (es->Success)
	{
		ps.t = { &ts.tokens[0], &ts.tokens[0] + ts.tokens.size() };
		try {
			ParseMain();
		}
		catch (ErrorInfo pe)
		{
			es->Success = false;
			es->Info = pe;
		}
	}

	GPS = nullptr;

	if (es->Success == false)
	{
		ReleaseData(ps.rd);
		return nullptr;
	}
	else
	{
		Assert(ps.t.next == ps.t.end, "Didn't consume full buffer.");
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
	for (ClearColor* clear : data->ClearColors)
		delete clear;
	for (ClearDepth* clear : data->ClearDepths)
		delete clear;
	for (ClearStencil* clear : data->ClearStencils)
		delete clear;
	for (Resolve* resolve : data->Resolves)
		delete resolve;
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
	for (View* v : data->Views)
		delete v;
	for (RasterizerState* rs : data->RasterizerStates)
		delete rs;
	for (DepthStencilState* dss : data->DepthStencilStates)
		delete dss;
	for (Viewport* vp : data->Viewports)
		delete vp;
	for (ObjImport* obj : data->Objs)
		delete obj;
	for (Constant* c : data->Constants)
		delete c;
	for (Tuneable* t : data->Tuneables)
		delete t;
	for (void* mem : data->Mems)
		free(mem);
	for (ast::Node* ast : data->Asts)
		delete ast;
	delete data;
}

#undef ParserAssert
#undef StructEntryDef

} // namespace rlf
