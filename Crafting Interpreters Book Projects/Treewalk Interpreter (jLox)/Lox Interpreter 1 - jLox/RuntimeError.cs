using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    internal class RuntimeError : Exception
    {
        readonly internal Token _Token;


        internal RuntimeError(Token token, string message)
            : base(message)
        {
            this._Token = token;
        }

    }


}
