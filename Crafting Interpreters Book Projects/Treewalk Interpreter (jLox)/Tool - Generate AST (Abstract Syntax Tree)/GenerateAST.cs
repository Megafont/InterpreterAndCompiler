using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Text;
using System.Threading.Tasks;

namespace Tool_Generate_AST_Abstract_Syntax_Tree
{
    public class GenerateAST
    {

        private static string[] _Indent = new string[]
        {
            "", // Namespace indent level
            "    ", // Class indent level
            "        ", // Subclass indent level
            "            ", // Subclass method indent level
            "                ", // Subclass method code indent level
        };



        /// <summary>
        /// This is a simple tool that generates some of the expression and statement classes for Lox Interpreter 1.
        /// It saves a lot of time not having to type them all ourselves as we develop the Lox interpreter.
        /// </summary>
        /// <remarks>
        /// NOTE: AST = Abstract Syntax Tree
        /// 
        /// The book author is using the return codes from the UNIX sysexits.h header, as it's the closest he could find to a standard.
        /// This is why the Main() function returns exit code 64 in the error case.
        /// There are several ways to return a program exit code in C#:
        /// https://stackoverflow.com/questions/155610/how-do-i-specify-the-exit-code-of-a-console-application-in-net
        /// </remarks>
        /// <param name="args">The passed in command line arguments.</param>
        static void Main(string[] args)
        {
            if (args.Length > 1)
            {
                Console.WriteLine("Usage #1: GenerateAST");
                Console.WriteLine("Usage #2: GenerateAST <output directory>");
                Console.WriteLine();
                Console.WriteLine("The first usage will output the generated file in the same folder as this program.");
                Environment.Exit(64);
            }


            string outputDir = "";

            // Get the output directory.
            if (args.Length < 1)
                outputDir = Directory.GetCurrentDirectory();
            else if (args.Length == 1)
                outputDir = args[0];
            else
            {
                // Execution should never get here because of the error checking at the top of this function.
            }


            Console.WriteLine("Generating output files at:");
            Console.WriteLine("    " + outputDir + "\\");
            Console.WriteLine();


            DefineAST(outputDir, "Expr", new List<string>()
            {
                "Assign   : Token name, Expr value",
                "Binary   : Expr left, Token operation, Expr right",
                "Call     : Expr callee, Token paren, List<Expr> arguments",
                "Get      : Expr classInstance, Token name",
                "Grouping : Expr expressionObject",
                "Literal  : Object value",
                "Logical  : Expr left, Token operation, Expr right",
                "Set      : Expr classInstance, Token name, Expr value",
                "Super    : Token keyword, Token method",
                "This     : Token keyword",
                "Unary    : Token operation, Expr right",
                "Variable : Token name",
            });


            DefineAST(outputDir, "Stmt", new List<string>()
            {
                "Block      : List<Stmt> statements",
                "Class      : Token name, Expr.Variable superClass, List<Stmt.Function> methods",
                "Expression : Expr expressionObject",
                "Function   : Token name, List<Token> parameters, List<Stmt> body",
                "If         : Expr condition, Stmt thenBranch, Stmt elseBranch",
                "Print      : Expr expressionObject",
                "Return     : Token keyword, Expr value",
                "Var        : Token name, Expr initializer",
                "While      : Expr condition, Stmt body",
            });


            Console.WriteLine();
            Console.WriteLine("Press any key to continue.");
            Console.ReadKey();
         }


        /// <summary>
        /// Generates certain source code files in the Lox interpreter.
        /// </summary>
        /// <param name="outputDir">The directory to output the source code files in.</param>
        /// <param name="baseName">The base class name that all other classes in the types list will be based on.</param>
        /// <param name="types">The subclasses to create from the base class.</param>
        /// <exception cref="IOException"</exception>
        private static void DefineAST(string outputDir, string baseName, List<string> types)
        {
            string path = outputDir + "/" + baseName + ".cs";


            using (StreamWriter streamWriter = new StreamWriter(path))
            {
                // Write using statements.
                streamWriter.WriteLine(_Indent[0] + "using System;");
                streamWriter.WriteLine(_Indent[0] + "using System.Collections.Generic;");
                WriteDoubleBlankLine(streamWriter);


                // Write the namespace.
                streamWriter.WriteLine(_Indent[0] + "namespace LoxInterpreter1_TreeWalkInterpreter");
                streamWriter.WriteLine(_Indent[0] + "{");




                // Generate the base class.
                streamWriter.WriteLine(_Indent[1] + "internal abstract class " + baseName);
                streamWriter.WriteLine(_Indent[1] + "{");

                // Generate the visitor interface.
                DefineVisitor(streamWriter, baseName, types);



                // Generate the Accept() method.
                streamWriter.WriteLine(_Indent[2] + "internal abstract R Accept<R>(Visitor<R> visitor);");
                WriteDoubleBlankLine(streamWriter);


                // Generate the AST classes that inherit from the base class.
                foreach (string type in types)
                {
                    string className = type.Split(':')[0].Trim();
                    string fields = type.Split(':')[1].Trim();

                    DefineType(streamWriter, baseName, className, fields);
                }
                WriteDoubleBlankLine(streamWriter);




                // End the base class.
                streamWriter.WriteLine(_Indent[1] + "}");
                WriteDoubleBlankLine(streamWriter);


                // End the namespace.
                streamWriter.WriteLine(_Indent[0] + "}");

            } // end using

        }


