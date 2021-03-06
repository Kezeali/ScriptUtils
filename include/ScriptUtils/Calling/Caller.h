/*
* ScriptUtils
* by Elliot Hayward
* Public Domain
*/

#if !defined(BOOST_PP_IS_ITERATING)

#ifndef H_SCRIPTUTILS_SCRIPTCALLER
#define H_SCRIPTUTILS_SCRIPTCALLER

#include <angelscript.h>

#include "../Exception.h"
#include "CallerBase.h"

#include <boost/preprocessor.hpp>

//! Maximum number of templated parameters for Caller::operator()
#define SCRIPTCALL_NUMPARAMS 16


namespace ScriptUtils { namespace Calling
{

	//! Throws if the given return value of a SetArgX Fn indicates failure
	template <typename T>
	inline void checkSetArgReturn(int r, asUINT arg, const T& obj)
	{
		if (r == asCONTEXT_NOT_PREPARED)
		{
			throw Exception("Caller: Trying to set an argument on an uninitialized script context");
		}
		else if (r == asINVALID_ARG)
		{
			std::string wrongNumOfArgs, rightNumOfArgs;
			if (arg == 0)
			{
				wrongNumOfArgs = "one argument";
				rightNumOfArgs = "no arguments";
			}
			else if (arg == 1)
			{
				wrongNumOfArgs = "2 arguments";
				rightNumOfArgs = "one argument";
			}
			else
			{
				std::ostringstream stream;
				stream << arg+1 << " arguments";
				wrongNumOfArgs = stream.str();
				stream.str("");
				stream << arg << " arguments";
				rightNumOfArgs = stream.str();
			}
			throw Exception("Caller: Was given " + wrongNumOfArgs + " to method taking " + rightNumOfArgs);
		}
		else if (r == asINVALID_TYPE)
		{
			const std::type_info &type = typeid(obj);
			std::ostringstream stream;
			stream << arg;
			throw Exception("Caller: The '" + std::string(type.name()) + "' type object passed as argument " + stream.str() + " is of incorrect type");
		}
	}

	//! Creates a callable object (with templated parameters) for an AngelScript function
	/*!
	* Based on code by SiCrane from gamedev.net, see:
	* http://www.gamedev.net/community/forums/viewreply.asp?ID=3463277
	*/
	class Caller : public CallerBase
	{
		typedef void (Caller::*safe_bool)() const;
    void this_type_does_not_support_comparisons() const {}
	public:
		//! Default constructor
		Caller()
			: CallerBase()
		{}

		//! Copy constructor
		Caller(const Caller& other)
			: CallerBase(other)
		{}

		//! Move constructor
		Caller(Caller&& other)
			: CallerBase(std::move(other))
		{}

		//! Constructor for object methods
		Caller(asIScriptContext *context, asIScriptObject *obj, asIScriptFunction* function)
			: CallerBase(context, obj, function)
		{}

		//! Constructor
		Caller(asIScriptContext *context, asIScriptFunction* function)
			: CallerBase(context, function)
		{}

		//! Creates a caller for a global method
		static Caller Create(asIScriptEngine *engine, const std::string& method_decl)
		{
			auto function = engine->GetGlobalFunctionByDecl(method_decl.c_str());

			return Caller(engine->CreateContext(), function);
		}

		//! Creates a caller for a global method
		static Caller Create(asIScriptModule *module, const std::string& method_decl)
		{
			auto function = module->GetFunctionByDecl(method_decl.c_str());

			return Caller(module->GetEngine()->CreateContext(), function);
		}

		//! Creates a caller for an object method
		static Caller Create(asIScriptObject *object, const std::string& method_decl)
		{
			auto type = object->GetObjectType();
			auto method = type->GetMethodByDecl(method_decl.c_str());

			return Caller(type->GetEngine()->CreateContext(), object, method);
		}

		//! Creates a caller for an object method
		static Caller Create(asIScriptContext* context, asIScriptObject *object, const std::string& method_decl)
		{
			auto type = object->GetObjectType();
			auto method = type->GetMethodByDecl(method_decl.c_str());

			return Caller(context, object, method);
		}

