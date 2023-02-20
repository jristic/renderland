
namespace rlf {
namespace shader {


#define TOKEN_TUPLE \
	TOKEN_ENTRY(Invalid) \
	TOKEN_ENTRY(LParen) \
	TOKEN_ENTRY(RParen) \
	TOKEN_ENTRY(LBrace) \
	TOKEN_ENTRY(RBrace) \
	TOKEN_ENTRY(LBracket) \
	TOKEN_ENTRY(RBracket) \
	TOKEN_ENTRY(LessThan) \
	TOKEN_ENTRY(GreaterThan) \
	TOKEN_ENTRY(Comma) \
	TOKEN_ENTRY(Equals) \
	TOKEN_ENTRY(Plus) \
	TOKEN_ENTRY(Minus) \
	TOKEN_ENTRY(Colon) \
	TOKEN_ENTRY(Semicolon) \
	TOKEN_ENTRY(At) \
	TOKEN_ENTRY(Period) \
	TOKEN_ENTRY(ForwardSlash) \
	TOKEN_ENTRY(Asterisk) \
	TOKEN_ENTRY(HashMark) \
	TOKEN_ENTRY(Percent) \
	TOKEN_ENTRY(IntegerLiteral) \
	TOKEN_ENTRY(FloatLiteral) \
	TOKEN_ENTRY(Identifier) \
	TOKEN_ENTRY(String) \

#define TOKEN_ENTRY(name) name,
enum class TokenType
{
	TOKEN_TUPLE
};
#undef TOKEN_ENTRY

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

#define TOKEN_ENTRY(name) #name,
const char* TokenNames[] =
{
	TOKEN_TUPLE
};
#undef TOKEN_ENTRY

#undef TOKEN_TUPLE


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
	fcLUT['<'] = TokenType::LessThan;
	fcLUT['>'] = TokenType::GreaterThan;
	fcLUT[','] = TokenType::Comma;
	fcLUT['='] = TokenType::Equals;
	fcLUT['+'] = TokenType::Plus;
	fcLUT['-'] = TokenType::Minus;
	fcLUT[':'] = TokenType::Colon;
	fcLUT[';'] = TokenType::Semicolon;
	fcLUT['@'] = TokenType::At;
	fcLUT['.'] = TokenType::Period;
	fcLUT['/'] = TokenType::ForwardSlash;
	fcLUT['*'] = TokenType::Asterisk;
	fcLUT['#'] = TokenType::HashMark;
	fcLUT['%'] = TokenType::Percent;
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
				Assert(next[0] == '*' && next[1] == '/',
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
		case TokenType::LessThan:
		case TokenType::GreaterThan:
		case TokenType::Comma:
		case TokenType::Equals:
		case TokenType::Plus:
		case TokenType::Minus:
		case TokenType::Colon:
		case TokenType::Semicolon:
		case TokenType::At:
		case TokenType::Period:
		case TokenType::ForwardSlash:
		case TokenType::Asterisk:
		case TokenType::HashMark:
		case TokenType::Percent:
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

#define KEYWORD_TUPLE \
	KEYWORD_ENTRY(Struct, 	"struct") \
	KEYWORD_ENTRY(Float, 	"float") \
	KEYWORD_ENTRY(Float2, 	"float2") \
	KEYWORD_ENTRY(Float3, 	"float3") \
	KEYWORD_ENTRY(Float4, 	"float4") \
	KEYWORD_ENTRY(Int, 		"int") \
	KEYWORD_ENTRY(Int2, 	"int2") \
	KEYWORD_ENTRY(Int3, 	"int3") \
	KEYWORD_ENTRY(Int4, 	"int4") \
	KEYWORD_ENTRY(Uint, 	"uint") \
	KEYWORD_ENTRY(Uint2, 	"uint2") \
	KEYWORD_ENTRY(Uint3, 	"uint3") \
	KEYWORD_ENTRY(Uint4, 	"uint4") \
	KEYWORD_ENTRY(Bool, 	"bool") \
	KEYWORD_ENTRY(Bool2, 	"bool2") \
	KEYWORD_ENTRY(Bool3, 	"bool3") \
	KEYWORD_ENTRY(Bool4, 	"bool4") \

#define KEYWORD_ENTRY(name, str) name,
enum class Keyword
{
	Invalid,
	KEYWORD_TUPLE
	_Count
};
#undef KEYWORD_ENTRY

#define KEYWORD_ENTRY(name, str) str,
const char* KeywordString[] = 
{
	"<Invalid>",
	KEYWORD_TUPLE
};
#undef KEYWORD_ENTRY

#undef KEYWORD_TUPLE

u32 Hash(const char* str)
{
	unsigned long h = 5381;
	unsigned const char* us = (unsigned const char *) str;
	while(*us != '\0') {
		h = ((h << 5) + h) + *us;
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
	std::unordered_map<std::string, u32>* structSizes;
	std::unordered_map<u32, Keyword> keyMap;
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
	u32 hash = Hash(str);
	if (GPS->keyMap.count(hash) > 0)
		return GPS->keyMap[hash];
	else
		return Keyword::Invalid;
}

void ParseStateInit(ParseState* ps)
{
	for (u32 i = 0 ; i < (u32)Keyword::_Count ; ++i)
	{
		Keyword key = (Keyword)i;
		const char* str = KeywordString[i];
		u32 hash = Hash(str);
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

bool TryConsumeKeyword(
	Keyword key,
	TokenIter& t)
{
	TokenType tok = PeekNextToken(t);
	if (tok == TokenType::Identifier)
	{
		const char* str = t.next->String;
		if (LookupKeyword(str) == key)
		{
			++t.next;
			return true;
		}
	}
	return false;
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

u32 ConsumeType(TokenIter& t, ParseState& ps)
{
	const char* name = ConsumeIdentifier(t);
	Keyword key = LookupKeyword(name);
	switch (key)
	{
	case Keyword::Float:
		return 4;
		break;
	case Keyword::Float2:
		return 8;
		break;
	case Keyword::Float3:
		return 12;
		break;
	case Keyword::Float4:
		return 16;
		break;
	case Keyword::Int:
		return 4;
		break;
	case Keyword::Int2:
		return 8;
		break;
	case Keyword::Int3:
		return 12;
		break;
	case Keyword::Int4:
		return 16;
		break;
	case Keyword::Uint:
		return 4;
		break;
	case Keyword::Uint2:
		return 8;
		break;
	case Keyword::Uint3:
		return 12;
		break;
	case Keyword::Uint4:
		return 16;
		break;
	case Keyword::Bool:
		return 4;
		break;
	case Keyword::Bool2:
		return 8;
		break;
	case Keyword::Bool3:
		return 12;
		break;
	case Keyword::Bool4:
		return 16;
		break;
	default:
		break;
	}
	auto search = ps.structSizes->find(name);
	InitAssert(search != ps.structSizes->end(), "Size for type %s is unknown",
		name);
	return search->second;
}

void ParseMain(ParseState& ps)
{
	TokenIter& t = ps.t;

	while (t.next != t.end)
	{
		if (TryConsumeKeyword(Keyword::Struct, t))
		{
			const char* structName = ConsumeIdentifier(t);
			ConsumeToken(TokenType::LBrace, t);
			u32 structSize = 0;
			while (true)
			{
				if (TryConsumeToken(TokenType::RBrace, t))
					break;
				u32 typeSize = ConsumeType(t, ps);
				u32 count = 1;
				/*const char* field = */ ConsumeIdentifier(t);
				while (TryConsumeToken(TokenType::Comma, t))
				{
					/* const char* otherField = */ ConsumeIdentifier(t);
					++count;
				}
				if (TryConsumeToken(TokenType::Colon, t))
					/*const char* semantic = */ ConsumeIdentifier(t);
				ConsumeToken(TokenType::Semicolon, t);

				structSize += count*typeSize;
			}
			(*ps.structSizes)[structName] = structSize;
		}
		else
			++t.next;
	}
}

void ParseBuffer(
	const char* buffer,
	u32 bufferSize,
	std::unordered_map<std::string, u32>& structSizes,
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
	ps.structSizes = &structSizes;
	ParseStateInit(&ps);

	GPS = &ps;

	if (es->Success)
	{
		ps.t = { &ts.tokens[0], &ts.tokens[0] + ts.tokens.size() };
		try {
			ParseMain(ps);
		}
		catch (ErrorInfo pe)
		{
			es->Success = false;
			es->Info = pe;
		}
	}

	GPS = nullptr;

	Assert(!es->Success || ps.t.next == ps.t.end, "Didn't consume full buffer.");
}


} // namespace shader
} // namespace rlf
