/*
* ScriptUtils
* By Elliot Hayward
* Public Domain
*/

#ifndef H_SCRIPTUTILS_TYPETRAITS
#define H_SCRIPTUTILS_TYPETRAITS

class asIObjectType;
class asIScriptModule;

namespace ScriptUtils { namespace Inheritance
{

	bool is_base_of(asIObjectType *base, asIObjectType *derived);

	bool is_base_of(asIScriptModule *module, const char * base_name, const char * derived_name);

	//! Returns true if the given type implements the given interface
	//! \todo When asIObjectType lists the typeId, use that to compare
	//! \todo Should this be renamed 'is_interface_of'?
	bool implements(asIObjectType *implementor, asIObjectType *interface);

	//! Returns true if the given type implements the given interface
	bool implements(asIScriptModule *module, const char *implementor_name, const char *interface_name);

	//! Returns true if this type or one of it's bases implements the given interface
	//! \todo Rename this 'is_interface_of_base'?
	bool base_implements(asIObjectType *derived, asIObjectType *interface);

	//! Returns the base class that implements the given interface
	//! \todo Rename this 'get_base_with_interface'
	asIObjectType * get_base_implementor(asIObjectType *derived, asIObjectType *interface);

}}

#endif
