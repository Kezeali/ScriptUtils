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

	// Old template class (as opposed to templated operator()) based code 
	//template <typename Fn>
	//class Caller {};

	//template <>
	//class Caller<void (void)> : CallingBase {
	//public:
	//	Caller(asIScriptObject * obj, const char * decl)
	//		: CallingBase(obj, decl)
	//	{}
	//
	//	Caller(asIScriptModule *module, const char * decl)
	//		: CallingBase(module, decl)
	//	{}
	//};

	//! Creates a callable object (with templated parameters) for an AngelScript function
	/*!
	* Based on code by SiCrane from gamedev.net, see:
	* http://www.gamedev.net/community/forums/viewreply.asp?ID=3463277
	*/
	class Caller : public CallerBase
	{
	public:
		//! Default constructor
		/*!
		* Provided to allow storing Callers by value in containers.
		* Shouldn't be used otherwise.
		*/
		Caller()
			: CallerBase()
		{}

		//! Constructor: for object methods
		Caller(asIScriptObject * obj, const char * decl)
			: CallerBase(obj, decl)
		{}

		//! Constructor: for global functions
		Caller(asIScriptModule *module, const char * decl)
			: CallerBase(module, decl)
		{}

		//! Creates a caller for a factory fn.
		static Caller FactoryCaller(asIObjectType *type, const std::string &params)
		{
			std::string type_name(type->GetName());
			int factoryId = type->GetFactoryIdByDecl((type_name+"@ "+type_name+"("+params+")").c_str());

			return Caller(type->GetEngine(), factoryId);
		}

		//! Creates a caller for a global fn. for which the ID is known
		static Caller CallerForGlobalFuncId(asIScriptEngine *engine, int funcId)
		{
			return Caller(engine, funcId);
		}

		//! Creates a object method for which the ID is known
		static Caller CallerForMethodFuncId(asIScriptObject *obj, int funcId)
		{
			Caller caller(obj->GetEngine(), funcId);
			caller.set_object(obj);
			return caller;
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
		R operator()(void)
		{
			execute();
			return static_cast< CallHelper<R>* >(return_address())->element;
		}

		//! Used when no return type is specified
		/*
		* Returns pointer to return address. This is null if the
		* script function returns void.
		*/
		void* operator()(void)
		{
			execute();
			return return_address();
		}

#define repeat_set_arg(z, n, text) set_arg(n, a ## n);

#define BOOST_PP_ITERATION_PARAMS_1 (3, (1, SCRIPTCALL_NUMPARAMS, "Caller.h"))
#include BOOST_PP_ITERATE()

#undef repeat_set_arg

	protected:
		Caller(asIScriptEngine *engine, int funcId)
			: CallerBase(engine, funcId)
		{}
	};

}} // namespace

#endif // for H_SCRIPTUTILS_SCRIPTCALLER

#else // for BOOST_PP_IS_ITERATING

#define n BOOST_PP_ITERATION()

		template <typename R, BOOST_PP_ENUM_PARAMS_Z(1 ,n, typename A)>
		R operator() (BOOST_PP_ENUM_BINARY_PARAMS_Z(1, n, A ,a))
		{
			// Calls set_arg(n, an) for each 'n'
			//  ~ does nothing, just junk arg
			//  See set_arg_pprepeat macro at the top of this file
			BOOST_PP_REPEAT(n, repeat_set_arg, ~)
				execute();

			return static_cast< CallHelper<R>* >(return_address())->element;
		}

		template <BOOST_PP_ENUM_PARAMS_Z(1 ,n, typename A)>
		void* operator() (BOOST_PP_ENUM_BINARY_PARAMS_Z(1, n, A ,a))
		{
			// Calls set_arg(n, an) for each 'n'
			//  ~ does nothing, just junk arg
			//  See set_arg_pprepeat macro at the top of this file
			BOOST_PP_REPEAT(n, repeat_set_arg, ~)
				execute();

			return return_address();
		}

#undef n
#endif // for BOOST_PP_IS_ITERATING
