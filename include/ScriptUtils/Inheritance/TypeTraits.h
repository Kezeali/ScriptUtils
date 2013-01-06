/*
* ScriptUtils
* By Elliot Hayward
* Public Domain
*/

#ifndef H_SCRIPTUTILS_TYPETRAITS
#define H_SCRIPTUTILS_TYPETRAITS

#include <angelscript.h>

namespace ScriptUtils { namespace Inheritance
{

	inline bool is_base_of(asIObjectType *base, asIObjectType *derived)
	{
		return derived->DerivesFrom(base);
	}

	inline bool is_base_of(asIScriptModule *module, const char * base_name, const char * derived_name)
	{
		asIScriptEngine *engine = module->GetEngine();
		asIObjectType *base = engine->GetObjectTypeById( module->GetTypeIdByDecl(base_name) );
		if (base == NULL)
			return false;
		asIObjectType *derived = engine->GetObjectTypeById( module->GetTypeIdByDecl(derived_name) );

		return is_base_of(base, derived);
	}

	//! Returns true if the given type implements the given interface
	//! \todo Should this be renamed 'is_interface_of'?
	inline bool implements(asIObjectType *implementor, asIObjectType *iface)
	{
		return implementor->Implements(iface);
	}

	//! Returns true if the given type implements the given interface
	inline bool implements(asIScriptModule *module, const char *implementor_name, const char *interface_name)
	{
		asIScriptEngine *engine = module->GetEngine();

		asIObjectType *implementor = engine->GetObjectTypeById( module->GetTypeIdByDecl(implementor_name) );
		if (implementor == NULL)
			return false;
		asIObjectType *iface = engine->GetObjectTypeById( module->GetTypeIdByDecl(interface_name) );
		if (iface == NULL)
			return false;

		return implements(implementor, iface);
	}

	//! Returns true if this type or one of it's bases implements the given interface
	//! \todo Rename this 'is_interface_of_base'?
	inline bool base_implements(asIObjectType *derived, asIObjectType *iface)
	{
		asIObjectType *baseType = derived;
		while (baseType)
		{
			if (implements(baseType, iface))
				return true;

			baseType = baseType->GetBaseType();
		}

		return false;
	}

	//! Returns the base class that implements the given interface
	//! \todo Rename this 'get_base_with_interface'
	inline asIObjectType* get_base_implementor(asIObjectType *derived, asIObjectType *iface)
	{
		asIObjectType *baseType = derived;
		while (baseType)
		{
			if (implements(baseType, iface))
				return baseType;

			baseType = baseType->GetBaseType();
		}

		return NULL;
	}

}}

#endif
