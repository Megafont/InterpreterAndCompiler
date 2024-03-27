// Lox Interpreter 2 (cLox).cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// cLox includes.
#include "Common.h"
#include "Chunk.h"
#include "Debug.h"
#include "VM.h"




/// <summary>
/// This static function is the interactive run prompt where users can type in lines of Lox script,
/// which are then immediately executed.
/// </summary>
/// <remarks>
/// REPL is an acronym describing the process loop behind the prompt follows: 
/// [R]ead, [E]valuate, [P]rint, and then [L]oop.
/// </remarks>
static void REPL()
{
    char line[1024];

    for (;;)
    {
        printf("> ");


        if (!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }


        // I added this so hitting return without entering any characters exits the
        // interactive prompt, just like in jLox.
        // If you only hit return, then the string is one char long since it only
        // contains one character.
        if (strlen(line) <= 1)
            break;
        

        Interpret(line);

    } // end for
}


/// <summary>
/// This function loads in a Lox script file.
/// </summary>
/// <param name="path">The file path of the Lox script to load.</param>
/// <returns>A char buffer containing the contents of the file.</returns>
static char* ReadFile(const char* path)
{
    FILE* file;
    
    // I changed this line to fopen_s() from fopen(), because the compiler bitched with an
    // error about fopen() being deprecated.
    fopen_s(&file, path, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74); // Exit this program with error code.
    }


    // Jump to the end of the file.
    fseek(file, 0L, SEEK_END);

    // Find out how many bytes we are from the end of the file (the file size).
    size_t fileSize = ftell(file);

    // Rewind to the start of the file.
    rewind(file);


    // Create a char buffer one character longer than the file.
    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL)
    {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74); // Exit this program with error code.
    }


    // Read in the file.
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74); // Exit this program with error code.
    }


    // Add a null character at the end of the buffer.
    buffer[bytesRead] = '\0';


    fclose(file);
    return buffer;
}


/// <summary>
/// Runs a Lox script file.
/// </summary>
/// <param name="path">The file path of the Lox script file to execute.</param>
static void RunFile(const char* path)
{
    char* source = ReadFile(path);
    InterpretResult result = Interpret(source);
    free(source);

    // Indicate an error in the exit code.
    if (result == INTERPRET_COMPILE_ERROR)
        exit(65);
    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
}


int main(int argc, const char* argv[])
{
    //std::cout << "Hello World!\n";

    InitVM();

    if (argc == 1)
    {
        // Start the interactive run prompt where the user can type in code.
        REPL();
    }
    else if (argc == 2)
    {
        // Run the Lox script file that was passed into this program as a command line argument.
        RunFile(argv[1]);
    }
    else
    {
        fprintf(stderr, "Usage: cLox [path]\n");
        exit(64); // Return an exit code from this application to indicate an error happened.
    }


    FreeVM();

    return 0;
}


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
