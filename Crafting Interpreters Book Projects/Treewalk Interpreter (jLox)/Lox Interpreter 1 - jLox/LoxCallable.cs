using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    internal interface LoxCallable
    {
        int Arity();
        object Call(Interpreter interpreter, List<object> arguments);
    }

}
