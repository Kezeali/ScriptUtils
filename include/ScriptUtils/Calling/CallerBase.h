/*
* ScriptUtils
* By Elliot Hayward
* Public Domain
*/

#ifndef H_SCRIPTUTILS_CALLERBASE
#define H_SCRIPTUTILS_CALLERBASE

#include <angelscript.h>

#include "../Exception.h"

#include <boost/signals2/signal.hpp>
#include <boost/function.hpp>
#include <memory>
#include <sstream>


namespace ScriptUtils { namespace Calling
{

	//! Converts data
	/*!
	* From SiCrane's code, not exactly sure why this was
	* used over any other type-casting method... But I
	* trust it.
	*/
	template <typename T>
	struct CallHelper
	{
		T element;
		CallHelper(T t) : element(t) {}
	};

	//class CallerContextCallbacks
	//{
	//public:
	//	CallerContextCallbacks(CallerBase *target)
	//		: m_Targer(target)
	//	{}
	//	//! Default line callback fn. - Fires LineSignal
	//	void LineCallback(asIScriptContext *ctx);
	//	//! Default exception callback fn. - Fires ScriptExceptionSignal
	//	void ExceptionCallback(asIScriptContext *ctx);

	//private:
	//	CallerBase *m_Targer;
	//};

	void CallerLineCallback(asIScriptContext *ctx, void *obj);

	void CallerExceptionCallback(asIScriptContext *ctx, void *obj);

	//! Base class for callers
	class CallerBase
	{
	public:
		//! Default constructor
		/*!
		* Constructs an empty Caller
		*/
		CallerBase()
			: ok(false), obj(NULL), funcId(-1), ctx(NULL),
			throwOnException(false),
			ScriptExceptionSignal(new exception_signal),
			LineSignal(new line_signal)
		{
		}

		//! Constructor for object methods
		CallerBase(asIScriptObject *obj, const char *decl)
			: obj(obj), decl(decl), ok(true),
			throwOnException(false),
			ScriptExceptionSignal(new exception_signal),
			LineSignal(new line_signal)
		{
			asIScriptEngine *engine = obj->GetEngine();

			ctx = engine->CreateContext();

			asIObjectType *type = obj->GetObjectType();
			funcId = type->GetMethodIdByDecl(decl);
			check_asreturn(funcId);

			check_asreturn( ctx->Prepare(funcId) );
			check_asreturn( ctx->SetObject(obj) );
			//obj->AddRef();
		}

		//! Constructor for global methods (functions)
		CallerBase(asIScriptModule *module, const char *decl)
			: obj(NULL), decl(decl), ok(true),
			throwOnException(false),
			ScriptExceptionSignal(new exception_signal),
			LineSignal(new line_signal)
		{
			asIScriptEngine *engine = module->GetEngine();

			ctx = engine->CreateContext();

			funcId = module->GetFunctionIdByDecl(decl);
			check_asreturn(funcId);

			check_asreturn( ctx->Prepare(funcId) );
		}

		//! Constructor for global methods (functions)
		CallerBase(asIScriptEngine *engine, int funcId)
			: obj(NULL), funcId(funcId), ok(true),
			throwOnException(false),
			ScriptExceptionSignal(new exception_signal),
			LineSignal(new line_signal)
		{
			ctx = engine->CreateContext();

			if (check_asreturn(funcId))
				decl = engine->GetFunctionDescriptorById(funcId)->GetDeclaration();

			check_asreturn( ctx->Prepare(funcId) );
		}

		//! Copy constructor
		CallerBase(const CallerBase &other)
			: ctx(other.ctx),
			obj(other.obj),
			decl(other.decl),
			funcId(other.funcId),
			ok(other.ok),
			throwOnException(other.throwOnException),
			LineSignal(other.LineSignal),
			ScriptExceptionSignal(other.ScriptExceptionSignal)
		{
			if (other.is_ok())
			{
				ctx = other.ctx->GetEngine()->CreateContext();

				if (ctx == nullptr)
					ok = false;
				else
				{
					check_asreturn( ctx->Prepare(funcId) );
					if (obj != nullptr)
						check_asreturn( ctx->SetObject(obj) );
				}
			}
		}

