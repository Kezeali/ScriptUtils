/*
* ScriptUtils
* By Elliot Hayward
* Public Domain
*/

#ifndef H_SCRIPTUTILS_ENGINEWRAPPER
#define H_SCRIPTUTILS_ENGINEWRAPPER

#include <angelscript.h>
#include <boost/signals2/signal.hpp>


namespace ScriptUtils
{

	//! Passed to event listeners whenever Engine#RegisterX() is called
	class RegistrationEvent
	{
	public:
		enum RegEventType
		{
			None,

			Enum, Interface, ObjectType,

			EnumValue,

			GlobalFunction, GlobalBehaviour, GlobalProperty,

			InterfaceMethod,

			ObjectMethod, ObjectBehaviour, ObjectProperty,

			StringFactory,

			Typedef
		};

	public:
		RegistrationEvent()
			: event_type(None)
		{}

		static void ForGlobalFunction(RegistrationEvent &out, const std::string &decl, asSFuncPtr fn_ptr, asDWORD call_conv)
		{
			out.event_type = GlobalFunction;
			out.declaraion = decl;
			out.function_ptr = fn_ptr;
			out.calling_convention = call_conv;
		}

		static void ForGlobalBehaviour(RegistrationEvent &out, asEBehaviours behave, const std::string &decl, asSFuncPtr fn_ptr, asDWORD call_conv)
		{
			out.event_type = GlobalBehaviour;
			out.behaviour = behave;
			out.declaraion = decl;
			out.function_ptr = fn_ptr;
			out.calling_convention = call_conv;
		}

	public:
		RegistrationEvent(const std::string &decl, asSFuncPtr fn_ptr, asDWORD call_conv)
			: event_type(GlobalFunction),
			declaraion(decl),
			function_ptr(fn_ptr),
			calling_convention(call_conv)
		{
		}

	public:
		std::string object_type_name;
		asEBehaviours behaviour;
		std::string declaraion;
		asSFuncPtr function_ptr;
		asDWORD calling_convention;

		RegEventType event_type;

	};

	//! Wrapps the AngelScript engine with expanded C++ behaviour
	class Engine
	{
	public:
		Engine(asIScriptEngine *engine)
			: m_Engine(engine)
		{
		}

		void RegisterGlobalFunction(const char *decl, asSFuncPtr fn, asDWORD call_conv);

		boost::signals2::signal<void (const RegistrationEvent &)> OnRegisteredGlobalFunction;

	private:
		asIScriptEngine *m_Engine;
	};

}

#endif
