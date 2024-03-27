using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LoxInterpreter1_TreeWalkInterpreter
{
    internal class Environment
    {
        /// <summary>
        /// Stores a reference to this environment's parent environment.
        /// </summary>
        internal readonly Environment _Enclosing;

        /// <summary>
        /// Stores variables and their values.
        /// </summary>
        internal readonly Dictionary<string, object> Values = new Dictionary<string, object>();



        internal Environment()
        {
            _Enclosing = null;
        }


        internal Environment(Environment enclosing)
        {
            _Enclosing = enclosing;

        }



        /// <summary>
        /// Sets the value of a variable.
        /// </summary>
        /// <param name="name">The name of the variable to set.</param>
        /// <param name="value">The value to set the variable to.</param>
        internal void Assign(Token name, object value)
        {
            if (Values.ContainsKey(name.Lexeme))
            {
                Values[name.Lexeme] = value;
                return;
            }

            if (_Enclosing != null)
            {
                _Enclosing.Assign(name, value);
                return;
            }

            throw new RuntimeError(name, "Undefined variable '" + name.Lexeme + "'.");
        }


        /// <summary>
        /// Sets the value of a non-global variable in the environment n steps above the current one in the hierarchy.
        /// </summary>
        /// <param name="distance">The number of steps up the tree for the environment to access.</param>
        /// <param name="name">The name of the variable.</param>
        /// <param name="value">The value to set the variable to.</param>
        internal void AssignAt(int distance, Token name, object value)
        {
            Environment environment = Ancestor(distance);
            
            if (environment.Values.ContainsKey(name.Lexeme))
            {
                environment.Values[name.Lexeme] = value;
            }
            else
            {
                throw new RuntimeError(name, "Undefined variable '" + name.Lexeme + "'.");
            }
        }


        /// <summary>
        /// This method is the same as Assign(), except that this one will create
        /// the specified variable if it does not exist.
        /// </summary>
        /// <param name="name">The name of the variable to set.</param>
        /// <param name="value">The value to set the variable to.</param>
        internal void Define(string name, object value)
        {
            if (!Values.ContainsKey(name))
                Values.Add(name, value);
            else
                Values[name] = value;
        }


        /// <summary>
        /// Gets the Environment n steps above the current one in the hierarchy.
        /// </summary>
        /// <param name="distance">The number of steps up the tree for the environment to access.</param>
        /// <returns>The environment n steps above the current one in the hierarchy.</returns>
        internal Environment Ancestor(int distance)
        {
            Environment environment = this;

            for (int i = 0; i < distance; i++)
            {
                environment = environment._Enclosing;
            }

            return environment;
        }


        /// <summary>
        /// Retrieves the value of the specified global variable.
        /// </summary>
        /// <param name="name">The name of the variable.</param>
        /// <returns>The value of the variable.</returns>
        internal object Get(Token name)
        {
            if (Values.TryGetValue(name.Lexeme, out object value))
            {
                return value;
            }

            // If the variable was not found in this environment, then search
            // the parent environments of this one all the way up to the global scope.
            if (_Enclosing != null)
                return _Enclosing.Get(name);

            throw new RuntimeError(name,
                                   "Undefined variable '" + name.Lexeme + "'.");
        }


        /// <summary>
        /// Retrieves the value of a non-global variable.
        /// </summary>
        /// <param name="distance">The number of jumps from the current environment to the one the variable was declared in.</param>
        /// <param name="name">The name of the variable.</param>
        /// <returns>The value of the variable.</returns>
        internal object GetAt(int distance, string name)
        {
            return Ancestor(distance).Values[name];
        }


    }



}
