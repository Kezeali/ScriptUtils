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
	_To * convert_ref(_From * obj) // fn param (so fn. other than 'RemoveReference' can be used)
	{
		if (obj == NULL)
			return NULL;

		_To* ret = dynamic_cast<_To*>(obj);
		if (ret == NULL)
		{
			obj->RemoveReference();
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
		r = engine->RegisterGlobalBehaviour(asBEHAVE_REF_CAST,
			(derived+"@ f("+base+"@)").c_str(), asFUNCTIONPR(convert_ref, (_Base*), _Derived*),
			asCALL_CDECL);

		r = engine->RegisterGlobalBehaviour(asBEHAVE_IMPLICIT_REF_CAST,
			(base+"@ f("+derived+"@)").c_str(), asFUNCTIONPR(convert_ref, (_Derived*), _Base*),
			asCALL_CDECL);
	}

}}

#endif