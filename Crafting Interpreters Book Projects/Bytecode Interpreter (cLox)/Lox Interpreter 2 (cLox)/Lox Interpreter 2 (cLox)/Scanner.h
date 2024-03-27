// This file contains code for the scanner.
//

#pragma once

// #ifndef cLox_Scanner_h
//  #define cLox_Scanner_h

// cLox includes.




/// <summary>
/// An enumeration of token types.
/// </summary>
/// <remarks>
/// Unlike in jLox, here in the cLox interpreter project this enumeration prefixes
/// the names of all of its values with "TOKEN_". The book says this is because C puts
/// enum names in the top level namespace. 
/// 
/// The new ERROR token type is for returning errors to the compiler. Another difference
/// between cLox and jLox, is that here in cLox, the scanner only scans a token
/// when the compiler needs one. In contrast, the jLox scanner just scanned the entire
/// Lox script before passing it to the interpreter.
/// </remarks>
enum TokenType
{
	// Single-character tokens.
	TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
	TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
	TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

	// One or two character tokens.
	TOKEN_BANG, TOKEN_BANG_EQUAL,  // Not and Not Equal
	TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER, TOKEN_GREATER_EQUAL,
	TOKEN_LESS, TOKEN_LESS_EQUAL,

	// Literals.
	TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

	// Keywords.
	TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
	TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
	TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
	TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

	TOKEN_ERROR, TOKEN_EOF
};




struct Token
{
	TokenType Type; // The type of this token.
	const char* Start; // The starting character of this token.
	int Length; // The length of this token.
	int Line; // The source code line number this token is on.
};


void InitScanner(const char* source);
Token ScanToken();

// #endif
