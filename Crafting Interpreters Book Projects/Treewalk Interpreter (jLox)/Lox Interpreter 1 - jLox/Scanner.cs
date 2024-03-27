using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    /// <summary>
    /// Scans script code and breaks it into its meaningful parts for the interpreter.
    /// </summary>
    /// <remarks>
    /// IMPORTANT: I had to change the code in all calls to string.Substring() to make it work.
    ///            This is because in the book this project is written in Java. Java's Substring() function
    ///            expects the second parameter to be the index to stop at, but in C#, the second parameter
    ///            is the number of characters to extract from the starting index. This problem caused a nasty
    ///            stack overflow error that took me a bit to figure out!
    /// </remarks>
    internal class Scanner
    {
        private readonly string _Source; // The code string to scan.
        private readonly List<Token> _Tokens = new List<Token>(); // A list to hold the tokens extracted from _Source.

        private int _Start = 0; // Index of the first character in the lexeme being scanned.
        private int _Current = 0; // Offset of the character currently being scanned (relative to _Start).
        private int _LineNumber = 1; // The line number of the line that the current character is on.


        // The list of reserved words in the Lox language.
        private static readonly Dictionary<string, TokenType> _Keywords = new Dictionary<string, TokenType>()
        {
            { "and", TokenType.AND },
            { "class", TokenType.CLASS },
            { "else", TokenType.ELSE },
            { "false", TokenType.FALSE },
            { "for", TokenType.FOR },
            { "fun", TokenType.FUN },
            { "if", TokenType.IF },
            { "nil", TokenType.NIL },
            { "print", TokenType.PRINT },
            { "return", TokenType.RETURN },
            { "super", TokenType.SUPER },
            { "this", TokenType.THIS },
            { "true", TokenType.TRUE },
            { "var", TokenType.VAR },
            { "while", TokenType.WHILE }
        };




        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="source">The code string to scan.</param>
        internal Scanner(string source)
        {
            this._Source = source;
        }




        /// <summary>
        /// Scans the script code and extracts all the tokens.
        /// </summary>
        /// <returns>A list of all the tokens extracted from the script code.</returns>
        internal List<Token> ScanTokens()
        {
            while(!IsAtEnd())
            {
                // We are at the beginning of the next lexeme.
                _Start = _Current;
                ScanToken();
            }

            _Tokens.Add(new Token(TokenType.EOF, "", null, _LineNumber));
            return _Tokens;
        }


        private void ScanToken()
        {
            char c = Advance();

            switch (c)
            {
                case '(': AddToken(TokenType.LEFT_PAREN); break;
                case ')': AddToken(TokenType.RIGHT_PAREN); break;
                case '{': AddToken(TokenType.LEFT_BRACE); break;
                case '}': AddToken(TokenType.RIGHT_BRACE); break;
                case ',': AddToken(TokenType.COMMA); break;
                case '.': AddToken(TokenType.DOT); break;
                case '-': AddToken(TokenType.MINUS); break;
                case '+': AddToken(TokenType.PLUS); break;
                case ';': AddToken(TokenType.SEMICOLON); break;
                case '*': AddToken(TokenType.STAR); break;

                case '!': AddToken(Match('=') ? TokenType.BANG_EQUAL : TokenType.BANG);
                    break;
                case '=': AddToken(Match('=') ? TokenType.EQUAL_EQUAL : TokenType.EQUAL);
                    break;
                case '<': AddToken(Match('=') ? TokenType.LESS_EQUAL : TokenType.LESS);
                    break;
                case '>': AddToken(Match('=') ? TokenType.GREATER_EQUAL : TokenType.GREATER);
                    break;

                case '/':
                    if (Match('/'))
                    {
                        // A comment goes until the end of the line.
                        while (Peek() != '\n' && !IsAtEnd())
                            Advance();
                    }
                    else
                    {
                        AddToken(TokenType.SLASH);
                    }
                    break;

                case ' ':
                case '\r':
                case '\t':
                    // Ignore whitespace.
                    break;

                case '\n':
                    _LineNumber++;
                    break;

                case '"': String(); break;

                default:
                    if (IsDigit(c))
                    {
                        Number();
                    }
                    else if (IsAlpha(c))
                    {
                        Identifier();
                    }
                    else
                    {
                        cLox1.Error(_LineNumber, "Unexpected character.");
                    }
                    break;

            } // end switch

        }


        /// <summary>
        /// Extracts an identifier (a keyword, function name, or variable name) from the script being scanned.
        /// </summary>
        private void Identifier()
        {
            while (IsAlphaNumeric(Peek()))
                Advance();

            string text = _Source.Substring(_Start, _Current - _Start);
            TokenType type;
            if (!_Keywords.TryGetValue(text, out type))                           
                type = TokenType.IDENTIFIER;

            AddToken(type);
        }


        /// <summary>
        /// Extracts a numeric literal from the script being scanned.
        /// </summary>
        private void Number()
        {
            while (IsDigit(Peek()))
                Advance();

            // Look for a fractional part.
            if (Peek() == '.' && IsDigit(PeekNext()))
            {
                // Consume the "."
                Advance();

                while (IsDigit(Peek()))
                    Advance();
            }

            AddToken(TokenType.NUMBER, 
                     Double.Parse(_Source.Substring(_Start, _Current - _Start)));
        }


        /// <summary>
        /// Extracts a string literal from the script being scanned.
        /// </summary>
        private void String()
        {
            while (Peek() != '"' && !IsAtEnd())
            {
                if (Peek() == '\n')
                    _LineNumber++;

                Advance();
            }

            if (IsAtEnd())
            {
                cLox1.Error(_LineNumber, "Unterminated string.");
                return;
            }

            // The closing ".
            Advance();

            // Trim the surrounding quotes off the string.
            string value = _Source.Substring(_Start + 1, (_Current - _Start - 2));
            AddToken(TokenType.STRING, value);
        }


        /// <summary>
        /// Checks if the next character matches the expected character.
        /// If so, it returns it and increments _Current;
        /// </summary>
        /// <remarks>
        /// This function is like a conditional version of the Advance() function.
        /// "We only consume the current character if it’s what we’re looking for."
        /// </remarks>
        /// <param name="expected">The expected next character.</param>
        /// <returns>True if the next character matches the expected character, or false otherwise.</returns>
        private bool Match(char expected)
        {
            if (IsAtEnd())
                return false;

            if (_Source[_Current] != expected)
                return false;
            
            _Current++;
            return true;
        }


        /// <summary>
        /// Gets the next character but does not increment _Current.
        /// </summary>
        /// <remarks>
        /// This function is "sort of like Advance(), but doesn’t consume the character. This is called lookahead.
        /// Since it only looks at the current unconsumed character, we have one character of lookahead. The smaller
        /// this number is, generally, the faster the scanner runs. The rules of the lexical grammar dictate how much
        /// lookahead we need. Fortunately, most languages in wide use peek only one or two characters ahead."
        /// </remarks>
        /// <returns>The next character.</returns>
        private char Peek()
        {
            if (IsAtEnd())
                return '\0';

            return _Source[_Current];
        }


        /// <summary>
        /// Gets the character after the next character, but does not increment _Current.
        /// </summary>
        /// <remarks>
        /// This function is just like Peek(), except that it gets the character after the next character.
        /// </remarks>
        /// <returns>The character after the next character.</returns>
        private char PeekNext()
        {
            if (_Current + 1 >= _Source.Length)
                return '\0';

            return _Source[_Current + 1];
        }


        /// <summary>
        /// Checks if the specified character is a letter or an underscore.
        /// </summary>
        /// <param name="c">The character to check.</param>
        /// <returns>True if the specified character is a letter or an underscore.</returns>
        private bool IsAlpha(char c)
        {
            return (c >= 'a' && c <= 'z') ||
                   (c >= 'A' && c <= 'Z') ||
                    c == '_';
        }


        /// <summary>
        /// Checks if the specified character is a letter, an underscore, or a digit.
        /// </summary>
        /// <param name="c">The character to check.</param>
        /// <returns>True if the specified character is a letter, an underscore, or a digit.</returns>
        private bool IsAlphaNumeric(char c)
        {
            return IsAlpha(c) || IsDigit(c);
        }


        /// <summary>
        /// Checks if the specified character is a digit.
        /// </summary>
        /// <param name="c">The character to check.</param>
        /// <returns>True if the specified character is a digit, or false otherwise.</returns>
        private bool IsDigit(char c)
        {
            return c >= '0' && c <= '9';
        }


        /// <summary>
        /// Advances the scanner to the next character in the script being scanned.
        /// </summary>
        /// <returns>The next character to look at.</returns>
        private char Advance()
        {
            return _Source[_Current++];
        }


        /// <summary>
        /// Adds a token to the tokens list.
        /// </summary>
        /// <param name="type">The token type of the token to add.</param>
        private void AddToken(TokenType type)
        {
            AddToken(type, null);
        }


        /// <summary>
        /// Adds a token to the tokens list.
        /// </summary>
        /// <param name="type">The token type of the token to add.</param>
        /// <param name="literal">The literal for the token being added.</param>
        private void AddToken(TokenType type, object literal)
        {
            string text = _Source.Substring(_Start, _Current - _Start);
            _Tokens.Add(new Token(type, text, literal, _LineNumber));
        }


        /// <summary>
        /// Checks if we've reached the end of the script code we're scanning.
        /// </summary>
        /// <returns>True if the end of the script has been reached or false otherwise.</returns>
        private bool IsAtEnd()
        {
            return _Current >= _Source.Length;
        }
    }


}
