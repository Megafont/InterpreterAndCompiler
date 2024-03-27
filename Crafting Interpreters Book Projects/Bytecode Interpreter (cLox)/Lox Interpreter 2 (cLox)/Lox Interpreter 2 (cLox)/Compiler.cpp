#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string> // I added this because I used it in the EndCompiler() function. It is not the same as String.h.

// cLox includes.
#include "Common.h"
#include "Compiler.h"
#include "Memory.h"
#include "Scanner.h"

#ifdef DEBUG_PRINT_CODE
	#include "Debug.h"
#endif




struct Parser
{
	Token Current; // Current token.
	Token Previous; // Previous token.
	bool HadError; // A flag indicating whether a compile error has occurred.
	bool PanicMode; // If a compile error occurs, this flag gets set to indicate we're in "panic mode".
};


enum Precedence 
{
	PREC_NONE,
	PREC_ASSIGNMENT,  // =
	PREC_OR,          // or
	PREC_AND,         // and
	PREC_EQUALITY,    // == !=
	PREC_COMPARISON,  // < > <= >=
	PREC_TERM,        // + -
	PREC_FACTOR,      // * /
	PREC_UNARY,       // ! -
	PREC_CALL,        // . ()
	PREC_PRIMARY
};


// This function pointer type is called "ParseFn" in the book.
typedef void (*ParseFunction)(bool canAssign);


struct ParseRule
{
	ParseFunction Prefix; // Pointer to a prefix parsing function.
	ParseFunction Infix; // Pointer to an infix parsing function.
	Precedence Precedence; // The precedence to parse at.
};


struct Local
{
	Token Name; // The name of the local variable.
	int Depth; // The scope depth of this variable (0 = global, and so on).
	bool IsCaptured; // Is this variable captured by a closure? See chapter 25 in the book.
};


// This is needed for closures so a nested function can access variables in its
// parent function. See chapter 25 in the book.
struct UpValue
{
	uint8_t Index; // The stack index of the variable referenced by this closure.
	bool IsLocal;
};


typedef enum FunctionType
{
	TYPE_FUNCTION,
	TYPE_INITIALIZER, // A class initializer method (aka a constructor)
	TYPE_METHOD, // A class method
	TYPE_SCRIPT, // This type represents the automatically generated function that contains the main section of a Lox script that's not inside a function declaration.
};


struct Compiler
{
	Compiler* Enclosing; // The parent compiler of this one. For example, if the current compiler is compiling a function FuncX(), then its parent is the compiler that is compiling the code where FuncX's declaration appears.
	ObjFunction* Function; // Keeps track of the currently compiling function.
	FunctionType Type; // The type of the currently compiling function.

	Local Locals[UINT8_COUNT]; // Stores all local variables in existance at the current point in compilation.
	int LocalCount; // The number of locals currently in existance.
	UpValue UpValues[UINT8_COUNT]; // The list of UpValues this compiler has compiled. See chapter 25 in the book.
	int ScopeDepth; // How many scopes deep we currently are in the code.
};


struct ClassCompiler
{
	ClassCompiler* Enclosing;
	bool HasSuperClass;
};




Parser parser;
Compiler* current = NULL;
ClassCompiler* currentClass = NULL;




/// <summary>
/// Returns a pointer to the bytecode chunk that is currently being compiled.
/// </summary>
/// <returns>A pointer to the bytecode chunk being compiled.</returns>
static Chunk* CurrentChunk()
{
	return &current->Function->Chunk;
}


static void ErrorAt(Token* token, const char* message)
{
	// If we are in panic mode (meaning we already had a compile error), then suppress any further errors.
	if (parser.PanicMode)
		return;

	parser.PanicMode = true;


	fprintf(stderr, "COMPILE ERROR: [Line %d] Error", token->Line);

	if (token->Type == TOKEN_EOF)
	{
		fprintf(stderr, " at end of source code.");
	}
	else if (token->Type == TOKEN_ERROR)
	{
		// Nothing
	}
	else
	{ 
		fprintf(stderr, " at '%.*s'", token->Length, token->Start);
	}


	fprintf(stderr, ": %s\n", message);
	parser.HadError = true;
}


static void Error(const char* message)
{
	ErrorAt(&parser.Previous, message);
}


static void ErrorAtCurrent(const char* message)
{
	ErrorAt(&parser.Current, message);
}


/// <summary>
/// Advances to the next token. Calls ErrorAtCurrent() if the parser returns an error.
/// </summary>
static void Advance()
{
	parser.Previous = parser.Current;

	for (;;)
	{
		parser.Current = ScanToken();
		if (parser.Current.Type != TOKEN_ERROR)
			break;

		ErrorAtCurrent(parser.Current.Start);
	}
}


/// <summary>
/// This function is similar to Advance, except that it verifies that the token is
/// of an expected type. If not, it calls ErrorAtCurrent();
/// </summary>
/// <param name="type">The expected type of the token being looked at.</param>
/// <param name="message">The error message to report if the token is not the expected type.</param>
static void Consume(TokenType type, const char* message)
{
	if (parser.Current.Type == type)
	{
		Advance();
		return;
	}

	ErrorAtCurrent(message);
}


