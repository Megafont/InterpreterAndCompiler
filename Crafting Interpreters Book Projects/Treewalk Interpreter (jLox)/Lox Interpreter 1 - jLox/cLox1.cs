using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    /// <summary>
    /// This is the first interpreter in the book, a tree-walk interpreter.
    /// The book made this project in Java, but I wrote it in C# instead.
    /// </summary>
    /// <remarks>
    /// The book author is using the return codes from the UNIX sysexits.h header, as it's the closest he could find to a standard.
    /// This is why the Main() function returns exit code 64 in the error case.
    /// There are several ways to return a program exit code in C#:
    /// https://stackoverflow.com/questions/155610/how-do-i-specify-the-exit-code-of-a-console-application-in-net
    /// </remarks>
    /// <param name="args">The passed in command line arguments.</param>
    public static class cLox1
    {
        /// <summary>
        /// An interpreter object that will interpret Lox code for us.
        /// </summary>
        private static readonly Interpreter _Interpreter = new Interpreter();

        /// <summary>
        /// Indicates whether the Executing Lox script encountered a compile time error.
        /// </summary>
        private static bool _HadError = false; // Rename to _HadCompileError after finishing first half of the book?

        /// <summary>
        /// Indicates whether the executing Lox script encountered a runtime error.
        /// </summary>
        private static bool _HadRuntimeError = false;




        /// <summary>
        /// The main method.
        /// </summary>
        /// <param name="args">The passed in command line arguments.</param>
        static void Main(string[] args)
        {
            if (args.Length > 1)
            {
                Console.WriteLine("Usage #1: cLox1");
                Console.WriteLine("Usage #2: cLox1 [script]");
                System.Environment.Exit(64); // Return an exit code from this application to indicate an error happened.
            }
            else if (args.Length == 1)
            {
                RunFile(args[0]);
            }
            else
            {
                // Start the interactive run prompt where the user can type in code.
                // REPL is an acronym describing the process loop behind the prompt follows: 
                // [R]ead, [E]valuate, [P]rint, and then [L]oop.
                RunPrompt();
            }

            return;
        }


        /// <summary>
        /// Execute the specified script file.
        /// </summary>
        /// <remarks>This mode is accessed by providing a file parameter when running cLox1.exe.</remarks>
        /// <param name="path">The path and filename of the file to execute</param>
        private static void RunFile(string path)
        {
            byte[] bytes = new byte[0];


            try
            {
                bytes = File.ReadAllBytes(path);
            }
            catch (Exception e)
            {
                Console.ForegroundColor = ConsoleColor.Red;

                Console.WriteLine("ERROR: Failed to read in the specified file: ");
                Console.WriteLine("EXCEPTION THROWN: " + e.GetType() + ": " + e.Message);

                Console.ForegroundColor = ConsoleColor.White;

                System.Environment.Exit(74);
            }


            Run(System.Text.Encoding.Default.GetString(bytes));



            // I ADDED THIS LITTLE BLOCK OF CODE.
            // It waits for the user to press a key before exiting this program.
            // That way, they have as much time as they need to read program output.
            // Otherwise, the console window would close instantly after the
            // Lox script has finished executing!
            Console.WriteLine();
            Console.WriteLine("Press any key to continue.");
            Console.ReadKey();



            // Indicate an error in the exit code.
            if (_HadError) 
                System.Environment.Exit(65);

            if (_HadRuntimeError)
                System.Environment.Exit(70);
        }


        /// <summary>
        /// Allows the user to enter and execute Lox commands from an interactive prompt.
        /// </summary>
        /// <remarks>
        /// This mode is accessed by running cLox1.exe without a file parameter.
        /// </remarks>
        private static void RunPrompt()
        {
            for (; ;)
            {
                Console.Write("> ");
                string line = Console.ReadLine();
                if (string.IsNullOrWhiteSpace(line))
                    break;

                Run(line);
                _HadError = false;
            }
        }


        /// <summary>
        /// Executes the passed in command string.
        /// </summary>
        /// <param name="source">The command string to execute.</param>
        private static void Run(string source)
        {
            Scanner scanner = new Scanner(source);
            List<Token> tokens = scanner.ScanTokens();
            Parser parser = new Parser(tokens);
            List<Stmt> statements = parser.Parse();
            

            // Stop if there was a syntax error.
            if (_HadError)
                return;


            Resolver resolver = new Resolver(_Interpreter);
            resolver.Resolve(statements);


            // Stop if there was a resolution error.
            if (_HadError)
                return;


            // This commented out line of code displays the expression as a string using our AstPrinter class.
            //Console.WriteLine(new AstPrinter().Print(expression));
            _Interpreter.Interpret(statements);
        }


        /// <summary>
        /// This method is called when an error has occurred.
        /// </summary>
        /// <param name="line">The line number where the error occurred.</param>
        /// <param name="message">The error message.</param>
        internal static void Error(int line, string message)
        {
            Console.ForegroundColor = ConsoleColor.Red;

            Report(line, "", message);

            Console.ForegroundColor = ConsoleColor.White;
        }


        /// <summary>
        /// This method is called when an arror has occurred.
        /// </summary>
        /// <param name="token">The token that caused the error.</param>
        /// <param name="message">The error message.</param>
        internal static void Error(Token token, string message)
        {
            Console.ForegroundColor = ConsoleColor.Red;

            if (token.Type == TokenType.EOF)
            {
                Report(token.LineNumber, " at end", message);
            }
            else
            {
                Report(token.LineNumber, " at '" + token.Lexeme + "'", message);
            }

            Console.ForegroundColor = ConsoleColor.White;

        }


        /// <summary>
        /// Writes an error message into the console.
        /// </summary>
        /// <param name="line">The line number where the error occurred.</param>
        /// <param name="where">The lexeme of the Token that caused the error.</param>
        /// <param name="message">The error message.</param>
        private static void Report(int line, string where, string message)
        {
            Console.ForegroundColor = ConsoleColor.Red;

            Console.WriteLine("[line " + line + "] Error" + where + ": " + message);

            Console.ForegroundColor = ConsoleColor.White;

            _HadError = true;
        }


        /// <summary>
        /// This method is called when a runtime error happens.
        /// </summary>
        /// <param name="error">The runtime error that has occurred.</param>
        internal static void RuntimeError(RuntimeError error)
        {
            Console.ForegroundColor = ConsoleColor.Red;

            Console.Error.WriteLine("RUNTIME ERROR: " + error.Message + "\n[line " + error._Token.LineNumber + "]");

            Console.ForegroundColor = ConsoleColor.White;

            _HadRuntimeError = true;
        }
    }


}
