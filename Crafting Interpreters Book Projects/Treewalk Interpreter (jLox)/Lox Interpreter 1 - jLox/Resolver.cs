using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    /// <summary>
    /// This class resolves variables, and was added to solve a bug in Chapter 11 of the book.
    /// </summary>
    internal class Resolver : Expr.Visitor<object>, Stmt.Visitor<object>
    {
        private enum FunctionType
        {
            NONE,
            FUNCTION,
            INITIALIZER, // Class constructor
            METHOD,
        }

        private enum ClassType
        {
            NONE,
            CLASS,
            SUBCLASS,
        }


        private ClassType _CurrentClass = ClassType.NONE;


        private readonly Interpreter _Interpreter;
        private readonly Stack<Dictionary<string, bool>> _Scopes = new Stack<Dictionary<string, bool>>();
        private FunctionType _CurrentFunction = FunctionType.NONE;




        internal Resolver(Interpreter interpreter)
        {
            _Interpreter = interpreter;
        }




        #region Resolving Expressions

        // ========================================================================================================================================================================================================
        // Below are implementations of the methods in the Expr.Visitor interface.
        // ========================================================================================================================================================================================================

        public object VisitAssignExpr(Expr.Assign expr)
        {
            Resolve(expr.Value);
            ResolveLocal(expr, expr.Name);

            return null;
        }


        public object VisitBinaryExpr(Expr.Binary expr)
        {
            Resolve(expr.Left);
            Resolve(expr.Right);

            return null;
        }


        public object VisitCallExpr(Expr.Call expr)
        {
            Resolve(expr.Callee);

            foreach (Expr argument in expr.Arguments)
            {
                Resolve(argument);
            }

            return null;
        }


        public object VisitGetExpr(Expr.Get expr)
        {
            Resolve(expr.ClassInstance);

            return null;
        }


        public object VisitGroupingExpr(Expr.Grouping expr)
        {
            Resolve(expr.ExpressionObject);

            return null;
        }


        public object VisitLiteralExpr(Expr.Literal expr)
        {
            return null;
        }


        public object VisitLogicalExpr(Expr.Logical expr)
        {
            Resolve(expr.Left);
            Resolve(expr.Right);

            return null;
        }


        public object VisitSetExpr(Expr.Set expr)
        {
            Resolve(expr.Value);
            Resolve(expr.ClassInstance);

            return null;
        }


        public object VisitSuperExpr(Expr.Super expr)
        {
            if (_CurrentClass == ClassType.NONE)
            {
                cLox1.Error(expr.Keyword, "Can't use 'super' outside of a class.");
            }
            else if (_CurrentClass != ClassType.SUBCLASS)
            {
                cLox1.Error(expr.Keyword, "Can't use 'super' in a class that has no superclass.");
            }

            ResolveLocal(expr, expr.Keyword);
            
            return null;
        }


        public object VisitThisExpr(Expr.This expr)
        {
            if (_CurrentClass == ClassType.NONE)
            {
                cLox1.Error(expr.Keyword, "Can't use 'this' outside of a class.");
                return null;
            }

            ResolveLocal(expr, expr.Keyword);

            return null;
        }


        public object VisitUnaryExpr(Expr.Unary expr)
        {
            Resolve(expr.Right);

            return null;
        }


        public object VisitVariableExpr(Expr.Variable expr)
        {
            /* This code had to be changed from how it is in the book to make it
             * work in C#. Here is the original if statement header:
             * 
                if (_Scopes.Count > 0 &&
                    _Scopes.Peek()[expr.Name.Lexeme] == false) 
            */
            if (_Scopes.Count > 0)
            {
                Dictionary<string, bool> scope = _Scopes.Peek();
                bool value;

                // From the book in chapter 11:
                // "First, we check to see if the variable is being accessed inside its
                // own initializer. This is where the values in the scope map come into
                // play. If the variable exists in the current scope but its value is
                // false, that means we have declared it but not yet defined it.
                // We report that error."
                if (scope.TryGetValue(expr.Name.Lexeme, out value) && value == false)
                {
                    cLox1.Error(expr.Name, "Can't read local variable in its own initializer.");
                }
            }
            

            ResolveLocal(expr, expr.Name);

            return null;
        }

        // ========================================================================================================================================================================================================

        #endregion




        #region Resolving Statements

        // ========================================================================================================================================================================================================
        // Below are implementations of the statements in the Stmt.Visitor interface.
        // ========================================================================================================================================================================================================

        public object VisitBlockStmt(Stmt.Block stmt)
        {
            BeginScope();
            Resolve(stmt.Statements);
            EndScope();

            return null;
        }


        public object VisitClassStmt(Stmt.Class stmt)
        {
            ClassType enclosingClass = _CurrentClass;
            _CurrentClass = ClassType.CLASS;


            Declare(stmt.Name);
            Define(stmt.Name);

            if (stmt.SuperClass != null &&
                stmt.Name.Lexeme == stmt.SuperClass.Name.Lexeme)
            {
                cLox1.Error(stmt.SuperClass.Name, "A class can't inherit from itself.");
            }

            if (stmt.SuperClass != null)
            {
                _CurrentClass = ClassType.SUBCLASS;
                Resolve(stmt.SuperClass);

                BeginScope();
                _Scopes.Peek().Add("super", true);
            }

            BeginScope();
            _Scopes.Peek().Add("this", true);

            foreach (Stmt.Function method in stmt.Methods)
            {
                FunctionType declaration = FunctionType.METHOD;
                if (method.Name.Lexeme == "init")
                    declaration = FunctionType.INITIALIZER;

                ResolveFunction(method, declaration);
            }


            EndScope();

            if (stmt.SuperClass != null)
                EndScope();
                       

            _CurrentClass = enclosingClass;


            return null;
        }


        public object VisitExpressionStmt(Stmt.Expression stmt)
        {
            Resolve(stmt.ExpressionObject);

            return null;
        }


        public object VisitFunctionStmt(Stmt.Function stmt)
        {
            Declare(stmt.Name);
            Define(stmt.Name);

            ResolveFunction(stmt, FunctionType.FUNCTION);

            return null;
        }


        public object VisitIfStmt(Stmt.If stmt)
        {
            Resolve(stmt.Condition);
            Resolve(stmt.ThenBranch);

            if (stmt.ElseBranch != null)
                Resolve(stmt.ElseBranch);

            return null;
        }


        public object VisitPrintStmt(Stmt.Print stmt)
        {
            Resolve(stmt.ExpressionObject);

            return null;
        }


        public object VisitReturnStmt(Stmt.Return stmt)
        {
            if (_CurrentFunction == FunctionType.NONE)
            {
                cLox1.Error(stmt.Keyword, "Can't return from top-level code.");
            }


            if (stmt.Value != null)
            {
                if (_CurrentFunction == FunctionType.INITIALIZER)
                {
                    cLox1.Error(stmt.Keyword, "Can't return a value from an initializer.");
                }

                Resolve(stmt.Value);
            }


            return null;
        }


        public object VisitVarStmt(Stmt.Var stmt)
        {
            Declare(stmt.Name);

            if (stmt.Initializer != null)
            {
                Resolve(stmt.Initializer);
            }

            Define(stmt.Name);

            return null;
        }


        public object VisitWhileStmt(Stmt.While stmt)
        {
            Resolve(stmt.Condition);
            Resolve(stmt.Body);

            return null;
        }

        // ========================================================================================================================================================================================================

        #endregion




        internal void Resolve(List<Stmt> statements)
        {
            foreach (Stmt statement in statements)
            {
                Resolve(statement);
            }
        }


        private void Resolve(Stmt stmt)
        {
            stmt.Accept(this);
        }


        private void Resolve(Expr expr)
        {
            expr.Accept(this);
        }


        private void ResolveFunction(Stmt.Function function, FunctionType type)
        {
            FunctionType enclosingFunction = _CurrentFunction;
            _CurrentFunction = type;

            BeginScope();

            foreach (Token param in function.Parameters)
            {
                Declare(param);
                Define(param);
            }

            Resolve(function.Body);
            
            EndScope();

            _CurrentFunction = enclosingFunction;
        }


        private void BeginScope()
        {
            _Scopes.Push(new Dictionary<string, bool>());
        }


        private void EndScope()
        {
            _Scopes.Pop();
        }


        private void Declare(Token name)
        {
            if (_Scopes.Count < 1)
                return;

            Dictionary<string, bool> scope = _Scopes.Peek();
            if (scope.ContainsKey(name.Lexeme))
            {
                cLox1.Error(name, "Variable already exists with this name in this scope.");
            }
            else
            {
                scope.Add(name.Lexeme, false);
            }
        }


        private void Define(Token name)
        {
            if (_Scopes.Count < 1)
                return;

            Dictionary<string, bool> scope = _Scopes.Peek();
            if (scope.ContainsKey(name.Lexeme))
            {
                scope[name.Lexeme] = true;
            }
            else
            {
                scope.Add(name.Lexeme, true);
            }
        }


        private void ResolveLocal(Expr expr, Token name)
        {           
            // I had to change the direction this loop iterates, because in C# it is apparently
            // opposite to the direction it is in the book's Java code. This also required
            // changing the call to _Interpreter.Resolve() so the second parameter is i, whereas
            // before it was "_Scopes.Count - 1 - i".
            // Here is the original first line of the loop: for (int i = _Scopes.Count - 1; i >= 0; i--)
            for (int i = 0; i < _Scopes.Count; i++)
            {
                if (_Scopes.ElementAt(i).ContainsKey(name.Lexeme))
                {
                    _Interpreter.Resolve(expr, i);
                    
                    return;
                }
            }
        }


        /// <summary>
        /// Outputs the contents of all scopes for debug purposes.
        /// </summary>
        /// <remarks>
        /// NOTE: You may notice the global scope is missing in the output. This is normal,
        ///       because scopes in the Resolver refer to local scopes that contain variables.
        ///       The Resolver's job is to resolve references to local variables after all.
        /// </remarks>
        internal void Debug_OutputScopes()
        {
            Console.WriteLine("Scope Count: " + _Scopes.Count);
            Console.WriteLine(new string('-', 80));
            for (int i = 0; i < _Scopes.Count; i++)
            {
                foreach (string s in _Scopes.ElementAt(i).Keys)
                {
                    Console.WriteLine("Scope[" + i + "] - " + s);
                }
            }

        }



    }

}
