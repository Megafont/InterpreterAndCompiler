using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    internal class LoxInstance
    {
        private readonly LoxClass _Klass;
        private readonly Dictionary<string, object> _Fields = new Dictionary<string, object>();




        public LoxInstance(LoxClass klass)
        {
            _Klass = klass;
        }




        public object Get(Token name)
        {
            if (_Fields.TryGetValue(name.Lexeme, out object value))
            {
                return value;
            }

            LoxFunction method = _Klass.FindMethod(name.Lexeme);
            if (method != null)
            {
                return method.Bind(this);
            }

            throw new RuntimeError(name, "Undefined property '" + name.Lexeme + "'.");
        }
         

        public void Set(Token name, object value)
        {
            if (_Fields.ContainsKey(name.Lexeme))
            {
                _Fields[name.Lexeme] = value;
            }
            else
            {
                _Fields.Add(name.Lexeme, value);
            }
        }


        public override string ToString()
        {
            return _Klass.Name + " instance";
        }
    }
}