static bool Check(TokenType type)
{
	return parser.Current.Type == type;
}


static bool Match(TokenType type)
{
	if (!Check(type))
		return false;

	Advance();
	return true;
}


/// <summary>
/// Appends a new byte to the end of the chunk of bytecode the compiler is generating.
/// </summary>
/// <param name="byte">The byte to add.</param>
static void EmitByte(uint8_t byte)
{
	WriteChunk(CurrentChunk(), byte, parser.Previous.Line);
}


static void EmitBytes(uint8_t byte1, uint8_t byte2)
{
	EmitByte(byte1);
	EmitByte(byte2);
}


static void EmitLoop(int loopStart)
{
	EmitByte(OP_LOOP);

	// Calculate how far back we need to jump to return to the start of the loop.
	// The (+2) takes into account the OP_LOOP instruction's two-byte operand.
	int offset = CurrentChunk()->Count - loopStart + 2;
	if (offset > UINT16_MAX)
	{
		Error("Loop body contains too many instructions to jump over.");
	}

	// This bitwise and operation effectively masks the value so only the last 8 bits remain.
	// That's because 0xff doesn't specify the values of its higher 8 bits, so they are all 0.
	EmitByte((offset >> 8) & 0xff);
	EmitByte(offset & 0xff);
}


static int EmitJump(uint8_t instruction)
{
	// Emit the appropriate jump instruction into the bytecode chunk.
	EmitByte(instruction);

	// Emit two placeholder bytes for the jump instruction's operand.
	// We use two bytes so our jump instructions can jump over up to 65,535 bytes of code.
	// Some instruction sets have a separate "long" jump instruction that has a larger
	// operand so it can jump over larger chunks of bytecode.
	EmitByte(0xff);
	EmitByte(0xff);

	// Return the index of the jump instruction we emitted.
	return CurrentChunk()->Count - 2;
}


static void EmitReturn()
{
	if (current->Type == TYPE_INITIALIZER)
	{
		EmitBytes(OP_GET_LOCAL, 0);
	}
	else
	{
		EmitByte(OP_NIL); // Lox functions return nil if there is no return statement in a function.
	}

	EmitByte(OP_RETURN);
}


static uint8_t MakeConstant(Value value)
{
	int constant = AddConstant(CurrentChunk(), value);
	if (constant > UINT8_MAX)
	{
		Error("Cannot add any more constants in this bytecode chunk.");
		return 0;
	}

	return (uint8_t) constant;
}


static void EmitConstant(Value value)
{
	EmitBytes(OP_CONSTANT, MakeConstant(value));
}


/// <summary>
/// This function is used to update a jump instruction after some other code has been compiled.
/// It sets the operand for the instruction so it knows how far to jump.
/// We have to compile some other code first in order to know how far it needs to jump in the
/// bytecode. So this function is used to update a jump instruction once we have that
/// information.
/// </summary>
/// <param name="offset">The index of the jump instruction to update in the bytecode.</param>
static void PatchJump(int offset)
{
	// Calculate how far we need to jump in the bytecode.
	// The -2 is to account for the two dummy bytes that EmitJump() placed
	// after the jump instruction as placeholders for its operand.
	int jump = CurrentChunk()->Count - offset - 2;

	
	if (jump > UINT16_MAX)
	{
		Error("Too much code to jump over.");
	}


	// Write the first and second bytes of the jump instruction's operand into the
	// bytecode chunk.
	// NOTE: This bitwise and operation effectively masks the value so only the last 8 bits remain.
	//		 That's because 0xff doesn't specify the values of its higher 8 bits, so they are all 0.
	CurrentChunk()->Code[offset] = (jump >> 8) & 0xff;
	CurrentChunk()->Code[offset + 1] = jump & 0xff;
}


static void InitCompiler(Compiler* compiler, FunctionType type)
{
	compiler->Enclosing = current;
	compiler->Function = NULL;
	compiler->Type = type;
	compiler->LocalCount = 0;
	compiler->ScopeDepth = 0;

	compiler->Function = NewFunction();

	current = compiler;
	if (type != TYPE_SCRIPT) // Is this compiler compiling a function rather than top-level Lox code?
	{
		current->Function->Name = CopyString(parser.Previous.Start, parser.Previous.Length);
	}

	// Create a Local with a blank name. This is for the VM's own internal use.
	// We give it a blank name so that there is no way a user of Lox can access it.
	Local* local = &current->Locals[current->LocalCount++];
	local->Depth = 0;
	local->IsCaptured = false;
	if (type != TYPE_FUNCTION)
	{
		local->Name.Start = "this";
		local->Name.Length = 4;
	}
	else
	{
		local->Name.Start = "";
		local->Name.Length = 0;
	}

}


