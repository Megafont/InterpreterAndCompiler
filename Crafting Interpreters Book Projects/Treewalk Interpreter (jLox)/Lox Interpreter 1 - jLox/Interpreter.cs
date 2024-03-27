using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    internal class Interpreter: Expr.Visitor<object>,
                                Stmt.Visitor<object>
    {
        /// <summary>
        /// Stores information about variables defined by the Lox script we're interpreting.
        /// </summary>
        internal readonly Environment _Globals = new Environment();        
        private Environment _Environment;


        private readonly Dictionary<Expr, int> _Locals = new Dictionary<Expr, int>();




        internal Interpreter()
        {
            _Environment = _Globals;

            _Globals.Define("clock", new Native_Functions.Clock());
        }




        /// <summary>
        /// Interprets the passed in expression.
        /// </summary>
        /// <param name="expression">The expression to interpret.</param>
        internal void Interpret(List<Stmt> statements)
        {
            try
            {
                foreach (Stmt statement in statements)
                {
                    Execute(statement);
                }
            }
            catch (RuntimeError error)
            {
                cLox1.RuntimeError(error);
            }
        }




        #region Interpreting Expressions

        // ========================================================================================================================================================================================================
        // Below are implementations of the methods in the Expr.Visitor interface.
        // ========================================================================================================================================================================================================

        public object VisitAssignExpr(Expr.Assign expr)
        {
            object value = Evaluate(expr.Value);

            if (_Locals.TryGetValue(expr, out int distance))
            {
                _Environment.AssignAt(distance, expr.Name, value);
            }
            else
            {
                _Globals.Assign(expr.Name, value);
            }
            
            return value;
        }


        public object VisitBinaryExpr(Expr.Binary expr)
        {
            object left = Evaluate(expr.Left);
            object right = Evaluate(expr.Right);

            switch (expr.Operation.Type)
            {
                case TokenType.GREATER:
                    CheckNumberOperands(expr.Operation, left, right);
                    return (double)left > (double)right;
                case TokenType.GREATER_EQUAL:
                    CheckNumberOperands(expr.Operation, left, right);
                    return (double)left >= (double)right;
                case TokenType.LESS:
                    CheckNumberOperands(expr.Operation, left, right);
                    return (double)left < (double)right;
                case TokenType.LESS_EQUAL:
                    CheckNumberOperands(expr.Operation, left, right);
                    return (double)left <= (double)right;
                case TokenType.BANG_EQUAL:
                    return !IsEqual(left, right);
                case TokenType.EQUAL_EQUAL:
                    return IsEqual(left, right);


                case TokenType.MINUS:
                    CheckNumberOperands(expr.Operation, left, right);
                    return (double)left - (double)right;
                case TokenType.PLUS:
                    // Check if we should we do numeric addition.
                    if ((left is double) && (right is double))
                        return (double)left + (double)right;
                    
                    // Check if we should do string concatenation.
                    if ((left is string) && (right is string))
                        return (string)left + (string)right;
                    
                    throw new RuntimeError(expr.Operation, "Operands must be two numbers or two strings.");

                case TokenType.SLASH:
                    CheckNumberOperands(expr.Operation, left, right);
                    return (double)left / (double)right;
                case TokenType.STAR:
                    CheckNumberOperands(expr.Operation, left, right);
                    return (double)left * (double)right;
            }// end switch


            // Unreachable
            return null;

        }


        public object VisitCallExpr(Expr.Call expr)
        {
            object callee = Evaluate(expr.Callee);


            List<object> arguments = new List<object>();
            foreach (Expr argument in expr.Arguments)
            {
                arguments.Add(Evaluate(argument));
            }


            if (!(callee is LoxCallable))
            {
                throw new RuntimeError(expr.Paren, "Can only call functions and classes.");
            }


            LoxCallable function = (LoxCallable) callee;

            if (arguments.Count != function.Arity())
            {
                throw new RuntimeError(expr.Paren, "Expected " + function.Arity() + " function arguments but got " + arguments.Count + ".");
            }



            return function.Call(this, arguments);
        }


        public object VisitGetExpr(Expr.Get expr)
        {
            object obj = Evaluate(expr.ClassInstance);
            if (obj is LoxInstance)
            {
                return ((LoxInstance)obj).Get(expr.Name);
            }

            throw new RuntimeError(expr.Name, "Only instances have properties.");
        }


        public object VisitGroupingExpr(Expr.Grouping expr)
        {
            return Evaluate(expr.ExpressionObject);
        }


        public object VisitLiteralExpr(Expr.Literal expr)
        {
            return expr.Value;
        }


        public object VisitLogicalExpr(Expr.Logical expr)
        {
            object left = Evaluate(expr.Left);

            if (expr.Operation.Type == TokenType.OR)
            {
                if (IsTruthy(left))
                {
                    return left;
                }
                else
                {
                    if (!IsTruthy(left))
                    {
                        return left;
                    }
                }
            }

            return Evaluate(expr.Right);
        }


        public object VisitSetExpr(Expr.Set expr)
        {
            object obj = Evaluate(expr.ClassInstance);

            if (!(obj is LoxInstance))
            {
                throw new RuntimeError(expr.Name, "Only instances have fields.");
            }

            object value = Evaluate(expr.Value);
            ((LoxInstance) obj).Set(expr.Name, value);
            return value;
        }


        public object VisitSuperExpr(Expr.Super expr)
        {
            int distance = _Locals[expr];

            LoxClass superClass = (LoxClass)_Environment.GetAt(distance, "super");

            LoxInstance obj = (LoxInstance)_Environment.GetAt(distance - 1, "this");

            LoxFunction method = superClass.FindMethod(expr.Method.Lexeme);

            if (method == null)
            {
                throw new RuntimeError(expr.Method, "Undefined property '" + expr.Method.Lexeme + "'.");
            }

            return method.Bind(obj);
        }


        public object VisitThisExpr(Expr.This expr)
        {
            return LookUpVariable(expr.Keyword, expr);
        }


        public object VisitUnaryExpr(Expr.Unary expr)
        {
            object right = Evaluate(expr.Right);

            switch (expr.Operation.Type)
            {
                case TokenType.BANG:
                    return !IsTruthy(right);
                case TokenType.MINUS:
                    CheckNumberOperand(expr.Operation, right);
                    return -(double)right;

            } // end switch


            // Unreachable
            return null;

        }


        public object VisitVariableExpr(Expr.Variable expr)
        {
            return LookUpVariable(expr.Name, expr);
        }

        // ========================================================================================================================================================================================================

        #endregion




        #region Interpreting Statements

        // ========================================================================================================================================================================================================
        // Below are implementations of the statements in the Stmt.Visitor interface.
        // ========================================================================================================================================================================================================

        public object VisitBlockStmt(Stmt.Block stmt)
        {
            ExecuteBlock(stmt.Statements, new Environment(_Environment));

            return null;
        }


        public object VisitClassStmt(Stmt.Class stmt)
        {
            object superClass = null;

            if (stmt.SuperClass != null)
            {
                superClass = Evaluate(stmt.SuperClass);
                if (!(superClass is LoxClass))
                {
                    throw new RuntimeError(stmt.SuperClass.Name, "Superclass must be a class.");
                }
            }

            _Environment.Define(stmt.Name.Lexeme, null);

            if (stmt.SuperClass != null)
            {
                _Environment = new Environment(_Environment);
                _Environment.Define("super", superClass);
            }

            Dictionary<string, LoxFunction> methods = new Dictionary<string, LoxFunction>();
            foreach (Stmt.Function method in stmt.Methods)
            {
                LoxFunction function = new LoxFunction(method, 
                                                       _Environment,
                                                       method.Name.Lexeme == "init");

                methods.Add(method.Name.Lexeme, function);
            }

            LoxClass klass = new LoxClass(stmt.Name.Lexeme, (LoxClass)superClass, methods);

            if (superClass != null)
                _Environment = _Environment._Enclosing;

            _Environment.Assign(stmt.Name, klass);

            return null;
        }


        public object VisitExpressionStmt(Stmt.Expression stmt)
        {
            Evaluate(stmt.ExpressionObject);
            
            return null;
        }


        public object VisitFunctionStmt(Stmt.Function stmt)
        {
            LoxFunction function = new LoxFunction(stmt, _Environment, false);
            _Environment.Define(stmt.Name.Lexeme, function);
            return null;
        }


        public object VisitIfStmt(Stmt.If stmt)
        {
            if (IsTruthy(Evaluate(stmt.Condition)))
            {
                Execute(stmt.ThenBranch);
            }
            else if (stmt.ElseBranch != null)
            {
                Execute(stmt.ElseBranch);
            }

            return null;
        }


        public object VisitPrintStmt(Stmt.Print stmt)
        {
            object value = Evaluate(stmt.ExpressionObject);
            Console.WriteLine(Stringify(value));
            
            return null;
        }


        public object VisitReturnStmt(Stmt.Return stmt)
        {
            object value = null;

            if (stmt.Value != null)
                value = Evaluate(stmt.Value);

            throw new Return(value);
        }


        public object VisitVarStmt(Stmt.Var stmt)
        {
            object value = null;
            if (stmt.Initializer != null)
            {
                value = Evaluate(stmt.Initializer);
            }

            _Environment.Define(stmt.Name.Lexeme, value);

            return null;
        }


        public object VisitWhileStmt(Stmt.While stmt)
        {
            while (IsTruthy(Evaluate(stmt.Condition)))
            {
                Execute(stmt.Body);
            }

            return null;
        }

        // ========================================================================================================================================================================================================

        #endregion




        /// <summary>
        /// Checks if the operand of a unary operator is the correct data type.
        /// </summary>
        /// <param name="operation">The operator being applied to the operand.</param>
        /// <param name="operand">The operand value.</param>
        private void CheckNumberOperand(Token operation, object operand)
        {
            if (operand is double)
                return;

            throw new RuntimeError(operation, "Operand must be a number.");
        }


        /// <summary>
        /// Checks if the operands of a binary operator are the correct data type.
        /// </summary>
        /// <param name="operation">The operator being applied to the operands.</param>
        /// <param name="left">The value to the left of the operator.</param>
        /// <param name="right">The value to the right of the operator.</param>
        private void CheckNumberOperands(Token operation, object left, object right)
        {
            if (left is double && right is double)
                return;

            throw new RuntimeError(operation, "Operands must be numbers.");
        }


        /// <summary>
        /// Evaluates an expression by passing a reference to this class into the Expr object.
        /// The Expr object knows the correct visitor method to call based on which type of expression it is.
        /// </summary>
        /// <param name="expr">The expression to evaluate.</param>
        /// <returns>The value the passed in expression evaluated to.</returns>
        private object Evaluate(Expr expr)
        {
            return expr.Accept(this);
        }


        /// <summary>
        /// Executes a statement by passing a reference to this class into the Stmt object.
        /// The Stmt object knows the correct visitor method to call based on which type of statement it is.
        /// </summary>
        /// <param name="stmt">The statement to execute.</param>
        private void Execute(Stmt stmt)
        {
            stmt.Accept(this);
        }


        /// <summary>
        /// Executes a group of statements that are inside a block.
        /// </summary>
        /// <param name="statements">The list of statements in the block.</param>
        /// <param name="environment">The environment for the block.</param>
        internal void ExecuteBlock(List<Stmt> statements, Environment environment)
        {
            Environment previous = _Environment;

            try
            {
                _Environment = environment;

                foreach (Stmt statement in statements)
                {
                    Execute(statement);
                }
            }
            finally
            {
                _Environment = previous;
            }
        }


        internal void Resolve(Expr expr, int depth)
        {
            if (_Locals.ContainsKey(expr))
            {
                _Locals[expr] = depth;
            }
            else
            {
                _Locals.Add(expr, depth);
            }
        }


        private object LookUpVariable(Token name, Expr expr)
        {
            if (_Locals.TryGetValue(expr, out int distance))
            {
                return _Environment.GetAt(distance, name.Lexeme);
            }
            else
            {
                return _Globals.Get(name);
            }
            
        }


        /// <summary>
        /// Checks if two objects in our Lox programming language are considered equal or not.
        /// </summary>
        /// <param name="a">The first object.</param>
        /// <param name="b">The second object.</param>
        /// <returns>True if the objects are considered equal or false otherwise.</returns>
        private bool IsEqual(object a, object b)
        {
            if (a == null && b == null)
                return true;
            if (a == null)
                return false;

            return a.Equals(b);
        }


        /// <summary>
        /// Checks if the specified value evaluates to true or false in our Lox programming language.
        /// </summary>
        /// <param name="obj">The value to check.</param>
        /// <returns>The boolean equivalent of the passed in value.</returns>
        private bool IsTruthy(object obj)
        {
            if (obj == null)
                return false;
            if (obj is bool)
                return (bool) obj;

            return true;
        }


        /// <summary>
        /// Converts a value in the Lox language to a string.
        /// </summary>
        /// <param name="obj">The value to "stringify".</param>
        /// <returns>The passed in value converted to a string.</returns>
        private string Stringify(object obj)
        {
            if (obj == null)
                return "nil";


            if (obj is double)
            {
                string text = obj.ToString();

                // This if statement may be irrelevant, as it is Java specific.
                if (text.EndsWith(".0"))
                {
                    text = text.Substring(0, text.Length - 2);
                }

                return text;
            }


            return obj.ToString();
        }



    }


}
