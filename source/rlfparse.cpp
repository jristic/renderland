
namespace rlf
{

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
	Assert(b.next != b.end, "unexpected end-of-buffer.");

	switch (*b.next)
	{
	case '(':
		++b.next;
		return TOKEN_LPAREN;
	case ')':
		++b.next;
		return TOKEN_RPAREN;
	case '{':
		++b.next;
		return TOKEN_LBRACE;
	case '}':
		++b.next;
		return TOKEN_RBRACE;
	case ',':
		++b.next;
		return TOKEN_COMMA;
	case '=':
		++b.next;
		return TOKEN_EQUALS;
	case '-':
		++b.next;
		return TOKEN_MINUS;
	case '"':
		do {
			++b.next;
		} while (b.next < b.end && *b.next != '"');
		++b.next; // pass the closing quotes
		return TOKEN_STRING;
	case '0': case '1': case '2': case '3': case '4': 
	case '5': case '6': case '7': case '8': case '9':
		++b.next;
		while(b.next < b.end && isdigit(*b.next))
			++b.next;
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
		do {
			++b.next;
		} while (b.next < b.end && (isalpha(*b.next) || isdigit(*b.next)));
		return TOKEN_IDENTIFIER;
	default:
		Assert(false, "unexpected character when reading token: %c",
			*b.next);
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
	Assert(foundTok == tok, "unexpected token, expected %d found %d",
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
	Assert(tok == TOKEN_IDENTIFIER, "unexpected token (wanted identifier)");

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
	Assert(tok == TOKEN_STRING, "unexpected token (wanted string)");

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

	Assert(false, "expected bool (true/false), got: %.*s", value.len, value.base);
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
	Assert(tok == TOKEN_INTEGER_LITERAL, "unexpected token (wanted string)");

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
		Assert(false, "Unsigned int expected, TOKEN_MINUS invalid here");
	}
	BufferIter nb = b;
	Token tok = PeekNextToken(b);
	Assert(tok == TOKEN_INTEGER_LITERAL, "unexpected token (wanted string)");

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

struct ParseState 
{
	RenderDescription* rd;
	std::unordered_map<BufferString, Dispatch*, BufferStringHash> dcMap;
	std::unordered_map<BufferString, ComputeShader*, BufferStringHash> shaderMap;
};


RenderDescription* ParseBuffer(
	char* buffer,
	int buffer_len)
{
	RenderDescription* rd = new RenderDescription();
	ParseState ps;
	ps.rd = rd;

	BufferIter b = { buffer, buffer + buffer_len };
	while (b.next != b.end)
	{
		BufferString structureId = ConsumeIdentifier(b);

		if (structureId == "ComputeShader")
		{
			BufferString nameId = ConsumeIdentifier(b);
			Assert(ps.shaderMap.find(nameId) == ps.shaderMap.end(), "%.*s already defined", 
				nameId.len, nameId.base);

			ComputeShader* cs = new ComputeShader();

			ConsumeToken(TOKEN_LBRACE, b);

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
					Assert(false, "unexpected field %.*s", fieldId.len, fieldId.base);
				}

				if (!TryConsumeToken(TOKEN_COMMA, b))
				{
					break;
				}
			}

			ConsumeToken(TOKEN_RBRACE, b);

			rd->Shaders.push_back(cs);
			ps.shaderMap[nameId] = cs;
		}
		else if (structureId == "Dispatch")
		{
			BufferString nameId = ConsumeIdentifier(b);
			Assert(ps.dcMap.find(nameId) == ps.dcMap.end(), "%.*s already defined", 
				nameId.len, nameId.base);

			Dispatch* dc = new Dispatch();

			ConsumeToken(TOKEN_LBRACE, b);

			while (true)
			{
				BufferString fieldId = ConsumeIdentifier(b);
				if (fieldId == "Shader")
				{
					ConsumeToken(TOKEN_EQUALS, b);
					BufferString shader = ConsumeIdentifier(b);
					auto iter = ps.shaderMap.find(shader);
					Assert(iter != ps.shaderMap.end(), "couldn't find shader %.*s", 
						shader.len, shader.base);
					dc->Shader = iter->second;
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
					Assert(false, "unexpected field %.*s", fieldId.len, fieldId.base);
				}

				if (!TryConsumeToken(TOKEN_COMMA, b))
				{
					break;
				}
			}

			ConsumeToken(TOKEN_RBRACE, b);

			rd->Dispatches.push_back(dc);
			ps.dcMap[nameId] = dc;
		}
		else if (structureId == "Passes")
		{
			ConsumeToken(TOKEN_LBRACE, b);

			while (true)
			{
				BufferString pass = ConsumeIdentifier(b);
				auto iter = ps.dcMap.find(pass);
				Assert(iter != ps.dcMap.end(), "couldn't find pass %.*s", 
					pass.len, pass.base);
				rd->Passes.push_back(iter->second);

				if (!TryConsumeToken(TOKEN_COMMA, b))
				{
					break;
				}
			}

			ConsumeToken(TOKEN_RBRACE, b);
		}
		else
		{
			Assert(false, "Unexpected structure: %.*s", structureId.len, structureId.base);
		}

		// Skip trailing whitespace so we don't miss the exit condition for this loop. 
		SkipWhitespace(b);
	}
	return rd;
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

}// namespace rlparse