static ObjFunction* EndCompiler()
{
	EmitReturn();
	ObjFunction* function = current->Function;
	
	
	// Print out the bytecode the compiler just generated if this preprocessor symbol is
	// defined, and only if there were NO compile errors. If the compiler hit an error,
	// then it won't help us to try to read partially compiled code anyway.
#ifdef DEBUG_PRINT_CODE
	if (!parser.HadError)
	{
		
		DisassembleChunk(CurrentChunk(), function->Name != NULL 
			? std::string("Compiler Debug Output: <fn >").insert(27, std::string(function->Name->Chars)).c_str()
			: "Compiler Debug Output: <script>");
	}
#endif


	current = current->Enclosing;
	return function;
}


static void BeginScope()
{
	current->ScopeDepth++;
}


static void EndScope()
{
	current->ScopeDepth--;

	// Emit bytecode instructions for removing from the stack all local variables that were
	// declared in the scope that is ending.
	while (current->LocalCount > 0 &&
		current->Locals[current->LocalCount - 1].Depth > current->ScopeDepth)
	{
		// Is the variable captured by a closure? See chapter 25 in the book.
		if (current->Locals[current->LocalCount - 1].IsCaptured)
		{
			EmitByte(OP_CLOSE_UPVALUE);
		}
		else // Variable is a local variable that is not captured by a closure.
		{
			EmitByte(OP_POP);
		}

		current->LocalCount--;
	}
}




// Forward declarations. These allow the referenced functions to be accessed by
// ones below this point, even though those functions are defined before
// the ones referenced by these forward declarations.
static void ParseExpression();
static void ParseStatement();
static void ParseDeclarationStatement();
static ParseRule* GetRule(TokenType type);
static void ParsePrecedence(Precedence precedence);




static uint8_t IdentifierConstant(Token* name)
{
	return MakeConstant(OBJ_VAL(CopyString(name->Start, name->Length)));
}


static bool IdentifiersEqual(Token* a, Token* b)
{
	if (a->Length != b->Length)
		return false;

	return memcmp(a->Start, b->Start, a->Length) == 0;
}


static int ResolveLocal(Compiler* compiler, Token* name)
{
	for (int i = compiler->LocalCount - 1; i >= 0; i--)
	{
		Local* local = &compiler->Locals[i];
		if (IdentifiersEqual(name, &local->Name))
		{
			if (local->Depth == -1)
			{
				Error("You can't read a local variable in its own initializer.");
			}

			return i;
		}

	} // End for

	return -1;
}


static int AddUpValue(Compiler* compiler, uint8_t index, bool isLocal)
{
	int upValueCount = compiler->Function->UpValueCount;
	

	// Check if this function already has an UpValue for this variable.
	for (int i = 0; i < upValueCount; i++)
	{
		UpValue* upValue = &compiler->UpValues[i];
		if (upValue->Index == index && upValue->IsLocal == isLocal)
		{
			return i;
		}
	}


	if (upValueCount == UINT8_COUNT)
	{
		Error("Too many closure variables in function.");
		return 0;
	}


	compiler->UpValues[upValueCount].IsLocal = isLocal;
	compiler->UpValues[upValueCount].Index = index;

	return compiler->Function->UpValueCount++;
}


static int ResolveUpValue(Compiler* compiler, Token* name)
{
	if (compiler->Enclosing == NULL)
		return -1;


	int local = ResolveLocal(compiler->Enclosing, name);
	if (local != -1)
	{
		compiler->Enclosing->Locals[local].IsCaptured = true;
		return AddUpValue(compiler, (uint8_t) local, true);
	}


	int upValue = ResolveUpValue(compiler->Enclosing, name);
	if (upValue != -1)
	{
		return AddUpValue(compiler, (uint8_t) upValue, false);
	}


	return -1;
}


/// <summary>
/// This is called to record the existance of a local variable in the compiler.
/// </summary>
/// <param name="name">The token containing the name of the local variable.</param>
static void AddLocal(Token name)
{
	if (current->LocalCount == UINT8_COUNT)
	{
		Error("There are too many local variables in this function.");
		return;
	}

	Local* local = &current->Locals[current->LocalCount++];
	local->Name = name;
	local->Depth = -1; // Indicates that this local variable is not fully initialized yet.
	local->IsCaptured = false;
}


/// <summary>
/// Declares a local variable.
/// </summary>
static void DeclareVariable()
{
	// Is this a global variable?
	if (current->ScopeDepth == 0)
		return;

	Token* name = &parser.Previous;

	for (int i = current->LocalCount - 1; i >= 0; i--)
	{
		Local* local = &current->Locals[i];
		if (local->Depth != -1 && local->Depth < current->ScopeDepth)
		{
			break;
		}

		if (IdentifiersEqual(name, &local->Name))
		{
			Error("There is already a variable with this name in this scope.");
		}
	} // End for

	AddLocal(*name);
}


