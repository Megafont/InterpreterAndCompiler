#include <stdio.h>

// cLox includes.
#include "Debug.h"
#include "Object.h"
#include "Value.h"




void DisassembleChunk(Chunk* chunk, const char* name)
{
	printf("\n== %s ==\n", name);
	
	for (int offset = 0; offset < chunk->Count;)
	{
		// This function returns the offset of the next instruction rather than letting the loop increment it.
		// We do it this way because instructions can have different sizes.
		offset = DisassembleInstruction(chunk, offset);
	}
}


static int ConstantInstruction(const char* name, Chunk* chunk, int offset)
{
	uint8_t constant = chunk->Code[offset + 1];

	// Print out the instruction name and its constant index parameter.
	printf("%-16s %4d '", name, constant);

	// Print out the actual constant value referenced by this instruction.
	PrintValue(chunk->Constants.Values[constant]);

	printf("'\n");

	// Return the offset of the next instruction. This instruction is 2-bytes long, so we add that to the current instruction's offset.
	return offset + 2;
}


static int InvokeInstruction(const char* name, Chunk* chunk, int offset)
{
	uint8_t constant = chunk->Code[offset + 1];
	uint8_t argCount = chunk->Code[offset + 2];

	printf("%-16s (%d args) %4d '", name, argCount, constant);
	PrintValue(chunk->Constants.Values[constant]);
	printf("'\n");

	return offset + 3;
}


static int SimpleInstruction(const char* name, int offset)
{
	// Print out the instruction name.
	printf("%s\n", name);

	// Return the offset of the next instruction.
	// A simple instruction is only one byte, so we add 1 to the current instruction's offset.
	return offset + 1;
}


static int ByteInstruction(const char* name, Chunk* chunk, int offset)
{
	uint8_t slot = chunk->Code[offset + 1];

	// Print out the instruction name and its slot parameter.
	printf("%-16s %4d\n", name, slot);


	return offset + 2;
}


static int JumpInstruction(const char* name, int sign, Chunk* chunk, int offset)
{
	uint16_t jump = (uint16_t)(chunk->Code[offset + 1] << 8);
	jump |= chunk->Code[offset + 2]; // Add 2 to take the jump instruction's two-byte operand into account.

	// Print out the jump instruction's name, its bytecode index, and the bytecode index it will jump to.
	printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);

	return offset + 3;
}


