using System;
using System.Collections.Generic;


namespace LoxInterpreter1_TreeWalkInterpreter
{
    internal abstract class Stmt
    {


        internal interface Visitor<R>
        {
                R VisitBlockStmt(Block stmt);
                R VisitClassStmt(Class stmt);
                R VisitExpressionStmt(Expression stmt);
                R VisitFunctionStmt(Function stmt);
                R VisitIfStmt(If stmt);
                R VisitPrintStmt(Print stmt);
                R VisitReturnStmt(Return stmt);
                R VisitVarStmt(Var stmt);
                R VisitWhileStmt(While stmt);
        }


        internal abstract R Accept<R>(Visitor<R> visitor);




        internal class Block : Stmt
        {


            internal Block(List<Stmt> statements)
            {
                this.Statements = statements;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitBlockStmt(this);
            }


            internal readonly List<Stmt> Statements;


        }




        internal class Class : Stmt
        {


            internal Class(Token name, Expr.Variable superClass, List<Stmt.Function> methods)
            {
                this.Name = name;
                this.SuperClass = superClass;
                this.Methods = methods;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitClassStmt(this);
            }


            internal readonly Token Name;
            internal readonly Expr.Variable SuperClass;
            internal readonly List<Stmt.Function> Methods;


        }




        internal class Expression : Stmt
        {


            internal Expression(Expr expressionObject)
            {
                this.ExpressionObject = expressionObject;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitExpressionStmt(this);
            }


            internal readonly Expr ExpressionObject;


        }




        internal class Function : Stmt
        {


            internal Function(Token name, List<Token> parameters, List<Stmt> body)
            {
                this.Name = name;
                this.Parameters = parameters;
                this.Body = body;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitFunctionStmt(this);
            }


            internal readonly Token Name;
            internal readonly List<Token> Parameters;
            internal readonly List<Stmt> Body;


        }




        internal class If : Stmt
        {


            internal If(Expr condition, Stmt thenBranch, Stmt elseBranch)
            {
                this.Condition = condition;
                this.ThenBranch = thenBranch;
                this.ElseBranch = elseBranch;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitIfStmt(this);
            }


            internal readonly Expr Condition;
            internal readonly Stmt ThenBranch;
            internal readonly Stmt ElseBranch;


        }




        internal class Print : Stmt
        {


            internal Print(Expr expressionObject)
            {
                this.ExpressionObject = expressionObject;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitPrintStmt(this);
            }


            internal readonly Expr ExpressionObject;


        }




        internal class Return : Stmt
        {


            internal Return(Token keyword, Expr value)
            {
                this.Keyword = keyword;
                this.Value = value;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitReturnStmt(this);
            }


            internal readonly Token Keyword;
            internal readonly Expr Value;


        }




        internal class Var : Stmt
        {


            internal Var(Token name, Expr initializer)
            {
                this.Name = name;
                this.Initializer = initializer;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitVarStmt(this);
            }


            internal readonly Token Name;
            internal readonly Expr Initializer;


        }




        internal class While : Stmt
        {


            internal While(Expr condition, Stmt body)
            {
                this.Condition = condition;
                this.Body = body;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitWhileStmt(this);
            }


            internal readonly Expr Condition;
            internal readonly Stmt Body;


        }




    }


}