static void ParseBinaryExpression(bool canAssign)
{
	TokenType operatorType = parser.Previous.Type;
	ParseRule* rule = GetRule(operatorType);
	ParsePrecedence((Precedence)(rule->Precedence + 1));

	switch (operatorType)
	{
		case TOKEN_BANG_EQUAL:
			EmitBytes(OP_EQUAL, OP_NOT);
			break;
		case TOKEN_EQUAL_EQUAL:
			EmitByte(OP_EQUAL);
			break;
		case TOKEN_GREATER:
			EmitByte(OP_GREATER);
			break;
		case TOKEN_GREATER_EQUAL:
			EmitBytes(OP_LESS, OP_NOT);
			break;
		case TOKEN_LESS:
			EmitByte(OP_LESS);
			break;
		case TOKEN_LESS_EQUAL:
			EmitBytes(OP_GREATER, OP_NOT);
			break;
		case TOKEN_PLUS:
			EmitByte(OP_ADD);
			break;
		case TOKEN_MINUS:
			EmitByte(OP_SUBTRACT);
			break;
		case TOKEN_STAR:
			EmitByte(OP_MULTIPLY);
			break;
		case TOKEN_SLASH:
			EmitByte(OP_DIVIDE);
			break;

		default:
			return; // Unreachable.
	} // end switch
}


static uint8_t ParseArgumentList()
{
	uint8_t argCount = 0;

	if (!Check(TOKEN_RIGHT_PAREN))
	{
		do
		{
			ParseExpression();

			if (argCount == 255)
			{
				Error("Can't have more than 255 function arguments.");
			}

			argCount++;

		} while (Match(TOKEN_COMMA));
	}

	Consume(TOKEN_RIGHT_PAREN, "Expected ')' after function arguments.");
	return argCount;
}


static void ParseCallExpression(bool canAssign)
{
	uint8_t argCount = ParseArgumentList();
	EmitBytes(OP_CALL, argCount);
}


static void ParseDotExpression(bool canAssign)
{
	Consume(TOKEN_IDENTIFIER, "Expected property name after '.'.");
	uint8_t name = IdentifierConstant(&parser.Previous);

	if (canAssign && Match(TOKEN_EQUAL))
	{
		ParseExpression();
		EmitBytes(OP_SET_PROPERTY, name);
	}
	else if (Match(TOKEN_LEFT_PAREN))
	{
		uint8_t argCount = ParseArgumentList();
		EmitBytes(OP_INVOKE, name);
		EmitByte(argCount);
	}
	else
	{
		EmitBytes(OP_GET_PROPERTY, name);
	}
}


static void ParseLiteralExpression(bool canAssign)
{
	switch(parser.Previous.Type)
	{
		case TOKEN_FALSE:
			EmitByte(OP_FALSE);
			break;
		case TOKEN_NIL:
			EmitByte(OP_NIL);
			break;
		case TOKEN_TRUE:
			EmitByte(OP_TRUE);
			break;

		default:
			return; // Unreachable.
	} // end switch
}

static void ParseGroupingExpression(bool canAssign)
{
	ParseExpression();
	Consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}


static void ParseNumberExpression(bool canAssign)
{
	double value = strtod(parser.Previous.Start, NULL);
	EmitConstant(NUMBER_VAL(value));
}


/// <summary>
/// Finishes parsing the logical and operator. The first operand has already been compiled by
/// the time this function is called.
/// </summary>
static void ParseAnd_(bool canAssign)
{
	int endJump = EmitJump(OP_JUMP_IF_FALSE);

	EmitByte(OP_POP);
	ParsePrecedence(PREC_AND);

	PatchJump(endJump);
}


/// <summary>
/// Finishes parsing the logical or operator. The first operand has already been compiled by
/// the time this function is called.
/// </summary>
static void ParseOr_(bool canAssign)
{
	int elseJump = EmitJump(OP_JUMP_IF_FALSE);
	int endJump = EmitJump(OP_JUMP);

	PatchJump(elseJump);
	EmitByte(OP_POP);

	ParsePrecedence(PREC_OR);
	PatchJump(endJump);
}


static void ParseStringExpression(bool canAssign)
{
	// Get the string's characters from the Lexeme. The " + 1" and " - 2"
	// parts just trim the leading and trailing quotation marks (") off the ends.
	EmitConstant(OBJ_VAL(CopyString(parser.Previous.Start + 1,
									parser.Previous.Length - 2)));
}


static void NamedVariable(Token name, bool canAssign)
{
	uint8_t getOp, setOp;
	int arg = ResolveLocal(current, &name);
	if (arg != -1)
	{
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	}
	else if ((arg = ResolveUpValue(current, &name)) != -1)
	{
		getOp = OP_GET_UPVALUE;
		setOp = OP_SET_UPVALUE;
	}
	else
	{
		arg = IdentifierConstant(&name);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}


	if (canAssign && Match(TOKEN_EQUAL))
	{
		ParseExpression();
		EmitBytes(setOp, (uint8_t) arg);
	}
	else
	{
		EmitBytes(getOp, (uint8_t) arg);
	}
}