		//! Creates a caller for a factory fn.
		static Caller FactoryCaller(asIObjectType *type, const std::string &params)
		{
			return FactoryCaller(type->GetEngine()->CreateContext(), type, params);
		}

		//! Creates a caller for a factory fn.
		static Caller FactoryCaller(asIScriptContext* ctx, asIObjectType *type, const std::string &params)
		{
			std::string type_name(type->GetName());
			auto factory = type->GetFactoryByDecl((type_name+"@ "+type_name+"("+params+")").c_str());

			return Caller(ctx, factory);
		}

		//! Creates a caller for a global fn. for which the ID is known
		static Caller CallerForGlobalFuncId(asIScriptEngine *engine, int funcId)
		{
			return Caller(engine->CreateContext(), engine->GetFunctionById(funcId));
		}

		//! Creates a object method for which the ID is known
		static Caller CallerForMethodFuncId(asIScriptObject *obj, int funcId)
		{
			Caller caller(obj->GetEngine()->CreateContext(), obj->GetEngine()->GetFunctionById(funcId));
			caller.set_object(obj);
			return caller;
		}

		Caller& operator =(const Caller &other)
		{
			CallerBase::operator=(other);
			return *this;
		}

		Caller& operator =(Caller &&other)
		{
			CallerBase::operator=(std::move(other));
			return *this;
		}

		operator safe_bool() const
		{
			return is_ok() ? &Caller::this_type_does_not_support_comparisons : 0;
		}

		//! Calls the function with no params
		/*!
		* \tparam R
		* The return type. There is no conversion function - stored data
		* must be compatible (i.e. (R)ctx->GetReturnAddress() must be
		* possible).
		*
		* \todo ?ConversionException for return conversion errors - if conversion checking / callbacks are implimented
		*/
		template <typename R>
		R call(void)
		{
			refresh();
			execute();
			return static_cast< CallHelper<R>* >(return_address())->element;
		}

		//! Function-style call
		/*
		* Returns pointer to return address. This is null if the
		* script function returns void.
		*
		* \todo Return 'ScriptReturn' object, with a 'convert<Type>()' method
		*/
		void* operator()(void)
		{
			refresh();
			execute();
			return return_address();
		}

#define repeat_set_arg(z, n, text) checkSetArgReturn(set_arg(n, a ## n), n, a##n);

#define BOOST_PP_ITERATION_PARAMS_1 (3, (1, SCRIPTCALL_NUMPARAMS, "ScriptUtils/Calling/Caller.h"))
#include BOOST_PP_ITERATE()

#undef repeat_set_arg
	};

}} // namespace

#endif // for H_SCRIPTUTILS_SCRIPTCALLER

#else // for BOOST_PP_IS_ITERATING

#define n BOOST_PP_ITERATION()

		template <typename R, BOOST_PP_ENUM_PARAMS_Z(1 ,n, typename A)>
		R call(BOOST_PP_ENUM_BINARY_PARAMS_Z(1, n, A ,a))
		{
			// Prepare the asIScriptContext (does nothing if it is already prepared)
			refresh();
			// Calls set_arg(n, an) for each 'n'
			//  ~ does nothing, just junk arg
			BOOST_PP_REPEAT(n, repeat_set_arg, ~)

			execute();

			return static_cast< CallHelper<R>* >(return_address())->element;
		}

		template <BOOST_PP_ENUM_PARAMS_Z(1 ,n, typename A)>
		void* operator() (BOOST_PP_ENUM_BINARY_PARAMS_Z(1, n, A ,a))
		{
			// Prepare the asIScriptContext (does nothing if it is already prepared)
			refresh();
			// Calls set_arg(n, an) for each 'n'
			//  ~ does nothing, just junk arg
			BOOST_PP_REPEAT(n, repeat_set_arg, ~)

			execute();

			return return_address();
		}

#undef n
#endif // for BOOST_PP_IS_ITERATING
