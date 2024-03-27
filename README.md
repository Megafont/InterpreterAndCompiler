# Interpreter And Compiler

This was a fun education project I did on my own over the summer of 2021. Following the book [Crafting Interpreters](https://craftinginterpreters.com/) by Robert Nystrom. It actually takes you through building two separate versions of the same made up programming language: Lox. The first project (known as jLox since he wrote it in Java) is a treewalk interpreter. I chose to write this in C# instead, which required a few changes of course.

The second half of the book takes you through creating a bytecode compiler (known as cLox since he wrote it in C) for the same fictional programming language. Since I'm familiar with C++ but not C, I wrote it in C++ instead, so it required some little modifications.

By the end of their respective sections, both projects support a fairly simple but powerful programming language (Lox), which includes classes, methods, functions, expressions, inheritance, and a few more things.

Both projects allow you to type in code at the command line since they are both console apps. They both also have the option of running scripts stored in text files with the *.Lox file extension. You do this by running the program and giving it one command line argument: the filename of the script you want to run.