static void ParseVariableExpression(bool canAssign)
{
	NamedVariable(parser.Previous, canAssign);
}


static Token SyntheticToken(const char* text)
{
	Token token;

	token.Start = text;
	token.Length = (int)strlen(text);

	return token;
}


static void ParseSuperExpression(bool canAssign)
{
	if (currentClass == NULL)
	{
		Error("Can't use 'super' outside of a class.");
	}
	else if (!currentClass->HasSuperClass)
	{
		Error("Can't use 'super' in a class with no superclass.");
	}

	Consume(TOKEN_DOT, "Expected '.' after 'super'.");
	Consume(TOKEN_IDENTIFIER, "Expected superclass method name.");
	uint8_t name = IdentifierConstant(&parser.Previous);

	NamedVariable(SyntheticToken("this"), false);
	if (Match(TOKEN_LEFT_PAREN))
	{
		uint8_t argCount = ParseArgumentList();
		NamedVariable(SyntheticToken("super"), false);
		EmitBytes(OP_SUPER_INVOKE, name);
		EmitByte(argCount);
	}
	else
	{
		NamedVariable(SyntheticToken("super"), false);
		EmitBytes(OP_GET_SUPER, name);
	}
}


static void ParseThisExpression(bool canAssign)
{
	if (currentClass == NULL)
	{
		Error("Can't use 'this' outside of a class.");
		return;
	}

	ParseVariableExpression(false);
}


static void ParseUnaryExpression(bool canAssign)
{
	TokenType operatorType = parser.Previous.Type;

	// Compile the operand.
	ParsePrecedence(PREC_UNARY);

	// Emit the operator instruction.
	switch (operatorType)
	{
		case TOKEN_BANG:
			EmitByte(OP_NOT);
			break;

		case TOKEN_MINUS:
			EmitByte(OP_NEGATE);
			break;

		default:
			return; // Unreachable;
	} // End switch.
}




/// <summary>
/// This array is a table that pairs parsing functions with certain tokens. It is basically
/// a table of function pointers.
/// </summary>
/// <remarks>
/// I had to comment out the first part of each line, since that is designated initializer
/// syntax from C (which is not supported in C++). One response to a question on 
/// stackoverflow.com said C++ 20 will support it. As you might guess by the []s, it just
/// lets you set the index in the array that the value should be placed in. The symbols inside
/// the []s are values from the Token enum.
/// 
/// To solve the issue, I simply changed it to valid C++ code instead. Each element of the
/// array is set to a new ParseRule value, but we don't explicitly specify the array index
/// anymore. The Token enum is defined in the same order as this table, so the values all end
/// up in the correct indices in the array anyway, thankfully.
/// 
/// The first column sets the ParseRule struct's Prefix function pointer field.
/// The second column sets the ParseRule struct's Infix function pointer field.
/// The third column sets the ParseRule struct's Precedence field to a value from the Precedence enum.
/// </remarks>
ParseRule rules[] =
{
	/*[TOKEN_LEFT_PAREN]		= */	{ParseGroupingExpression,	ParseCallExpression,	PREC_CALL},
	/*[TOKEN_RIGHT_PAREN]		= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_LEFT_BRACE]		= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_RIGHT_BRACE]		= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_COMMA]				= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_DOT]				= */	{NULL,						ParseDotExpression,		PREC_CALL},
	/*[TOKEN_MINUS]				= */	{ParseUnaryExpression,		ParseBinaryExpression,	PREC_TERM},
	/*[TOKEN_PLUS]				= */	{NULL,						ParseBinaryExpression,	PREC_TERM},
	/*[TOKEN_SEMICOLON]			= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_SLASH]				= */	{NULL,						ParseBinaryExpression,	PREC_FACTOR},
	/*[TOKEN_STAR]				= */	{NULL,						ParseBinaryExpression,	PREC_FACTOR},
	/*[TOKEN_BANG]				= */	{ParseUnaryExpression,		NULL,					PREC_NONE},
	/*[TOKEN_BANG_EQUAL]		= */	{NULL,						ParseBinaryExpression,	PREC_EQUALITY},
	/*[TOKEN_EQUAL]				= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_EQUAL_EQUAL]		= */	{NULL,						ParseBinaryExpression,	PREC_EQUALITY},
	/*[TOKEN_GREATER]			= */	{NULL,						ParseBinaryExpression,	PREC_COMPARISON},
	/*[TOKEN_GREATER_EQUAL]		= */	{NULL,						ParseBinaryExpression,	PREC_COMPARISON},
	/*[TOKEN_LESS]				= */	{NULL,						ParseBinaryExpression,	PREC_COMPARISON},
	/*[TOKEN_LESS_EQUAL]		= */	{NULL,						ParseBinaryExpression,	PREC_COMPARISON},
	/*[TOKEN_IDENTIFIER]		= */	{ParseVariableExpression,	NULL,					PREC_NONE},
	/*[TOKEN_STRING]			= */	{ParseStringExpression,		NULL,					PREC_NONE},
	/*[TOKEN_NUMBER]			= */	{ParseNumberExpression,		NULL,					PREC_NONE},
	/*[TOKEN_AND]				= */	{NULL,						ParseAnd_,				PREC_AND},
	/*[TOKEN_CLASS]				= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_ELSE]				= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_FALSE]				= */	{ParseLiteralExpression,	NULL,					PREC_NONE},
	/*[TOKEN_FOR]				= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_FUN]				= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_IF]				= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_NIL]				= */	{ParseLiteralExpression,	NULL,					PREC_NONE},
	/*[TOKEN_OR]				= */	{NULL,						ParseOr_,				PREC_OR},
	/*[TOKEN_PRINT]				= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_RETURN]			= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_SUPER]				= */	{ParseSuperExpression,		NULL,					PREC_NONE},
	/*[TOKEN_THIS]				= */	{ParseThisExpression,		NULL,					PREC_NONE},
	/*[TOKEN_TRUE]				= */	{ParseLiteralExpression,	NULL,					PREC_NONE},
	/*[TOKEN_VAR]				= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_WHILE]				= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_ERROR]				= */	{NULL,						NULL,					PREC_NONE},
	/*[TOKEN_EOF]				= */	{NULL,						NULL,					PREC_NONE},
};