		//! Move constructor
		CallerBase(CallerBase &&other)
			: ctx(other.ctx),
			obj(other.obj),
			decl(std::move(other.decl)),
			funcId(other.funcId),
			ok(other.ok),
			throwOnException(other.throwOnException),
			LineSignal(std::move(other.LineSignal)),
			ScriptExceptionSignal(std::move(other.ScriptExceptionSignal))
		{
			other.ctx = nullptr;
			other.obj = nullptr;
		}

		//! Destructor
		~CallerBase()
		{
			release();
		}

		//! Copy-assignement operator
		CallerBase& operator= (const CallerBase &other)
		{
			// Release this object's reference to the context & script object;
			//  ctx is about to be copied from the other caller
			release();

			decl = other.decl;
			obj = other.obj;
			funcId = other.funcId;

			ok = other.ok;

			if (other.is_ok())
			{
				ctx = other.ctx->GetEngine()->CreateContext();

				if (ctx == nullptr)
					ok = false;
				else
				{
					check_asreturn( ctx->Prepare(funcId) );
					if (obj != nullptr)
						check_asreturn( ctx->SetObject(obj) );
				}
			}

			// Signal ptrs
			LineSignal = other.LineSignal;
			ScriptExceptionSignal = other.ScriptExceptionSignal;

			throwOnException = other.throwOnException;

			return *this;
		}

		//! Move-assignement operator
		CallerBase& operator= (CallerBase &&other)
		{
			// Release this object's reference to the context & script object;
			//  ctx is about to be taken from the other caller
			release();

			// Take the other caller's ctx
			std::swap(ctx, other.ctx);

			std::swap(obj, other.obj);

			decl = std::move(other.decl);
			funcId = other.funcId;
			ok = other.ok;

			// Signal ptrs
			LineSignal = std::move(other.LineSignal);
			ScriptExceptionSignal = std::move(other.ScriptExceptionSignal);

			throwOnException = other.throwOnException;

			return *this;
		}

		//! Releases the internal asIScriptContext
		void release()
		{
			if (ctx != nullptr)
			{
				bool heldObject = false;
				if (ctx->GetState() == asEXECUTION_PREPARED && obj != NULL)
				{
					//obj->AddRef();
					ctx->SetObject(NULL); // Temporarily remove the object from the ctx
					heldObject = true;
				}
				if (ctx->Release() > 0 && heldObject)
				{
					// If this caller wasn't the last reference to the ctx, re-set the object
					ctx->SetObject(obj);
					//obj->Release();
				}
				ctx = nullptr;
				obj = nullptr;
				ok = false;
			}
		}

		//! Returns true if this caller is prepared to execute
		bool is_ok() const
		{
			return ok;
		}

		bool refresh()
		{
			if (ctx != nullptr && ctx->GetState() != asEXECUTION_PREPARED)
			{
				asIScriptEngine *engine = ctx->GetEngine();
				ctx->Release();
				ctx = engine->CreateContext();

				if (ctx == nullptr)
				{
					ok = false;
					return false;
				}

				check_asreturn( ctx->Prepare(funcId) );
				if (obj != nullptr)
					check_asreturn( ctx->SetObject(obj) );
			}
			return is_ok();
		}

		//! Sets the object for this caller (if it wasn't set before, or needs to be changed)
		bool set_object(asIScriptObject *obj)
		{
			//if (obj != NULL)
			//	obj->Release();
			obj = obj;
			//if (obj != NULL)
			//	obj->AddRef();
			if (ctx->GetState() & asEXECUTION_PREPARED)
			{
				return check_asreturn( ctx->SetObject(obj) );
			}
			else
				return true;
		}

		//! Throw a ScriptUtils#Caller#ScriptException if a script exception occors during execution
		void SetThrowOnException(bool should_throw)
		{
			throwOnException = should_throw;
		}

