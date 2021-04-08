

namespace rlfparse
{


enum Token {
	TOKEN_INVALID,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_COMMA,
	TOKEN_EQUALS,
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

	if (*b.next == '{')
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
	// else if (isdigit(*b.next)) {
	// 	++b.next;
	// 	while(b.next < b.end && isdigit(*b.next))
	// 		++b.next;
	// 	retval = TOKEN_INTEGER_LITERAL;
	// }
	else if (islower(*b.next) || isupper(*b.next))
	{
		do {
			++b.next;
		} while (b.next < b.end && (islower(*b.next) || isupper(*b.next)));
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
	BufferIter b = *pb;
	b = SkipWhitespace(b);
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

// int GetIntegerLiteral(
// 	char* buffer_start,
// 	char* buffer_end,
// 	char** out_buffer_read)
// {
// 	char* buffer_next = buffer_start;
// 	int len = 0;
// 	while (buffer_next < buffer_end && isdigit(*buffer_next)) {
// 		++buffer_next;
// 		++len;
// 	}
// 	buffer_next = buffer_start;
// 	int val = 0;
// 	int base = pow(10, len-1);
// 	while (buffer_next < buffer_end && isdigit(*buffer_next)) {
// 		val += (*buffer_next - '0') * base;
// 		base /= 10;
// 		++buffer_next;
// 	}
// 	*out_buffer_read = buffer_next;
// 	return val;
// }

std::string IdentifierToString(BufferString id)
{
	return std::string(id.base, id.len);
}

const char* AddStringToDescriptionData(BufferString str, RenderDescription* rd)
{
	auto pair = rd->Strings.insert(std::string(str.base, str.len));
	return pair.first->c_str();
}



RenderDescription* ParseBuffer(
	char* buffer,
	int buffer_len)
{
	RenderDescription* rd = new RenderDescription();
	std::unordered_map<BufferString, DispatchCompute*, BufferStringHash> dcMap;

	BufferIter b = { buffer, buffer + buffer_len };
	while (b.next != b.end)
	{
		BufferString structureId;
		b = ConsumeIdentifier(b, &structureId);

		if (CompareIdentifier(structureId, "DispatchCompute"))
		{
			BufferString nameId;
			b = ConsumeIdentifier(b, &nameId);
			std::string name = IdentifierToString(nameId);
			Assert(dcMap.find(nameId) == dcMap.end(), "%s already defined", name.c_str());

			DispatchCompute* dc = new DispatchCompute();

			b = ConsumeToken(TOKEN_LBRACE, b);

			while (true)
			{
				BufferString fieldId;
				b = ConsumeIdentifier(b, &fieldId);
				std::string field = IdentifierToString(fieldId);
				if (CompareIdentifier(fieldId, "ShaderPath"))
				{
					b = ConsumeToken(TOKEN_EQUALS, b);
					BufferString value;
					b = ConsumeString(b, &value);
					dc->ShaderPath = AddStringToDescriptionData(value, rd);
				}
				else
				{
					Assert(false, "unexpected field %s", field.c_str());
				}

				if (!TryConsumeToken(TOKEN_COMMA, &b))
				{
					break;
				}
			}

			b = ConsumeToken(TOKEN_RBRACE, b);

			rd->Dispatches.insert(dc);
			dcMap[nameId] = dc;
		}
		else if (CompareIdentifier(structureId, "Passes"))
		{
			b = ConsumeToken(TOKEN_LBRACE, b);

			while (true)
			{
				BufferString pass;
				b = ConsumeIdentifier(b, &pass);
				std::string name = IdentifierToString(pass);
				auto iter = dcMap.find(pass);
				Assert(iter != dcMap.end(), "couldn't find pass %s", name.c_str());
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

	for (DispatchCompute* dc : data->Dispatches)
	{
		delete dc;
	}

	delete data;
}

}// namespace rlparse