static void ParsePrecedence(Precedence precedence)
{
	Advance();

	ParseFunction prefixRule = GetRule(parser.Previous.Type)->Prefix;
	if (prefixRule == NULL)
	{
		Error("Expected expression.");
		return;
	}

	// Use the function pointer we just obtained to call the appropriate prefix expression parsing function.
	bool canAssign = precedence <= PREC_ASSIGNMENT;
	prefixRule(canAssign);


	while (precedence <= GetRule(parser.Current.Type)->Precedence)
	{
		Advance();
		ParseFunction infixRule = GetRule(parser.Previous.Type)->Infix;

		// Use the function pointer we just obtained to call the appropriate infix expression parsing function.
		infixRule(canAssign);
	} // End while.

	if (canAssign && Match(TOKEN_EQUAL))
	{
		Error("Invalid assignment target.");
	}
}


static uint8_t ParseVariable(const char* errorMessage)
{
	Consume(TOKEN_IDENTIFIER, errorMessage);

	DeclareVariable();
	if (current->ScopeDepth > 0)
		return 0;

	return IdentifierConstant(&parser.Previous);
}


static void MarkInitialized()
{
	if (current->ScopeDepth == 0)
		return;

	current->Locals[current->LocalCount - 1].Depth = current->ScopeDepth;
}


static void DefineVariable(uint8_t global)
{
	if (current->ScopeDepth > 0)
	{
		MarkInitialized();
		return;
	}

	EmitBytes(OP_DEFINE_GLOBAL, global);
}


static ParseRule* GetRule(TokenType type)
{
	return &rules[type];
}


static void ParseExpression()
{
	ParsePrecedence(PREC_ASSIGNMENT);
}


static void ParseBlock()
{
	while (!Check(TOKEN_RIGHT_BRACE) && !Check(TOKEN_EOF))
	{
		ParseDeclarationStatement();
	}

	Consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}


/// <summary>
/// This compiles a function body and its parameters.
/// </summary>
static void ParseFunctionBody(FunctionType type)
{
	Compiler compiler;
	InitCompiler(&compiler, type);
	
	// Quoted from the book:
	// "This beginScope() doesn’t have a corresponding endScope() call. Because we end Compiler
	// completely when we reach the end of the function body, there’s no need to close the lingering
	// outermost scope."
	BeginScope();


	// Parse the parameter list.
	Consume(TOKEN_LEFT_PAREN, "Expected '(' after function name.");
	if (!Check(TOKEN_RIGHT_PAREN))
	{
		do
		{
			current->Function->Arity++;
			if (current->Function->Arity > 255)
			{
				ErrorAtCurrent("Can't have more than 255 function parameters.");
			}

			uint8_t constant = ParseVariable("Expected function parameter name.");
			DefineVariable(constant);

		} while (Match(TOKEN_COMMA));

	} // End if

	Consume(TOKEN_RIGHT_PAREN, "Expected ')' after function parameters.");
	Consume(TOKEN_LEFT_BRACE, "Expected '{' before function body.");

	ParseBlock();

	ObjFunction* function = EndCompiler();
	EmitBytes(OP_CLOSURE, MakeConstant(OBJ_VAL(function)));


	// Emit data for the UpValues into the bytecode. This will be used by the OP_CLOSURE
	// instruction when the VM (virtual machine) runs our compiled Lox program.
	// See chapter 25 in the book.
	for (int i = 0; i < function->UpValueCount; i++)
	{
		EmitByte(compiler.UpValues[i].IsLocal ? 1 : 0);
		EmitByte(compiler.UpValues[i].Index);
	}
}


