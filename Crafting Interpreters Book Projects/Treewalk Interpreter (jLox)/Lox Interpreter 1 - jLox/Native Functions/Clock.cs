using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter.Native_Functions
{
    internal class Clock : LoxCallable
    {
        public int Arity() { return 0; }

        public object Call(Interpreter interpreter,
                           List<Object> arguments)
        {
            long milliseconds = DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond;
            return milliseconds / 1000.0;
        }

        public override string ToString() { return "<native fn 'Clock'>"; }

    }
}
