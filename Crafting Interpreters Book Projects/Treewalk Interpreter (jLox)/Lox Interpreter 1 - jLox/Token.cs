using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    /// <summary>
    /// Stores the data of a script token.
    /// </summary>
    internal class Token
    {
        internal readonly TokenType Type; // The type of this token.
        internal readonly string Lexeme; // A meaningful substring from the line of code, like a function name.
        internal readonly object Literal; // "The living version of a literal that will be used by the interpreter later", if applicable. For example, the string literal "ABC" turned into a usable object for the interpreter.
        internal readonly int LineNumber; // The line number this token is on in the script.




        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="type">The token type.</param>
        /// <param name="lexeme">The token's lexeme.</param>
        /// <param name="literal">"The living version of a literal that will be used by the interpreter later", if applicable. For example, the string literal "ABC" turned into a usable object for the interpreter.</param>
        /// <param name="lineNumber">The line number the token is on in the script.</param>
        public Token(TokenType type, string lexeme, object literal, int lineNumber)
        {
            this.Type = type;
            this.Lexeme = lexeme;
            this.Literal = literal;
            this.LineNumber = lineNumber;
        }


        /// <summary>
        /// Creates a string representing this token object.
        /// </summary>
        /// <returns>A string representation of this token object.</returns>
        public override string ToString()
        {
            return Type + " " + Lexeme + " " + Literal;
        }

    }
}
