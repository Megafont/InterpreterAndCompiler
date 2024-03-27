using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    /// <summary>
    /// Parses a list of tokens.
    /// </summary>
    internal class Parser
    {
        /// <summary>
        /// Holds the list of tokens to parse.
        /// </summary>
        private readonly List<Token> _Tokens;


        /// <summary>
        /// Stores the index of the current token.
        /// </summary>
        private int _Current = 0;




        /// <summary>
        /// A simple exception class for parser errors.
        /// </summary>
        private class ParseError : Exception { }




        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="tokens">The list of tokens to parse.</param>
        internal Parser(List<Token> tokens)
        {
            this._Tokens = tokens;
        }




        /// <summary>
        /// This method is called to start the parsing process.
        /// </summary>
        /// <returns>A list of statements parsed from the tokens of the Lox script.</returns>
        internal List<Stmt> Parse()
        {
            List<Stmt> statements = new List<Stmt>();

            while (!IsAtEnd())
            {
                statements.Add(ParseDeclarationStatement());
            }

            return statements;
        }




        #region Expression Parsing

        // ========================================================================================================================================================================================================
        // This group of methods parses expressions and generates a binary syntax tree.
        // Each method parses a different type of expression. There is one for each level of
        // precedence in the grammer of our language. The functions below are in order of
        // precedence.
        // ========================================================================================================================================================================================================


        private Expr ParseExpression()
        {
            return ParseExpression_Assignment();
        }


        private Expr ParseExpression_Assignment()
        {
            Expr expr = ParseExpression_Or();

            if (Match(TokenType.EQUAL))
            {
                Token equals = Previous();
                Expr value = ParseExpression_Assignment();

                if (expr is Expr.Variable)
                {
                    Token name = ((Expr.Variable) expr).Name;
                    return new Expr.Assign(name, value);
                }
                else if (expr is Expr.Get)
                {
                    Expr.Get get = (Expr.Get) expr;
                    return new Expr.Set(get.ClassInstance, get.Name, value);
                }

                Error(equals, "Invaliud assignment target.");
            }

            return expr;
        }


        private Expr ParseExpression_Or()
        {
            Expr expr = ParseExpression_And();

            while (Match(TokenType.OR))
            {
                Token operation = Previous();
                Expr right = ParseExpression_And();
                expr = new Expr.Logical(expr, operation, right);
            }

            return expr;
        }


        private Expr ParseExpression_And()
        {
            Expr expr = ParseExpression_Equality();

            while (Match(TokenType.AND))
            {
                Token operation = Previous();
                Expr right = ParseExpression_Equality();
                expr = new Expr.Logical(expr, operation, right);
            }

            return expr;
        }


        private Expr ParseExpression_Equality()
        {
            Expr expr = ParseExpression_Comparison();

            while (Match(TokenType.BANG_EQUAL, TokenType.EQUAL_EQUAL))
            {
                Token operation = Previous();
                Expr right = ParseExpression_Comparison();
                expr = new Expr.Binary(expr, operation, right);
            }

            return expr;
        }


        private Expr ParseExpression_Comparison()
        {
            Expr expr = ParseExpression_Term();

            while (Match(TokenType.GREATER, TokenType.GREATER_EQUAL, TokenType.LESS, TokenType.LESS_EQUAL))
            {
                Token operation = Previous();
                Expr right = ParseExpression_Term();
                expr = new Expr.Binary(expr, operation, right);
            }

            return expr;
        }


        private Expr ParseExpression_Term()
        {
            Expr expr = ParseExpression_Factor();

            while (Match(TokenType.MINUS, TokenType.PLUS))
            {
                Token operation = Previous();
                Expr right = ParseExpression_Factor();
                expr = new Expr.Binary(expr, operation, right);
            }

            return expr;
        }


        private Expr ParseExpression_Factor()
        {
            Expr expr = ParseExpression_Unary();

            while (Match(TokenType.SLASH, TokenType.STAR))
            {
                Token operation = Previous();
                Expr right = ParseExpression_Unary();
                expr = new Expr.Binary(expr, operation, right);
            }

            return expr;
        }


        private Expr ParseExpression_Unary()
        {
            if (Match(TokenType.BANG, TokenType.MINUS))
            {
                Token operation = Previous();
                Expr right = ParseExpression_Unary();
                return new Expr.Unary(operation, right);
            }

            return ParseExpression_Call();
        }


        private Expr ParseExpression_Call()
        {
            Expr expr = ParseExpression_Primary();

            while(true)
            {
                if (Match(TokenType.LEFT_PAREN))
                {
                    expr = FinishCall(expr);
                }
                else if (Match(TokenType.DOT))
                {
                    Token name = Consume(TokenType.IDENTIFIER, "Expected property name after '.'.");
                    expr = new Expr.Get(expr, name);
                }
                else
                {
                    break;
                }

            } // end while

            return expr;
        }


        private Expr ParseExpression_Primary()
        {
            if (Match(TokenType.FALSE))
                return new Expr.Literal(false);
            if (Match(TokenType.TRUE))
                return new Expr.Literal(true);
            if (Match(TokenType.NIL))
                return new Expr.Literal(null);

            if (Match(TokenType.NUMBER, TokenType.STRING))
            {
                return new Expr.Literal(Previous().Literal);
            }

            if (Match(TokenType.SUPER))
            {
                Token keyword = Previous();
                Consume(TokenType.DOT, "Expected '.' after 'super'.");
                Token method = Consume(TokenType.IDENTIFIER, "Expected superclass method name.");
                return new Expr.Super(keyword, method);
            }

            if (Match(TokenType.THIS))
            {
                return new Expr.This(Previous());
            }

            if (Match(TokenType.IDENTIFIER))
            {
                return new Expr.Variable(Previous());
            }

            if (Match(TokenType.LEFT_PAREN))
            {
                Expr expr = ParseExpression();
                Consume(TokenType.RIGHT_PAREN, "Expected ')' after expression.");
                return new Expr.Grouping(expr);
            }


            throw Error(Peek(), "Expected an expression.");

        }


        /// <summary>
        /// This helper method finishes parsing a function call.
        /// </summary>
        /// <param name="callee">The function being called.</param>
        /// <returns>An Expr.Call expression object representing the function call.</returns>
        private Expr FinishCall(Expr callee)
        {
            List<Expr> arguments = new List<Expr>();

            if (!Check(TokenType.RIGHT_PAREN))
            {
                do
                {
                    if (arguments.Count >= 255)
                    {
                        Error(Peek(), "Can't have more than 255 arguments to a function.");
                    }
                    //else // I feel like the following line should be in an else clause like this.
                    //{
                    arguments.Add(ParseExpression());
                    //}

                } while (Match(TokenType.COMMA));
            }

            Token paren = Consume(TokenType.RIGHT_PAREN, "Expected ')' after function call arguments.");

            return new Expr.Call(callee, paren, arguments);
        }

        // ========================================================================================================================================================================================================

        #endregion




        #region Statement Parsing

        // ========================================================================================================================================================================================================
        // This group of methods parses statements.
        // ========================================================================================================================================================================================================

        private Stmt ParseStatement()
        {
            if (Match(TokenType.FOR))
                return ParseForStatement();
            if (Match(TokenType.IF))
                return ParseIfStatement();
            if (Match(TokenType.PRINT))
                return ParsePrintStatement();
            if (Match(TokenType.RETURN))
                return ParseReturnStatement();
            if (Match(TokenType.WHILE))
                return ParseWhileStatement();
            if (Match(TokenType.LEFT_BRACE))
                return new Stmt.Block(ParseBlockStatement());

            return ParseExpressionStatement();
        }


        private List<Stmt> ParseBlockStatement()
        {
            List<Stmt> statements = new List<Stmt>();

            while (!Check(TokenType.RIGHT_BRACE) && !IsAtEnd())
            {
                statements.Add(ParseDeclarationStatement());
            }

            Consume(TokenType.RIGHT_BRACE, "Expected '}' after block.");
            return statements;
        }


        private Stmt ParseDeclarationStatement()
        {
            try
            {
                if (Match(TokenType.CLASS))
                    return ParseClassDeclarationStatement();
                if (Match(TokenType.FUN))
                    return ParseFunctionDeclarationStatement("function");
                if (Match(TokenType.VAR))
                    return ParseVariableDeclarationStatement();

                return ParseStatement();
            }
            catch (ParseError)
            {
                Synchronize();
                return null;
            }
        }

        
        private Stmt ParseClassDeclarationStatement()
        {
            Token name = Consume(TokenType.IDENTIFIER, "Expected class name.");

            Expr.Variable superClass = null;
            if (Match(TokenType.LESS))
            {
                Consume(TokenType.IDENTIFIER, "Expected superclass name.");
                superClass = new Expr.Variable(Previous());
            }

            Consume(TokenType.LEFT_BRACE, "Expected '{' before class body.");

            List<Stmt.Function> methods = new List<Stmt.Function>();
            while (!Check(TokenType.RIGHT_BRACE) && !IsAtEnd())
            {
                methods.Add(ParseFunctionDeclarationStatement("method"));
            }

            Consume(TokenType.RIGHT_BRACE, "Expected '}' after class body.");

            return new Stmt.Class(name, superClass, methods);
        }


        private Stmt ParseForStatement()
        {
            Consume(TokenType.LEFT_PAREN, "Expected '(' after 'for'.");


            Stmt initializer;
            if (Match(TokenType.SEMICOLON))
            {
                initializer = null;
            }
            else if (Match(TokenType.VAR))
            {
                initializer = ParseVariableDeclarationStatement();
            }
            else
            {
                initializer = ParseExpressionStatement();
            }


            Expr condition = null;
            if (!Check(TokenType.SEMICOLON))
            {
                condition = ParseExpression();
            }

            Consume(TokenType.SEMICOLON, "Expected ';' after for loop condition.");


            Expr increment = null;
            if (!Check(TokenType.RIGHT_PAREN))
            {
                increment = ParseExpression();
            }

            Consume(TokenType.RIGHT_PAREN, "Expected ')' after for loop clauses.");


            Stmt body = ParseStatement();


            if (increment != null)
            {
                body = new Stmt.Block(new List<Stmt>()
                    {
                        body,
                        new Stmt.Expression(increment)
                    }  ); 
                                        
            }

            if (condition == null)
            {
                condition = new Expr.Literal(true);
            }
            body = new Stmt.While(condition, body);

            if (initializer != null)
            {
                body = new Stmt.Block(new List<Stmt>()
                    {
                        initializer,
                        body
                    } );
            }

            return body;
        }


        private Stmt ParseIfStatement()
        {
            Consume(TokenType.LEFT_PAREN, "Expected '(' after 'if'.");
            Expr condition = ParseExpression();
            Consume(TokenType.RIGHT_PAREN, "Expected ')' after if condition.");

            Stmt thenBranch = ParseStatement();
            Stmt elseBranch = null;
            if (Match(TokenType.ELSE))
            {
                elseBranch = ParseStatement();
            }

            return new Stmt.If(condition, thenBranch, elseBranch);
        }

        private Stmt ParseExpressionStatement()
        {
            Expr expr = ParseExpression();
            Consume(TokenType.SEMICOLON, "Expected ';' after expression.");
            return new Stmt.Expression(expr);
        }


        private Stmt ParsePrintStatement()
        {
            Expr value = ParseExpression();
            Consume(TokenType.SEMICOLON, "Expected ';' after value.");
            return new Stmt.Print(value);
        }


        private Stmt ParseReturnStatement()
        {
            Token keyword = Previous();
            Expr value = null;
            if (!Check(TokenType.SEMICOLON))
            {
                value = ParseExpression();
            }

            Consume(TokenType.SEMICOLON, "Expected ';' after return value.");
            return new Stmt.Return(keyword, value);
        }


        private Stmt ParseVariableDeclarationStatement()
        {
            Token name = Consume(TokenType.IDENTIFIER, "Expected variable name.");

            Expr initializer = null;
            if (Match(TokenType.EQUAL))
            {
                initializer = ParseExpression();
            }

            Consume(TokenType.SEMICOLON, "Expected ';' after variable declaration.");
            return new Stmt.Var(name, initializer);
        }


        private Stmt ParseWhileStatement()
        {
            Consume(TokenType.LEFT_PAREN, "Expected '(' after 'while'.");
            Expr condition = ParseExpression();
            Consume(TokenType.RIGHT_PAREN, "Expected ')' after while condition.");
            Stmt body = ParseStatement();

            return new Stmt.While(condition, body);
        }


        private Stmt.Function ParseFunctionDeclarationStatement(string kind)
        {
            Token name = Consume(TokenType.IDENTIFIER, "Expected " + kind + " name.");
            Consume(TokenType.LEFT_PAREN, "Expected '(' after " + kind + " name.");
            List<Token> parameters = new List<Token>();

            if (!Check(TokenType.RIGHT_PAREN))
            {
                do
                {
                    if (parameters.Count >= 255)
                    {
                        Error(Peek(), "Can't have more than 255 " + kind + " parameters.");
                    }

                    parameters.Add(Consume(TokenType.IDENTIFIER, "Expected " + kind + " parameter name."));

                } while (Match(TokenType.COMMA));
            }

            Consume(TokenType.RIGHT_PAREN, "Expected ')' after " + kind + " parameters.");

            Consume(TokenType.LEFT_BRACE, "Expected '{' before " + kind + " body.");
            List<Stmt> body = ParseBlockStatement();
            return new Stmt.Function(name, parameters, body);
        }


        // ========================================================================================================================================================================================================

        #endregion



        
        /// <summary>
        /// Checks if the current token matches any of the specified types.
        /// </summary>
        /// <remarks>
        /// If the current token matches one of the passed in token types, then this function consumes that token and returns true.
        /// Otherwise, the token is not consumed and this function returns false.</remarks>
        /// /// <param name="types">The list of token types to compare the current token to.</param>
        /// <returns>True if the current token is one of the types in the passed in list or false otherwise.</returns>
        private bool Match(params TokenType[] types)
        {
            foreach (TokenType type in types)
            {
                if (Check(type))
                {
                    Advance();
                    return true;
                }
            }

            return false;
        }


        /// <summary>
        /// Consumes the current token if it matches the specified type, and advances the parser to the next one.
        /// If not, then it throws an error.
        /// </summary>
        /// <param name="type">The token type to compare the current token to.</param>
        /// <param name="message">The error message to report if the current token does not match the specified token type.</param>
        /// <returns>The consumed token.</returns>
        private Token Consume(TokenType type, string message)
        {
            if (Check(type))
                return Advance();

            throw Error(Peek(), message);
        }


        /// <summary>
        /// Checks if the current token matches the specified type.
        /// This function never consumes the token, unlike the Match() function.
        /// </summary>
        /// <param name="type">The token type to compare the current token to.</param>
        /// <returns>True if the current token matches the specified type, or false otherwise.</returns>
        private bool Check(TokenType type)
        {
            if (IsAtEnd())
                return false;

            return Peek().Type == type;
        }


        /// <summary>
        /// Advances the parser to the next token.
        /// </summary>
        /// <returns>The previous token.</returns>
        private Token Advance()
        {
            if (!IsAtEnd())
                _Current++;

            return Previous();
        }


        /// <summary>
        /// Checks if we've reached the last token or not.
        /// </summary>
        /// <returns>True if the last token has been reached or false otherwise.</returns>
        private bool IsAtEnd()
        {
            return Peek().Type == TokenType.EOF;
        }


        /// <summary>
        /// Grabs the current token from the list.
        /// </summary>
        /// <returns>The current token.</returns>
        private Token Peek()
        {
            return _Tokens[_Current];
        }


        /// <summary>
        /// Grabs the previous token from the list.
        /// </summary>
        /// <returns>The previous token.</returns>
        private Token Previous()
        {
            return _Tokens[_Current - 1];
        }


        /// <summary>
        /// Reports an error to the main cLox1 class.
        /// </summary>
        /// <param name="token">The token where the error occurred.</param>
        /// <param name="message">The error message.</param>
        /// <returns>A ParseError exception object.</returns>
        private ParseError Error(Token token, string message)
        {
            cLox1.Error(token, message);
            return new ParseError();
        }


        /// <summary>
        /// Synchronizes the parser's state after a syntax error has occurred.
        /// </summary>
        /// <remarks>
        /// This method is first discussed in section 6.3.3 in the book:
        /// https://craftinginterpreters.com/parsing-expressions.html
        /// </remarks>
        private void Synchronize()
        {
            Advance();

            while (!IsAtEnd())
            {
                if (Previous().Type == TokenType.SEMICOLON)
                    return;

                switch (Peek().Type)
                {
                    case TokenType.CLASS:
                    case TokenType.FUN:
                    case TokenType.VAR:
                    case TokenType.FOR:
                    case TokenType.IF:
                    case TokenType.WHILE:
                    case TokenType.PRINT:
                    case TokenType.RETURN:
                        return;

                } // end switch

                Advance();

            } // end while


        }



    }




}
