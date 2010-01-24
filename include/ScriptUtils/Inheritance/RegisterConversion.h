/*
* ScriptUtils
* By Elliot Hayward
* Public Domain
*/

#ifndef H_SCRIPTUTILS_REGISTERCONVERSION
#define H_SCRIPTUTILS_REGISTERCONVERSION

#include <angelscript.h>

#include "../Exception.h"

#include <type_traits>

namespace ScriptUtils { namespace Inheritance
{

	template <class _From, class _To>
	_To * convert_ref(_From * obj) // fn param (so fn. other than 'AddReference' can be used)
	{
		if (obj == NULL)
			return NULL;

		_To* ret = dynamic_cast<_To*>(obj);
		if (ret != NULL)
		{
			ret->AddReference();
		}
		return ret;
	}

	//! Registers conversion operators for two ref. counted application objects
	/*!
	* Both objects must be registered <em>application objects</em>, not script objects.<br>
	*/
	template <class _Base, class _Derived>
	void RegisterBaseOf(asIScriptEngine *engine, const std::string& base, const std::string& derived)
	{
		int r;
		r = engine->RegisterObjectBehaviour(base.c_str(), asBEHAVE_REF_CAST,
			(derived+"@ f()").c_str(), asFUNCTION((convert_ref<_Base, _Derived>)),
			asCALL_CDECL_OBJLAST);

		r = engine->RegisterObjectBehaviour(derived.c_str(), asBEHAVE_IMPLICIT_REF_CAST,
			(base+"@ f()").c_str(), asFUNCTION((convert_ref<_Derived, _Base>)),
			asCALL_CDECL_OBJLAST);
	}

}}

#endif