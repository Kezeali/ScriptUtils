/*
* ScriptUtils
* By Elliot Hayward
* Public Domain
*/

#include <angelscript.h>

//#include "../include/ScriptUtils/Exception.h"
#include "../include/ScriptUtils/Inheritance/TypeTraits.h"


namespace ScriptUtils { namespace Inheritance
{

	bool is_base_of(asIObjectType *base, asIObjectType *derived)
	{
		asIObjectType *baseType = derived;
		while (baseType != NULL)
		{
			// See whether the base type of derrived is the type we are looking for
			if (baseType == base)
				return true;

			baseType = baseType->GetBaseType();
		}

		return false;
	}

	bool is_base_of(asIScriptModule *module, const char * base_name, const char * derived_name)
	{
		asIScriptEngine *engine = module->GetEngine();
		asIObjectType *base = engine->GetObjectTypeById( module->GetTypeIdByDecl(base_name) );
		if (base == NULL)
			return false;
		asIObjectType *derived = engine->GetObjectTypeById( module->GetTypeIdByDecl(derived_name) );

		return is_base_of(base, derived);
	}

	bool implements(asIObjectType *implementor, asIObjectType *interface)
	{
		return implementor->Implements(interface);
	}

	bool implements(asIScriptModule *module, const char *implementor_name, const char *interface_name)
	{
		asIScriptEngine *engine = module->GetEngine();

		asIObjectType *implementor = engine->GetObjectTypeById( module->GetTypeIdByDecl(implementor_name) );
		if (implementor == NULL)
			return false;
		asIObjectType *interface = engine->GetObjectTypeById( module->GetTypeIdByDecl(interface_name) );
		if (interface == NULL)
			return false;

		return implements(implementor, interface);
	}

	bool base_implements(asIObjectType *derived, asIObjectType *interface)
	{
		asIObjectType *baseType = derived;
		while (baseType)
		{
			if (implements(baseType, interface))
				return true;

			baseType = baseType->GetBaseType();
		}

		return false;
	}

	asIObjectType* get_base_implementor(asIObjectType *derived, asIObjectType *interface)
	{
		asIObjectType *baseType = derived;
		while (baseType)
		{
			if (implements(baseType, interface))
				return baseType;

			baseType = baseType->GetBaseType();
		}

		return NULL;
	}

}}
