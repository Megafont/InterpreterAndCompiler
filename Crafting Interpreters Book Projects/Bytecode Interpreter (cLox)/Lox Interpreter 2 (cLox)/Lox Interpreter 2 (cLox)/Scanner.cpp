#include <stdio.h>
#include <string.h>

// cLox includes.
#include "Common.h"
#include "Scanner.h"




struct Scanner
{
	const char* Start; // The start of the lexeme being scanned.
	const char* Current; // The current character being looked at.
	int line; // The line number the current lexeme is on in the source code.
};




Scanner scanner;




void InitScanner(const char* source)
{
	scanner.Start = source;
	scanner.Current = source;
	scanner.line = 1;
}


static char Advance()
{
	scanner.Current++;
	return scanner.Current[-1];
}


static bool IsAtEnd()
{
	return *scanner.Current == '\0';
}


/// <summary>
/// Gets the current character without consuming it.
/// </summary>
/// <returns>The current character.</returns>
static char Peek()
{
	return *scanner.Current;
}


/// <summary>
/// Gets the character after the current one without consuming it.
/// </summary>
/// <returns>The character after the current one.</returns>
static char PeekNext()
{
	if (IsAtEnd())
		return '\0';

	return scanner.Current[1];
}


static bool IsAlpha(char c)
{
	return (c >= 'a' && c <= 'z') ||
		   (c >= 'A' && c <= 'Z') ||
			c == '_';
}


static bool IsDigit(char c)
{
	return c >= '0' && c <= '9';
}


static Token MakeToken(TokenType type)
{
	Token token;

	token.Type = type;
	token.Start = scanner.Start;
	token.Length = (int)(scanner.Current - scanner.Start);
	token.Line = scanner.line;

	return token;
}


static Token Number()
{
	while (IsDigit(Peek()))
		Advance();

	// Look for a fractional part of the number.
	if (Peek() == '.' && IsDigit(PeekNext()))
	{
		// Consume the ".".
		Advance();

		while (IsDigit(Peek()))
			Advance();
	}

	return MakeToken(TOKEN_NUMBER);
}


static bool Match(char expected)
{
	if (IsAtEnd())
		return false;

	if (*scanner.Current != expected)
		return false;

	scanner.Current++;
	return true;
}


static Token ErrorToken(const char* message)
{
	Token token;

	token.Type = TOKEN_ERROR;
	token.Start = message;
	token.Length = (int)strlen(message);
	token.Line = scanner.line;

	return token;
}


static void SkipWhiteSpace()
{
	for (;;)
	{
		char c = Peek();

		switch (c)
		{
			case ' ':
			case '\r':
			case '\t':
				Advance();
				break;

			case '\n':
				scanner.line++;
				Advance();
				break;

			// Skip comments too.
			case '/':
				// Check that the second character is also a '/'.
				// A double "//" denotes the start of a comment in Lox code.
				if (PeekNext() == '/')
				{
					// A comment goes until the end of a line.
					while (Peek() != '\n' && !IsAtEnd())
						Advance();					
				}
				else
				{
					return;
				}

				break;

			default:
				return;

		} // end switch

	} // end for
}


static Token String()
{
	while (Peek() != '"' && !IsAtEnd())
	{
		// Watch for newline characters since Lox supports multi-line strings.
		if (Peek() == '\n')
			scanner.line++;

		Advance();
	} // end while

	if (IsAtEnd())
		return ErrorToken("Unterminated string.");

	// The closing quotation mark of the string.
	Advance();
	return MakeToken(TOKEN_STRING);
}


static TokenType CheckKeyword(int start, int length, const char* rest, TokenType type)
{
	if (scanner.Current - scanner.Start == start + length &&
		memcmp(scanner.Start + start, rest, length) == 0)
	{
		return type;
	}

	return TOKEN_IDENTIFIER;
}


