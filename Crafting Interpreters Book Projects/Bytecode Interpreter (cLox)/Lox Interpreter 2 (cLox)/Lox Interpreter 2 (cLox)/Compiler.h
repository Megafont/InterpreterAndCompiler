// This file containts code for the compiler.
//

#pragma once

// #ifndef cLox_Compiler_h
//	#define cLox_Compiler_h


// cLox includes.
#include "Object.h"
#include "VM.h"




ObjFunction* Compile(const char* source);

void MarkCompilerRoots(); // Used by the cLox garbage collector. See chapter 26 in the book.

// #endif
