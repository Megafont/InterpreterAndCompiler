using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    internal class LoxClass : LoxCallable
    {
        public readonly string Name;
        public readonly LoxClass SuperClass;

        private readonly Dictionary<string, LoxFunction> _Methods;



        public LoxClass(string name, LoxClass superClass, Dictionary<string, LoxFunction> methods)
        {
            this.SuperClass = superClass;
            this.Name = name;
            this._Methods = methods;
        }




        public int Arity()
        {
            // Get this Lox class' constructor (if there is one).
            LoxFunction initializer = FindMethod("init");
            if (initializer == null)
                return 0;

            return initializer.Arity();
        }


        public object Call(Interpreter interpreter, List<object> arguments)
        {
            LoxInstance instance = new LoxInstance(this);

            // Get this Lox class' constructor (if there is one), and call it.
            LoxFunction initializer = FindMethod("init");
            if (initializer != null)
                initializer.Bind(instance).Call(interpreter, arguments);


            return instance;
        }


        public LoxFunction FindMethod(string name)
        {
            if (_Methods.TryGetValue(name, out LoxFunction value))
            {
                return value;
            }

            if (SuperClass != null)
            {
                return SuperClass.FindMethod(name);
            }

            return null;
        }




        public override string ToString()
        {
            return Name;
        }

    }
}
