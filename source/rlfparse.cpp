

namespace rlf
{


enum Token {
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

struct BufferString {
	const char* base;
	size_t len;

	bool operator==(const BufferString& other) const
	{
		if (len != other.len)
			return false;
		for (size_t i = 0 ; i < len ; ++i)
		{
			if (base[i] != other.base[i])
				return false;
		}
		return true;
	}
};

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

struct BufferIter {
	char* next;
	char* end;
};

bool CompareIdentifier(BufferString id, const char* string) {
	if (id.len != strlen(string))
		return false;
	for (int i = 0 ; i < id.len ; ++i)
		if (id.base[i] != string[i])
			return false;
	return true;
}

BufferIter SkipWhitespace(
	BufferIter b)
{
	while (b.next < b.end && isspace(*b.next)) {
		++b.next;
	}
	return b;
}

BufferIter PeekNextToken(
	BufferIter b,
	Token* outTok)
{
	Assert(b.next != b.end, "unexpected end-of-buffer.");

	if (*b.next == '(')
	{
		++b.next;
		*outTok = TOKEN_LPAREN;
	}
	else if (*b.next == ')')
	{
		++b.next;
		*outTok = TOKEN_RPAREN;
	}
	else if (*b.next == '{')
	{
		++b.next;
		*outTok = TOKEN_LBRACE;
	}
	else if (*b.next == '}')
	{
		++b.next;
		*outTok = TOKEN_RBRACE;
	}
	else if (*b.next == ',')
	{
		++b.next;
		*outTok = TOKEN_COMMA;
	}
	else if (*b.next == '=')
	{
		++b.next;
		*outTok = TOKEN_EQUALS;
	}
	else if (*b.next == '-')
	{
		++b.next;
		*outTok = TOKEN_MINUS;
	}
	else if (isdigit(*b.next)) {
		++b.next;
		while(b.next < b.end && isdigit(*b.next))
			++b.next;
		*outTok = TOKEN_INTEGER_LITERAL;
	}
	else if (isalpha(*b.next))
	{
		// identifiers have to start with a letter, but can contain numbers
		do {
			++b.next;
		} while (b.next < b.end && (isalpha(*b.next) || isdigit(*b.next)));
		*outTok = TOKEN_IDENTIFIER;
	}
	else if (*b.next == '"')
	{
		do {
			++b.next;
		} while (b.next < b.end && *b.next != '"');
		++b.next; // pass the closing quotes
		*outTok = TOKEN_STRING;
	}
	else {
		*outTok = TOKEN_INVALID;
		Assert(false, "unexpected character when reading token: %c",
			*b.next);
	}

	return b;
}

BufferIter ConsumeToken(
	Token tok,
	BufferIter b)
{
	b = SkipWhitespace(b);
	Token foundTok;
	b = PeekNextToken(b, &foundTok);
	Assert(foundTok == tok, "unexpected token, expected %d found %d",
		tok, foundTok);
	return b;
}

bool TryConsumeToken(
	Token tok,
	BufferIter* pb)
{
	BufferIter b = SkipWhitespace(*pb);
	Token foundTok;
	b = PeekNextToken(b, &foundTok);
	bool found = (foundTok == tok);
	if (found)
	{
		*pb = b;
	}
	return found;
}

BufferIter ConsumeIdentifier(
	BufferIter b,
	BufferString* id)
{
	BufferIter start = SkipWhitespace(b);
	Token tok;
	BufferIter tokRead = PeekNextToken(start, &tok);
	Assert(tok == TOKEN_IDENTIFIER, "unexpected token (wanted identifier)");

	id->base = start.next;
	id->len = tokRead.next - start.next;

	return tokRead;
}

BufferIter ConsumeString(
	BufferIter b,
	BufferString* str)
{
	BufferIter start = SkipWhitespace(b);
	Token tok;
	BufferIter tokRead = PeekNextToken(start, &tok);
	Assert(tok == TOKEN_STRING, "unexpected token (wanted string)");

	str->base = start.next + 1;
	str->len = tokRead.next - start.next - 2;

	return tokRead;	
}

BufferIter ConsumeBool(
	BufferIter b, 
	bool* outBool)
{
	BufferString value;
	b = ConsumeIdentifier(b, &value);	
	if (CompareIdentifier(value, "true"))
		*outBool = true;
	else if (CompareIdentifier(value, "false"))
		*outBool = false;
	else
	{
		Assert(false, "expected bool (true/false), got: %.*s", value.len, value.base);
	}
	return b;
}

BufferIter ConsumeIntLiteral(
	BufferIter b,
	i32* outVal)
{
	b = SkipWhitespace(b);
	bool negative = false;
	if (TryConsumeToken(TOKEN_MINUS, &b))
	{
		negative = true;
		b = SkipWhitespace(b);
	}
	Token tok;
	BufferIter tokRead = PeekNextToken(b, &tok);
	Assert(tok == TOKEN_INTEGER_LITERAL, "unexpected token (wanted string)");

	i32 val = 0;
	do 
	{
		val *= 10;
		val += (*b.next - '0');
		++b.next;
	} while (b.next < tokRead.next);

	*outVal = negative ? -val : val;

	return tokRead;
}

BufferIter ConsumeUintLiteral(
	BufferIter b,
	u32* outVal)
{
	b = SkipWhitespace(b);
	if (TryConsumeToken(TOKEN_MINUS, &b))
	{
		Assert(false, "Unsigned int expected, TOKEN_MINUS invalid here");
	}
	Token tok;
	BufferIter tokRead = PeekNextToken(b, &tok);
	Assert(tok == TOKEN_INTEGER_LITERAL, "unexpected token (wanted string)");

	u32 val = 0;
	do 
	{
		val *= 10;
		val += (*b.next - '0');
		++b.next;
	} while (b.next < tokRead.next);

	*outVal = val;

	return tokRead;
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
		BufferString structureId;
		b = ConsumeIdentifier(b, &structureId);

		if (CompareIdentifier(structureId, "ComputeShader"))
		{
			BufferString nameId;
			b = ConsumeIdentifier(b, &nameId);
			Assert(ps.shaderMap.find(nameId) == ps.shaderMap.end(), "%.*s already defined", 
				nameId.len, nameId.base);

			ComputeShader* cs = new ComputeShader();

			b = ConsumeToken(TOKEN_LBRACE, b);

			while (true)
			{
				BufferString fieldId;
				b = ConsumeIdentifier(b, &fieldId);
				if (CompareIdentifier(fieldId, "ShaderPath"))
				{
					b = ConsumeToken(TOKEN_EQUALS, b);
					BufferString value;
					b = ConsumeString(b, &value);
					cs->ShaderPath = AddStringToDescriptionData(value, rd);
				}
				else if (CompareIdentifier(fieldId, "EntryPoint"))
				{
					b = ConsumeToken(TOKEN_EQUALS, b);
					BufferString value;
					b = ConsumeString(b, &value);
					cs->EntryPoint = AddStringToDescriptionData(value, rd);
				}
				else
				{
					Assert(false, "unexpected field %.*s", fieldId.len, fieldId.base);
				}

				if (!TryConsumeToken(TOKEN_COMMA, &b))
				{
					break;
				}
			}

			b = ConsumeToken(TOKEN_RBRACE, b);

			rd->Shaders.push_back(cs);
			ps.shaderMap[nameId] = cs;
		}
		else if (CompareIdentifier(structureId, "Dispatch"))
		{
			BufferString nameId;
			b = ConsumeIdentifier(b, &nameId);
			Assert(ps.dcMap.find(nameId) == ps.dcMap.end(), "%.*s already defined", 
				nameId.len, nameId.base);

			Dispatch* dc = new Dispatch();

			b = ConsumeToken(TOKEN_LBRACE, b);

			while (true)
			{
				BufferString fieldId;
				b = ConsumeIdentifier(b, &fieldId);
				if (CompareIdentifier(fieldId, "Shader"))
				{
					b = ConsumeToken(TOKEN_EQUALS, b);
					BufferString shader;
					b = ConsumeIdentifier(b, &shader);
					auto iter = ps.shaderMap.find(shader);
					Assert(iter != ps.shaderMap.end(), "couldn't find shader %.*s", 
						shader.len, shader.base);
					dc->Shader = iter->second;
				}
				else if (CompareIdentifier(fieldId, "ThreadPerPixel"))
				{
					b = ConsumeToken(TOKEN_EQUALS, b);
					b = ConsumeBool(b, &dc->ThreadPerPixel);
				}
				else if (CompareIdentifier(fieldId, "Groups"))
				{
					b = ConsumeToken(TOKEN_EQUALS, b);
					b = ConsumeToken(TOKEN_LBRACE, b);
					b = ConsumeUintLiteral(b, &dc->Groups.x);
					b = ConsumeToken(TOKEN_COMMA, b);
					b = ConsumeUintLiteral(b, &dc->Groups.y);
					b = ConsumeToken(TOKEN_COMMA, b);
					b = ConsumeUintLiteral(b, &dc->Groups.z);
					b = ConsumeToken(TOKEN_RBRACE, b);
				}
				else
				{
					Assert(false, "unexpected field %.*s", fieldId.len, fieldId.base);
				}

				if (!TryConsumeToken(TOKEN_COMMA, &b))
				{
					break;
				}
			}

			b = ConsumeToken(TOKEN_RBRACE, b);

			rd->Dispatches.push_back(dc);
			ps.dcMap[nameId] = dc;
		}
		else if (CompareIdentifier(structureId, "Passes"))
		{
			b = ConsumeToken(TOKEN_LBRACE, b);

			while (true)
			{
				BufferString pass;
				b = ConsumeIdentifier(b, &pass);
				auto iter = ps.dcMap.find(pass);
				Assert(iter != ps.dcMap.end(), "couldn't find pass %.*s", 
					pass.len, pass.base);
				rd->Passes.push_back(iter->second);

				if (!TryConsumeToken(TOKEN_COMMA, &b))
				{
					break;
				}
			}

			b = ConsumeToken(TOKEN_RBRACE, b);
		}
		else
		{
			Assert(false, "Unexpected identifier.");
		}

		// Skip trailing whitespace so we don't miss the exit condition for this loop. 
		b = SkipWhitespace(b);
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