        /// <summary>
        /// Defines the visitor interface for one of the AST classes.
        /// </summary>
        private static void DefineVisitor(StreamWriter streamWriter, string baseName, List<string> types)
        {
            WriteDoubleBlankLine(streamWriter);


            // Start the interface.
            streamWriter.WriteLine(_Indent[2] + "internal interface Visitor<R>");
            streamWriter.WriteLine(_Indent[2] + "{");

            // Write the visitor methods.
            foreach (string type in types)
            {
                string typeName = type.Split(':')[0].Trim();
                streamWriter.WriteLine(_Indent[3] + "    R Visit" + typeName + baseName + "(" + typeName + " " + baseName.ToLower() + ");");
            }

            // End the interface.
            streamWriter.WriteLine(_Indent[2] + "}");
            WriteDoubleBlankLine(streamWriter);

        }


        /// <summary>
        /// Generates one of the AST classes.
        /// </summary>
        /// <param name="streamWriter">The file stream to write into.</param>
        /// <param name="baseName">The base class name.</param>
        /// <param name="className">The subclass name.</param>
        /// <param name="fieldList">The subclass' field list.</param>
        private static void DefineType(StreamWriter streamWriter, string baseName, string className, string fieldList)
        {
            WriteDoubleBlankLine(streamWriter);


            // Start the class.
            streamWriter.WriteLine(_Indent[2] + "internal class " + className + " : " + baseName);
            streamWriter.WriteLine(_Indent[2] + "{");
            WriteDoubleBlankLine(streamWriter);


            // Write the constructor.
            streamWriter.WriteLine(_Indent[3] + "internal " + className + "(" + fieldList + ")");
            streamWriter.WriteLine(_Indent[3] + "{");

            // Store the constructor's parameters in the fields.
            string[] fields = fieldList.Split(new string[] { ", " }, StringSplitOptions.RemoveEmptyEntries);
            foreach (string field in fields)
            {
                string name = field.Split(' ')[1];
                streamWriter.WriteLine(_Indent[3] + "    this." + GetFieldNameFromParameterName(name) + " = " + name + ";");
            }


            // End the constructor.
            streamWriter.WriteLine(_Indent[3] + "}");
            WriteDoubleBlankLine(streamWriter);


            // Create the Visitor() method.
            streamWriter.WriteLine(_Indent[3] + "internal override R Accept<R>(Visitor<R> visitor)");
            streamWriter.WriteLine(_Indent[3] + "{");
            streamWriter.WriteLine(_Indent[3] + "    return visitor.Visit" + className + baseName + "(this);");
            streamWriter.WriteLine(_Indent[3] + "}");
            WriteDoubleBlankLine(streamWriter);


            // Write the fields.
            foreach (string field in fields)
            {
                string[] parts = field.Split(' ');
            
                string fieldType = parts[0];
                string fieldName = GetFieldNameFromParameterName(parts[1]);
                
                streamWriter.WriteLine(_Indent[3] + "internal readonly " + fieldType + " " + fieldName + ";");
            }
            WriteDoubleBlankLine(streamWriter);


            // End the class.
            streamWriter.WriteLine(_Indent[2] + "}");
            WriteDoubleBlankLine(streamWriter);
        }


        /// <summary>
        /// Writes two blank lines into the file stream.
        /// </summary>
        /// <param name="streamWriter">The StreamWriter to write to.</param>
        private static void WriteDoubleBlankLine(StreamWriter streamWriter)
        {
            streamWriter.WriteLine();
            streamWriter.WriteLine();
        }


        /// <summary>
        /// Takes the passed in parameter name and converts it to the corresponding field name.
        /// </summary>
        /// <param name="paramName">The parameter name to convert into a field name.</param>
        /// <returns>The passed in parameter name with its first letter capitalized.</returns>
        private static string GetFieldNameFromParameterName(string paramName)
        {
            // Capitalize the first letter of the parameter name.
            return paramName[0].ToString().ToUpper() + paramName.Substring(1, paramName.Length - 1);

        }

    }

}
