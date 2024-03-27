// This file defines common cLox interpreter stuff.
//

#pragma once

//#ifndef cLox_Common_h
//	#define cLox_Common_h

// cLox includes.
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



// When enabled, this causes the virtual machine to use the new value encoding that was added in chapter 30
// of the book. This slashes the amount of memory needed to store values in cLox. That in turn can reduce
// CPU cache misses, and thus also increase performance.
#define NAN_BOXING

#define DEBUG_PRINT_KEY // I added this. When this symbol is defined, a description of the columns in the debug output will be displayed.
#define DEBUG_PRINT_STACK // I added this. When enabled, the debug output prints out the cLox stack after every opcode runs, just like it does normally in the book.

#define DEBUG_PRINT_CODE // Enables debug code that prints out the compiled bytecode from the compiler.
#define DEBUG_TRACE_EXECUTION // Enables debug code in the virtual machine.

// #define DEBUG_STRESS_GC // Enables the stress test mode for the cLox garbage collector. This causes the garbage collector to run as often as possible. This is useful for debugging. See chapter 26 in the book.
// #define DEBUG_LOG_GC // Enables debug logging for the garbage collector.

#define UINT8_COUNT (UINT8_MAX + 1)

//#endif

