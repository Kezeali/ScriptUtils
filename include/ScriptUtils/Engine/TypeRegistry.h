/*
* ScriptUtils
* By Elliot Hayward
* Public Domain
*/

#ifndef H_SCRIPTUTILS_TYPEREGISTRY
#define H_SCRIPTUTILS_TYPEREGISTRY

#include <angelscript.h>

#include <unordered_map>


namespace ScriptUtils
{

	struct TypeRegistry
	{
		typedef std::unordered_map<std::type_info, int> registry_type;
		registry_type _Registry;

		template <class T>
		void RegisterType(int typeId)
		{
			// Link the C++ class type to the given script type
			_Registry.insert( registry_type::value_type(typeid(T), typeId) );
		}

		template <class T>
		int GetType()
		{
			registry_type::iterator _where = _Registry.find(typeid(t));
			if (_where != _Registry.end())
			{
				return _where->second;
			}
			else
				return -1;
		}

		template <class T>
		bool IsRegisteredAs(int typeId)
		{
			registry_type::iterator _where = _Registry.find(typeid(t));
			if (_where != _Registry.end())
			{
				return _where->second == typeId;
			}
			else
				return false;
		}

	};

}

#endif