static void ParseFunctionDeclaration()
{
	uint8_t global = ParseVariable("Expected function name.");
	MarkInitialized();
	ParseFunctionBody(TYPE_FUNCTION);
	DefineVariable(global);
}


static void ParseClassMethodDeclaration()
{
	Consume(TOKEN_IDENTIFIER, "Expected class method name.");
	uint8_t constant = IdentifierConstant(&parser.Previous);

	FunctionType type = TYPE_METHOD;
	if (parser.Previous.Length == 4 &&
		memcmp(parser.Previous.Start, "init", 4) == 0)
	{
		type = TYPE_INITIALIZER;
	}

	ParseFunctionBody(type);

	EmitBytes(OP_METHOD, constant);
}


static void ParseClassDeclaration()
{
	Consume(TOKEN_IDENTIFIER, "Expected class name.");
	Token className = parser.Previous;
	uint8_t nameConstant = IdentifierConstant(&parser.Previous);
	DeclareVariable();

	EmitBytes(OP_CLASS, nameConstant);
	DefineVariable(nameConstant);

	ClassCompiler classCompiler;
	classCompiler.HasSuperClass = false;
	classCompiler.Enclosing = currentClass;
	currentClass = &classCompiler;

	// Check for class inheritance
	if (Match(TOKEN_LESS))
	{
		Consume(TOKEN_IDENTIFIER, "Expected superclass name.");
		ParseVariableExpression(false);

		if (IdentifiersEqual(&className, &parser.Previous))
		{
			Error("A class can't inherit from itself.");
		}

		BeginScope();
		AddLocal(SyntheticToken("super"));
		DefineVariable(0);

		NamedVariable(className, false);
		EmitByte(OP_INHERIT);
		classCompiler.HasSuperClass = true;
	}

	NamedVariable(className, false);
	Consume(TOKEN_LEFT_BRACE, "Expected '{' before class body.");

	while (!Check(TOKEN_RIGHT_BRACE) && !Check(TOKEN_EOF))
	{
		ParseClassMethodDeclaration();
	}

	Consume(TOKEN_RIGHT_BRACE, "Expected '}' after class body.");
	EmitByte(OP_POP);

	if (classCompiler.HasSuperClass)
	{
		EndScope();
	}

	currentClass = currentClass->Enclosing;
}


static void ParseVarDeclarationStatement()
{
	uint8_t global = ParseVariable("Expected variable name.");

	if (Match(TOKEN_EQUAL))
	{
		ParseExpression();
	}
	else
	{
		// Initialize the variable's value to NIL since the user did not initialize it.
		EmitByte(OP_NIL);
	}

	Consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

	DefineVariable(global);
}


static void ParseExpressionStatement()
{
	ParseExpression();
	Consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
	EmitByte(OP_POP);
}


static void ParseForStatement()
{
	BeginScope();
	Consume(TOKEN_LEFT_PAREN, "Expected '(' after 'for'.");
	

	// Parse the initializer clause.
	if (Match(TOKEN_SEMICOLON))
	{
		// No initializer.
	}
	else if (Match(TOKEN_VAR))
	{
		ParseVarDeclarationStatement();
	}
	else
	{
		ParseExpressionStatement();
	}


	// Parse the condition clause.
	int loopStart = CurrentChunk()->Count;
	int exitJump = -1;
	if (!Match(TOKEN_SEMICOLON))
	{
		ParseExpression();
		Consume(TOKEN_SEMICOLON, "Expected ';' after for loop condition.");

		// Jump out of the loop if the condition is false.
		exitJump = EmitJump(OP_JUMP_IF_FALSE);
		EmitByte(OP_POP); // Pop the loop's condition value off of the stack since it is no longer needed.
	}
	else
	{
		// I added this else clause, but then commented this line out.
		// I think the reason the book doesn't catch this as an error, is because imagine this.
		// You have a for loop inside a function. We could jump out of the loop using Lox's
		// return statement. Therefore, a for loop with no condition expression is not necessarily
		// an infinite loop.
		// Error("For loop condition not specified.");
	}

	
	// Parse the incementer clause.
	if (!Match(TOKEN_RIGHT_PAREN))
	{
		int bodyJump = EmitJump(OP_JUMP);
		int incrementStart = CurrentChunk()->Count;
		
		ParseExpression();
		EmitByte(OP_POP); // Pop the value of the incrementer clause expression off of the stack since it is no longer needed.
		Consume(TOKEN_RIGHT_PAREN, "Expected ')' after for loop clauses.");

		EmitLoop(loopStart);
		loopStart = incrementStart;
		PatchJump(bodyJump);
	}


	ParseStatement();
	EmitLoop(loopStart);

	if (exitJump != -1)
	{
		PatchJump(exitJump);
		EmitByte(OP_POP); // Pop the loop's condition value off of the stack since it is no longer needed.
	}

	EndScope();
}