		typedef boost::function<void (asIScriptContext*)> script_callback_fn;

		typedef boost::signals2::signal<void (asIScriptContext*)> line_signal;
		typedef boost::signals2::signal<void (asIScriptContext*)> exception_signal;

		typedef std::shared_ptr<line_signal> line_signal_ptr;
		typedef std::shared_ptr<exception_signal> exception_signal_ptr;

		//! Connects a line callback slot
		boost::signals2::connection ConnectLineCallback(script_callback_fn fn)
		{
			if (LineSignal->empty())
				ctx->SetLineCallback(asFUNCTION(CallerLineCallback), LineSignal.get(), asCALL_CDECL);
			return LineSignal->connect(fn);
		}

		//! Connects an exception callback
		boost::signals2::connection ConnectExceptionCallback(script_callback_fn fn)
		{
			if (ScriptExceptionSignal->empty())
				ctx->SetExceptionCallback(asFUNCTION(CallerExceptionCallback), ScriptExceptionSignal.get(), asCALL_CDECL);
			return ScriptExceptionSignal->connect(fn);
		}

		//! Sets the given arg
		/*!
		* Use when args need to be set iteratively - otherwise use Caller#operator().
		*/
		template <typename T>
		int set_arg(asUINT arg, T t)
		{
			new (ctx->GetAddressOfArg(arg)) CallHelper<T>(t);
			return 0;
		}

		//template <typename T>
		//void set_arg(asUINT arg, T* t)
		//{
		//	ctx->SetArgObject(arg, (void*)t);
		//}

		template <>
		int set_arg(asUINT arg, asDWORD t)
		{
			return ctx->SetArgDWord(arg, t);
		}

		template <>
		int set_arg(asUINT arg, asQWORD t)
		{
			return ctx->SetArgQWord(arg, t);
		}

		template <>
		int set_arg(asUINT arg, float t)
		{
			return ctx->SetArgFloat(arg, t);
		}

		template <>
		int set_arg(asUINT arg, double t)
		{
			return ctx->SetArgDouble(arg, t);
		}

	protected:
		//! Called after each script line is executed
		//! \remarks This is a shared_ptr so that it can be shared between multiple Caller objects that reference the same context
		line_signal_ptr LineSignal;
		//! Called when a script throws an exception during execution
		exception_signal_ptr ScriptExceptionSignal;
		//script_exception_callback_fn OnScriptException;

		//! Executes the script method
		void execute(void)
		{
			// Make sure there is a valid ctx
			if (ctx == nullptr || ctx->GetState() != asEXECUTION_PREPARED)
				throw Exception("Can't execute '" + decl + "' - Caller is not prepared to execute");

			if (!ok)
				throw Exception("Can't execute '" + decl + "' - Caller is not valid");

			int r = ctx->Execute();
			if (r < 0)
				throw Exception("Failed while executing '" + decl + "'");
			
			if (r == asEXECUTION_EXCEPTION)
			{
				//! \todo ScriptException with line number, module, section, etc. properties and ctor taking ctx param
				if (throwOnException)
					throw Exception(std::string("Script Exception: ") + ctx->GetExceptionString());
			}
		}

		void* return_address(void)
		{
			return ctx->GetAddressOfReturnValue();
		}

		// Sets ok to false if the result of an AngelScript fn. indicates an error
		bool check_asreturn(int r)
		{
			if (r < 0)
				ok = false;
			return ok;
		}

	private:
		asIScriptContext* ctx;
		std::string decl;
		asIScriptObject* obj;
		int funcId;

		bool ok;

		bool throwOnException;
	};

	static void CallerLineCallback(asIScriptContext *ctx, void *obj)
	{
		CallerBase::line_signal *sig = static_cast<CallerBase::line_signal*>( obj );
		(*sig)(ctx);
	}

	static void CallerExceptionCallback(asIScriptContext *ctx, void *obj)
	{
		CallerBase::exception_signal *sig = static_cast<CallerBase::exception_signal*>( obj );
		(*sig)(ctx);
	}

}}

#endif
