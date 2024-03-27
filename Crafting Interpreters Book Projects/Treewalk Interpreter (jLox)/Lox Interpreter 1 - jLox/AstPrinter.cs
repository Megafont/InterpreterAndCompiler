using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    /* This entire class is commented out, because it was for debugging early on.
     * It will cause compile errors since it does not implement all of the newer visitor methods
     * required by the Expr.Visitor interface.
           
     
    internal class AstPrinter : Expr.Visitor<string>
    {
        /// <summary>
        /// A method that was used for running a simple test on this class before the parser had been developed.
        /// </summary>
        /// <remarks>
        /// To be able to run this test method, you'll have to:
        /// 1. Go into the Lox Interpreter 1 project properties.
        /// 2. Change the "Startup Object" setting to be this AstPrinter class, rather than the CLox1 class.
        /// 3. Build and run the project.
        /// 4. Don't forget to set the "Startup Object" setting back to the cLox1 class when you're done!
        /// </remarks>
        /// <param name="args">The command line arguements passed into this command line app.</param>
        internal static void Main(string[] args)
        {
            Expr expression = new Expr.Binary(
                new Expr.Unary(
                    new Token(TokenType.MINUS, "-", null, 1),
                    new Expr.Literal(123)),
                new Token(TokenType.STAR, "*", null, 1),
                new Expr.Grouping(
                    new Expr.Literal(45.67)));

            Console.WriteLine(new AstPrinter().Print(expression));

            Console.WriteLine();
            Console.WriteLine("Press any key to continue.");
            Console.ReadKey();

        }




        // Prints the passed in AST (abstract syntax tree).
        internal string Print(Expr expr)
        {
            return expr.Accept(this);
        }


        /// <summary>
        /// Converts the passed in list of expressions into a string containing the passed in expressions grouped in a pair of parentheses.
        /// </summary>
        /// <param name="lexemeName">The lexeme name.</param>
        /// <param name="expressions">The expressions that need to be grouped in a set of parentheses.</param>
        /// <returns>A string containing the passed in expression list grouped in a pair of parentheses.</returns>
        private string Parenthesize(string lexemeName, params Expr[] expressions)
        {
            StringBuilder builder = new StringBuilder();

            builder.Append("(").Append(lexemeName);
            foreach (Expr expr in expressions)
            {
                builder.Append(" ");
                builder.Append(expr.Accept(this)); // The Accept() function is a recursive function.
            }
            builder.Append(")");

            return builder.ToString();
        }





        // ========================================================================================================================================================================================================
        // Below are implementations of the methods in the Expr.Visitor interface.
        // ========================================================================================================================================================================================================

        public string VisitBinaryExpr(Expr.Binary expr)
        {
            return Parenthesize(expr.Operation.Lexeme, expr.Left, expr.Right);
        }


        public string VisitGroupingExpr(Expr.Grouping expr)
        {
            return Parenthesize("group", expr.ExpressionObject);
        }


        public string VisitLiteralExpr(Expr.Literal expr)
        {
            if (expr.Value == null) return "nil";
            return expr.Value.ToString();
        }


        public string VisitUnaryExpr(Expr.Unary expr)
        {
            return Parenthesize(expr.Operation.Lexeme, expr.Right);
        }

        // ========================================================================================================================================================================================================


    }


    */
}
