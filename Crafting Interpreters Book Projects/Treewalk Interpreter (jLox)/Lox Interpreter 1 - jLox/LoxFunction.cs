using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    internal class LoxFunction : LoxCallable
    {
        private readonly Stmt.Function _Declaration;
        private readonly Environment _Closure;

        private readonly bool _IsInitializer;




        public LoxFunction(Stmt.Function declaration, Environment closure, bool isInitializer)
        {
            _IsInitializer = isInitializer;
            _Closure = closure;
            _Declaration = declaration;
        }




        // ========================================================================================================================================================================================================
        // Below are implementations of the methods in the LoxCallable interface.
        // ========================================================================================================================================================================================================

        public int Arity()
        {
            return _Declaration.Parameters.Count;
        }


        public LoxFunction Bind(LoxInstance instance)
        {
            Environment environment = new Environment(_Closure);
            environment.Define("this", instance);

            return new LoxFunction(_Declaration, environment, _IsInitializer);
        }


        public object Call(Interpreter interpreter, List<object> arguments)
        {
            Environment environment = new Environment(_Closure);

            for (int i = 0; i < _Declaration.Parameters.Count; i++)
            {
                environment.Define(_Declaration.Parameters[i].Lexeme,
                                   arguments[i]);
            }

            try
            {
                interpreter.ExecuteBlock(_Declaration.Body, environment);
            }
            catch (Return returnValue)
            {
                if (_IsInitializer)
                {
                    // Initializers (class constuctors) always return 'this' in Lox.
                    return _Closure.GetAt(0, "this");
                }

                return returnValue.Value;
            }


            // Initialiers (constructors) always return 'this' in Lox.
            if (_IsInitializer)
                return _Closure.GetAt(0, "this");


            return null;
        }


        public override string ToString()
        {
            return "<fn " + _Declaration.Name.Lexeme + ">";
        }

        // ========================================================================================================================================================================================================


    }
}