static void ParseIfStatement()
{
	Consume(TOKEN_LEFT_PAREN, "Expected '(' after 'if'.");
	ParseExpression();
	Consume(TOKEN_RIGHT_PAREN, "Expected ')' after if condition.");

	// Keep track of where the jump instruction is in our bytecode.
	int thenJump = EmitJump(OP_JUMP_IF_FALSE);
	EmitByte(OP_POP); // Emit an instruction to pop the result of the if statement's condition expression off of the stack.
	ParseStatement();

	int elseJump = EmitJump(OP_JUMP);

	// Now that we've compiled the then clause, we know how much byte code it was turned
	// into. That means we can go back and update that OP_JUMP_IF_FALSE instruction
	// with the proper value of how far it should jump when the if statement's
	// condition is false. This trick is called "backpatching".
	PatchJump(thenJump);
	EmitByte(OP_POP); // Emit an instruction to pop the result of the if statement's condition expression off of the stack.
					  // We do this again here, because if the if statement was false, then the first OP_POP instruction
					  // we emitted will be skipped since it is in the code generated by the then clause.

	// Check if there is an else clause.
	if (Match(TOKEN_ELSE))
		ParseStatement();

	PatchJump(elseJump);
}


static void ParsePrintStatement()
{
	ParseExpression();
	Consume(TOKEN_SEMICOLON, "Expected ';' after print statement value");
	EmitByte(OP_PRINT);
}


static void ParseReturnStatement()
{
	if (current->Type == TYPE_SCRIPT)
	{
		Error("Can't return from top-level code.");
	}

	// If this is true, then there is no return value specified in the return statement.
	if (Match(TOKEN_SEMICOLON))
	{
		EmitReturn();
	}
	else
	{
		if (current->Type == TYPE_INITIALIZER)
		{
			Error("Can't return a value from a class initializer.");
		}

		ParseExpression();
		Consume(TOKEN_SEMICOLON, "Expected ';' after return value.");
		EmitByte(OP_RETURN);
	}
}


static void ParseWhileStatement()
{
	// Record the starting position of the loop in the bytecode chunk.
	int loopStart = CurrentChunk()->Count;

	Consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'.");
	ParseExpression();
	Consume(TOKEN_RIGHT_PAREN, "Expected ')' after while loop condition.");

	// This jump is what exits the loop if its condition is false.
	int exitJump = EmitJump(OP_JUMP_IF_FALSE);
	EmitByte(OP_POP);
	ParseStatement();
	EmitLoop(loopStart);

	PatchJump(exitJump);
	EmitByte(OP_POP);
}


static void Synchronize()
{
	parser.PanicMode = false;

	while (parser.Current.Type != TOKEN_EOF)
	{
		if (parser.Previous.Type == TOKEN_SEMICOLON)
			return;

		switch (parser.Current.Type)
		{
			case TOKEN_CLASS:
			case TOKEN_FUN:
			case TOKEN_VAR:
			case TOKEN_FOR:
			case TOKEN_IF:
			case TOKEN_WHILE:
			case TOKEN_PRINT:
			case TOKEN_RETURN:
				return;

			default:
				; // Do nothing.

		} // End switch

		Advance();

	} // End whie
}


static void ParseDeclarationStatement()
{
	if (Match(TOKEN_CLASS))
	{
		ParseClassDeclaration();
	}
	else if (Match(TOKEN_FUN))
	{
		ParseFunctionDeclaration();
	}
	else if (Match(TOKEN_VAR))
	{
		ParseVarDeclarationStatement();
	}
	else
	{
		ParseStatement();
	}


	if (parser.PanicMode)
		Synchronize();
}


static void ParseStatement()
{
	if (Match(TOKEN_PRINT))
	{
		ParsePrintStatement();
	}
	else if (Match(TOKEN_FOR))
	{
		ParseForStatement();
	}
	else if (Match(TOKEN_IF))
	{
		ParseIfStatement();
	}
	else if (Match(TOKEN_RETURN))
	{
		ParseReturnStatement();
	}
	else if (Match(TOKEN_WHILE))
	{
		ParseWhileStatement();
	}
	else if (Match(TOKEN_LEFT_BRACE))
	{
		BeginScope();
		ParseBlock();
		EndScope();
	}
	else
	{
		ParseExpressionStatement();
	}
}


ObjFunction* Compile(const char* source)
{
	InitScanner(source);

	Compiler compiler;
	InitCompiler(&compiler, TYPE_SCRIPT);


	parser.HadError = false;
	parser.PanicMode = false;



#if defined(DEBUG_PRINT_CODE) && defined(DEBUG_PRINT_KEY)
	PrintDebugOutputKey();
#endif



	Advance();
	
	while (!Match(TOKEN_EOF))
	{
		ParseDeclarationStatement();
	}


	ObjFunction* function = EndCompiler();

	return parser.HadError ? NULL : function;

}


void MarkCompilerRoots()
{
	Compiler* compiler = current;

	while (compiler != NULL)
	{
		MarkObject((Obj*)compiler->Function);
		compiler = compiler->Enclosing;
	}
}
