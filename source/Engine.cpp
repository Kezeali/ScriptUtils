/*
* ScriptUtils
* By Elliot Hayward
*/

#include "../include/Engine/Engine.h"

namespace ScriptUtils
{

	void Engine::RegisterGlobalFunction(const char *decl, asSFuncPtr fn, asDWORD call_conv)
	{
		m_Engine->RegisterGlobalFunction(decl, fn, call_conv);

		RegistrationEvent ev;
		RegistrationEvent::ForGlobalFunction(ev, decl, fn, call_conv);
		OnRegisteredGlobalFunction(ev);
	}

}