/// <summary>
/// This function detects whether a token is a Lox keyword or some other identifier, like a variable name.
/// </summary>
/// <returns>The detected identifier.</returns>
/// <remarks>
/// This function checks that the first letter matches that of a Lox keyword.
/// If so, then it calls CheckKeyword() to see if the rest of the token matches
/// a certain keyword. The 'a' case checks for the keyword "And". So the string "nd"
/// 
/// is passed into CheckKeyword() since that is the rest of the keword after the letter A.
/// This code does the minimum amount of work needed. As soon as it hits a character that doesn't
/// match the keyword it suspects the token to be, it bails out since it now knows the token
/// can't be that keyword.
/// </remarks>
static TokenType IndentifierType()
{
	switch (scanner.Start[0]) {
		case 'a':
			return CheckKeyword(1, 2, "nd", TOKEN_AND);
		case 'c':
			return CheckKeyword(1, 4, "lass", TOKEN_CLASS);
		case 'e':
			return CheckKeyword(1, 3, "lse", TOKEN_ELSE);
		case 'f':
			if (scanner.Current - scanner.Start > 1) // Check that there is a second character.
			{
				switch (scanner.Start[1]) {
					case 'a': return CheckKeyword(2, 3, "lse", TOKEN_FALSE);
					case 'o': return CheckKeyword(2, 1, "r", TOKEN_FOR);
					case 'u': return CheckKeyword(2, 1, "n", TOKEN_FUN);
				}
			}
			break;
		case 'i':
			return CheckKeyword(1, 1, "f", TOKEN_IF);
		case 'n':
			return CheckKeyword(1, 2, "il", TOKEN_NIL);
		case 'o':
			return CheckKeyword(1, 1, "r", TOKEN_OR);
		case 'p':
			return CheckKeyword(1, 4, "rint", TOKEN_PRINT);
		case 'r':
			return CheckKeyword(1, 5, "eturn", TOKEN_RETURN);
		case 's':
			return CheckKeyword(1, 4, "uper", TOKEN_SUPER);
		case 't':
			if (scanner.Current - scanner.Start > 1) // Check that there is a second character.
			{
				switch (scanner.Start[1]) {
					case 'h': return CheckKeyword(2, 2, "is", TOKEN_THIS);
					case 'r': return CheckKeyword(2, 2, "ue", TOKEN_TRUE);
				}
			}
			break;
		case 'v':
			return CheckKeyword(1, 2, "ar", TOKEN_VAR);
		case 'w':
			return CheckKeyword(1, 4, "hile", TOKEN_WHILE);
	} // end switch

	return TOKEN_IDENTIFIER;
}


static Token Identifier()
{
	while (IsAlpha(Peek()) || IsDigit(Peek()))
		Advance();

	return MakeToken(IndentifierType());
}


Token ScanToken()
{
	SkipWhiteSpace();
	scanner.Start = scanner.Current;

	if (IsAtEnd())
		return MakeToken(TOKEN_EOF);


	char c = Advance();
	if (IsAlpha(c))
		return Identifier();
	if (IsDigit(c))
		return Number();

	switch (c)
	{
		// Single-character tokens.
		case '(':
			return MakeToken(TOKEN_LEFT_PAREN);
		case ')':
			return MakeToken(TOKEN_RIGHT_PAREN);
		case '{':
			return MakeToken(TOKEN_LEFT_BRACE);
		case '}':
			return MakeToken(TOKEN_RIGHT_BRACE);
		case ';':
			return MakeToken(TOKEN_SEMICOLON);
		case ',':
			return MakeToken(TOKEN_COMMA);
		case '.':
			return MakeToken(TOKEN_DOT);
		case '-':
			return MakeToken(TOKEN_MINUS);
		case '+':
			return MakeToken(TOKEN_PLUS);
		case '/':
			return MakeToken(TOKEN_SLASH);
		case '*':
			return MakeToken(TOKEN_STAR);


			// One or two character tokens.
		case '!':
			return MakeToken(Match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
		case '=':
			return MakeToken(Match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
		case '<':
			return MakeToken(Match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
		case '>':
			return MakeToken(Match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);


			// Literals.
		case '"':
			return String();

	} // end switch


	return ErrorToken("Unexpected character.");
}

