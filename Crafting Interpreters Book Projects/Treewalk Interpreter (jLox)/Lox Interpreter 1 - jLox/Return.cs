using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    internal class Return : Exception
    {
        internal readonly object Value;



        internal Return(object value)
        {
            Value = value;   
        }

    }

}
