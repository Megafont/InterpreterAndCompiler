using System;
using System.Collections.Generic;


namespace LoxInterpreter1_TreeWalkInterpreter
{
    internal abstract class Expr
    {


        internal interface Visitor<R>
        {
                R VisitAssignExpr(Assign expr);
                R VisitBinaryExpr(Binary expr);
                R VisitCallExpr(Call expr);
                R VisitGetExpr(Get expr);
                R VisitGroupingExpr(Grouping expr);
                R VisitLiteralExpr(Literal expr);
                R VisitLogicalExpr(Logical expr);
                R VisitSetExpr(Set expr);
                R VisitSuperExpr(Super expr);
                R VisitThisExpr(This expr);
                R VisitUnaryExpr(Unary expr);
                R VisitVariableExpr(Variable expr);
        }


        internal abstract R Accept<R>(Visitor<R> visitor);




        internal class Assign : Expr
        {


            internal Assign(Token name, Expr value)
            {
                this.Name = name;
                this.Value = value;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitAssignExpr(this);
            }


            internal readonly Token Name;
            internal readonly Expr Value;


        }




        internal class Binary : Expr
        {


            internal Binary(Expr left, Token operation, Expr right)
            {
                this.Left = left;
                this.Operation = operation;
                this.Right = right;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitBinaryExpr(this);
            }


            internal readonly Expr Left;
            internal readonly Token Operation;
            internal readonly Expr Right;


        }




        internal class Call : Expr
        {


            internal Call(Expr callee, Token paren, List<Expr> arguments)
            {
                this.Callee = callee;
                this.Paren = paren;
                this.Arguments = arguments;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitCallExpr(this);
            }


            internal readonly Expr Callee;
            internal readonly Token Paren;
            internal readonly List<Expr> Arguments;


        }




        internal class Get : Expr
        {


            internal Get(Expr classInstance, Token name)
            {
                this.ClassInstance = classInstance;
                this.Name = name;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitGetExpr(this);
            }


            internal readonly Expr ClassInstance;
            internal readonly Token Name;


        }




        internal class Grouping : Expr
        {


            internal Grouping(Expr expressionObject)
            {
                this.ExpressionObject = expressionObject;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitGroupingExpr(this);
            }


            internal readonly Expr ExpressionObject;


        }




        internal class Literal : Expr
        {


            internal Literal(Object value)
            {
                this.Value = value;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitLiteralExpr(this);
            }


            internal readonly Object Value;


        }




        internal class Logical : Expr
        {


            internal Logical(Expr left, Token operation, Expr right)
            {
                this.Left = left;
                this.Operation = operation;
                this.Right = right;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitLogicalExpr(this);
            }


            internal readonly Expr Left;
            internal readonly Token Operation;
            internal readonly Expr Right;


        }




        internal class Set : Expr
        {


            internal Set(Expr classInstance, Token name, Expr value)
            {
                this.ClassInstance = classInstance;
                this.Name = name;
                this.Value = value;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitSetExpr(this);
            }


            internal readonly Expr ClassInstance;
            internal readonly Token Name;
            internal readonly Expr Value;


        }




        internal class Super : Expr
        {


            internal Super(Token keyword, Token method)
            {
                this.Keyword = keyword;
                this.Method = method;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitSuperExpr(this);
            }


            internal readonly Token Keyword;
            internal readonly Token Method;


        }




        internal class This : Expr
        {


            internal This(Token keyword)
            {
                this.Keyword = keyword;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitThisExpr(this);
            }


            internal readonly Token Keyword;


        }




        internal class Unary : Expr
        {


            internal Unary(Token operation, Expr right)
            {
                this.Operation = operation;
                this.Right = right;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitUnaryExpr(this);
            }


            internal readonly Token Operation;
            internal readonly Expr Right;


        }




        internal class Variable : Expr
        {


            internal Variable(Token name)
            {
                this.Name = name;
            }


            internal override R Accept<R>(Visitor<R> visitor)
            {
                return visitor.VisitVariableExpr(this);
            }


            internal readonly Token Name;


        }




    }


}