int DisassembleInstruction(Chunk* chunk, int offset)
{
	// Print out the current instruction's index in the bytecode array.
	printf("%04d ", offset);

	// Print out the source code line number of this instruction.
	if (offset > 0 &&
		chunk->Lines[offset] == chunk->Lines[offset - 1])
	{
		// Indicate that this instruction is on the same source code line as the previous one.
		printf("   | ");
	}
	else
	{
		// Print out the source code line number.
		printf("%4d ", chunk->Lines[offset]);
	}


	// Get the instruction at the specified offset.
	uint8_t instruction = chunk->Code[offset];

	// Print human-readable information about it.
	switch (instruction)
	{
		case OP_CONSTANT:
			return ConstantInstruction("OP_CONSTANT", chunk, offset);
		case OP_NIL:
			return SimpleInstruction("OP_NIL", offset);
		case OP_TRUE:
			return SimpleInstruction("OP_TRUE", offset);
		case OP_FALSE:
			return SimpleInstruction("OP_FALSE", offset);
		case OP_POP:
			return SimpleInstruction("OP_POP", offset);
		case OP_GET_LOCAL:
			return ByteInstruction("OP_GET_LOCAL", chunk, offset);
		case OP_SET_LOCAL:
			return ByteInstruction("OP_SET_LOCAL", chunk, offset);
		case OP_GET_GLOBAL:
			return ConstantInstruction("OP_GET_GLOBAL", chunk, offset);
		case OP_DEFINE_GLOBAL:
			return ConstantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
		case OP_SET_GLOBAL:
			return ConstantInstruction("OP_SET_GLOBAL", chunk, offset);
		case OP_GET_UPVALUE:
			return ByteInstruction("OP_GET_UPVALUE", chunk, offset);
		case OP_SET_UPVALUE:
			return ByteInstruction("OP_SET_UPVALUE", chunk, offset);
		case OP_GET_PROPERTY:
			return ConstantInstruction("OP_GET_PROPERTY", chunk, offset);
		case OP_SET_PROPERTY:
			return ConstantInstruction("OP_SET_PROPERTY", chunk, offset);
		case OP_GET_SUPER:
			return ConstantInstruction("OP_GET_SUPER", chunk, offset);
		case OP_EQUAL:
			return SimpleInstruction("OP_EQUAL", offset);
		case OP_GREATER:
			return SimpleInstruction("OP_GREATER", offset);
		case OP_LESS:
			return SimpleInstruction("OP_LESS", offset);
		case OP_ADD:
			return SimpleInstruction("OP_ADD", offset);
		case OP_SUBTRACT:
			return SimpleInstruction("OP_SUBTRACT", offset);
		case OP_MULTIPLY:
			return SimpleInstruction("OP_MULTIPLY", offset);
		case OP_DIVIDE:
			return SimpleInstruction("OP_DIVIDE", offset);
		case OP_NOT:
			return SimpleInstruction("OP_NOT", offset);
		case OP_NEGATE:
			return SimpleInstruction("OP_NEGATE", offset);
		case OP_PRINT:
			return SimpleInstruction("OP_PRINT", offset);
		case OP_JUMP:
			return JumpInstruction("OP_JUMP", 1, chunk, offset);
		case OP_JUMP_IF_FALSE:
			return JumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
		case OP_LOOP:
			return JumpInstruction("OP_LOOP", -1, chunk, offset);
		case OP_CALL:
			return ByteInstruction("OP_CALL", chunk, offset);
		case OP_INVOKE:
			return InvokeInstruction("OP_INVOKE", chunk, offset);
		case OP_SUPER_INVOKE:
			return InvokeInstruction("OP_SUPER_INVOKE", chunk, offset);
		case OP_CLOSURE:
		{
			offset++;
			uint8_t constant = chunk->Code[offset++];
			printf("%-16s %4d ", "OP_CLOSURE", constant);
			PrintValue(chunk->Constants.Values[constant]);
			printf("\n");

			ObjFunction* function = AS_FUNCTION(chunk->Constants.Values[constant]);
			for (int j = 0; j < function->UpValueCount; j++)
			{
				int isLocal = chunk->Code[offset++];
				int index = chunk->Code[offset++];
				printf("%04d      |                     %s %d\n",
					   offset - 2, isLocal ? "local" : "upvalue", index);
			}

			return offset;
		}
		case OP_CLOSE_UPVALUE:
			return SimpleInstruction("OP_CLOSE_UPVALUE", offset);
		case OP_RETURN:
			return SimpleInstruction("OP_RETURN", offset);
		case OP_CLASS:
			return ConstantInstruction("OP_CLASS", chunk, offset);
		case OP_INHERIT:
			return SimpleInstruction("OP_INHERIT", offset);
		case OP_METHOD:
			return ConstantInstruction("OP_METHOD", chunk, offset);

		default:
			printf("ERROR: Unknown opcode (%d)\n", instruction);
			return offset + 1;
	} // End switch

}


void PrintDebugOutputKey()
{
	printf("\n== Debug Output Key ==\n");
	printf("Column 1    The byte index of this opcode in the bytecode chunk.\n");
	printf("Column 2    The source code line number this opcode was generated from. A | means it was generated from the same line as the previous opcode.\n");
	printf("Column 3    This opcode's human-readable name. \n");
	printf("Column 4    For constant instructions, this column shows the constant index.\n");
	printf("            For byte instructions, this column instead shows the local variable slot index).\n");
	printf("            For jump instructions, this column shows the instruction's bytecode index, and the bytecode index it jumps to.\n");
	printf("Column 5    Not always used. For constant instructions, this is the value of the constant at the index shown in column 4.\n\n");

	printf("In the runtime debug output, some lines contain values surrounded by []s. These lines show the values currently stored in the cLox stack.\n");
	printf("Note that the stack debug output lines can be disabled by commenting out the DEBUG_PRINT_STACK preprocessor symbol I added in Common.h.\n");
	printf("The runtime debug output may also be interspersed with values printed out by the OP_PRINT instruction.\n\n");

	printf("fn is short for \"Function\". This abbreviation always appears just before a function name in the debug output.\n\n");

	printf("The OP_CLOSURE instruction displays the function associated with it in the runtime debug output. It also lists all UpValues it contains.\n");
	printf("    'local'    means the variable referenced by the UpValue is still in scope and living on the stack.\n");
	printf("    'upvalue'  means it has gone out of scope, and was copied to the heap for future use by closure(s) that still reference it.\n");
	printf("See chapter 25 of the online book \"Crafting Interpreters\", which this program was built from, for more on closures and UpValues.\n\n");
}

