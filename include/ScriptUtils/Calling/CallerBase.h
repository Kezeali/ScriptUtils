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

	//! CallHelper
	/*!
	* From SiCrane's code, not exactly sure why this was
	* used, but I guess it's supposed to work something
	* like std::reference_wrapper (maybe?)
	*/
	template <typename T>
	struct CallHelper
	{
		T element;
		CallHelper(T t) : element(t) {}
	private:
		CallHelper(const CallHelper& other) {}
	};

	static void CallerLineCallback(asIScriptContext *ctx, void *obj);

	static void CallerExceptionCallback(asIScriptContext *ctx, void *obj);

	//! Base class for callers
	class CallerBase
	{
	public:
		//! Default constructor
		/*!
		* Constructs an empty Caller
		*/
		CallerBase()
			: ctx(nullptr), obj(nullptr), func(nullptr), ok(false),
			throwOnException(false)
		{
		}

		//! Constructor for class methods
		CallerBase(asIScriptContext *context, asIScriptObject* object, asIScriptFunction* function)
			: ctx(context), obj(object), func(function), ok(true),
			throwOnException(false)
		{
			ctx->AddRef();

			check_asreturn(ctx->Prepare(func));
			check_asreturn(ctx->SetObject(obj));
		}

		//! Constructor for global methods (functions)
		CallerBase(asIScriptContext *context, asIScriptFunction* function)
			: ctx(context), obj(nullptr), func(function), ok(true),
			throwOnException(false)
		{
			ctx->AddRef();

			check_asreturn(ctx->Prepare(func));
		}

		//! Copy constructor
		CallerBase(const CallerBase &other)
			: ctx(other.ctx),
			obj(other.obj),
			func(other.func),
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
					check_asreturn(ctx->Prepare(func));
					if (obj != nullptr)
						check_asreturn(ctx->SetObject(obj));
				}
			}
		}

		//! Move constructor
		CallerBase(CallerBase &&other)
			: ctx(other.ctx),
			obj(other.obj),
			func(other.func),
			ok(other.ok),
			throwOnException(other.throwOnException),
			LineSignal(std::move(other.LineSignal)),
			ScriptExceptionSignal(std::move(other.ScriptExceptionSignal))
		{
			other.ctx = nullptr;
			other.obj = nullptr;
			other.func = nullptr;
		}

		//! Destructor
		~CallerBase()
		{
			release();
		}

		//! Copy-assignment operator
		CallerBase& operator= (const CallerBase &other)
		{
			// Release this object's reference to the context & script object;
			//  ctx is about to be copied from the other caller
			release();

			obj = other.obj;
			func = other.func;

			ok = other.ok;

			if (other.is_ok())
			{
				ctx = other.ctx->GetEngine()->CreateContext();

				if (ctx)
				{
					check_asreturn(ctx->Prepare(func));
					if (obj)
						check_asreturn(ctx->SetObject(obj));
				}
				else
					ok = false;
			}

			// Signal ptrs
			LineSignal = other.LineSignal;
			ScriptExceptionSignal = other.ScriptExceptionSignal;

			throwOnException = other.throwOnException;

			return *this;
		}

		//! Move-assignment operator
		CallerBase& operator= (CallerBase &&other)
		{
			// Release this object's reference to the context & script object;
			//  ctx is about to be taken from the other caller
			release();
			// Swap the uninit-ed ctx & obj with those of the other caller
			std::swap(ctx, other.ctx);
			std::swap(obj, other.obj);

			func = other.func;
			other.func = nullptr;

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
				if (ctx->GetState() == asEXECUTION_PREPARED && obj != nullptr)
				{
					// Temporarily remove the object from the ctx to prevent an error that seems to happen if a ctx is deallocated with a held object
					ctx->SetObject(nullptr);
					heldObject = true;
				}
				if (ctx->Release() > 0 && heldObject)
				{
					// If this caller wasn't the last reference to the ctx, restore the object 
					ctx->SetObject(obj);
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

				check_asreturn( ctx->Prepare(func) );
				if (obj != nullptr)
					check_asreturn( ctx->SetObject(obj) );
			}
			return is_ok();
		}

		int get_funcid() const
		{
			return func->GetId();
		}

		asIScriptFunction* get_func() const
		{
			return func;
		}

		//! Sets the object for this caller (if it wasn't set before, or needs to be changed)
		bool set_object(asIScriptObject *_obj)
		{
			//if (obj != nullptr)
			//	obj->Release();
			obj = _obj;
			//if (obj != nullptr)
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

		asEContextState GetState() const
		{
			return ctx->GetState();
		}

		typedef boost::function<void (asIScriptContext*)> script_callback_fn;

		typedef boost::signals2::signal<void (asIScriptContext*)> line_signal;
		typedef boost::signals2::signal<void (asIScriptContext*)> exception_signal;

		typedef std::shared_ptr<line_signal> line_signal_ptr;
		typedef std::shared_ptr<exception_signal> exception_signal_ptr;

		//! Connects a line callback slot
		boost::signals2::connection ConnectLineCallback(script_callback_fn fn)
		{
			if (!LineSignal)
			{
				LineSignal = std::make_shared<line_signal>();
				ctx->SetLineCallback(asFUNCTION(CallerLineCallback), LineSignal.get(), asCALL_CDECL);
			}
			return LineSignal->connect(fn);
		}

		//! Connects an exception callback
		boost::signals2::connection ConnectExceptionCallback(script_callback_fn fn)
		{
			if (!ScriptExceptionSignal)
			{
				ScriptExceptionSignal = std::make_shared<exception_signal>();
				ctx->SetExceptionCallback(asFUNCTION(CallerExceptionCallback), ScriptExceptionSignal.get(), asCALL_CDECL);
			}
			return ScriptExceptionSignal->connect(fn);
		}

		//! Returns the script context used by this Caller
		asIScriptContext* get_ctx() const
		{
			return ctx;
		}

		//! Sets the given arg
		/*!
		* Use when args need to be set iteratively - otherwise use Caller#operator().
		*/
		template <typename T>
		int set_arg(asUINT arg, T t)
		{
			if (ctx->GetAddressOfArg(arg) != nullptr)
			{
				new (ctx->GetAddressOfArg(arg)) CallHelper<T>(t);
				return 0;
			}
			else
				return asINVALID_ARG;
		}

		template <typename T>
		int set_arg(asUINT arg, T* t)
		{
			return ctx->SetArgAddress(arg, (void*)t);
		}

		int set_arg(asUINT arg, asDWORD t)
		{
			return ctx->SetArgDWord(arg, t);
		}
		int set_arg(asUINT arg, asQWORD t)
		{
			return ctx->SetArgQWord(arg, t);
		}
		int set_arg(asUINT arg, float t)
		{
			return ctx->SetArgFloat(arg, t);
		}
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

		std::string get_declaration() const
		{
			return func ? func->GetDeclaration() : "invalid function object";
		}

		//! Executes the script method
		void execute()
		{
			// Make sure there is a valid ctx
			if (ctx == nullptr || ctx->GetState() != asEXECUTION_PREPARED)
				throw Exception("Can't execute " + get_declaration() + " - Caller is not prepared to execute");

			if (!ok)
				throw Exception("Can't execute " + get_declaration() + " - Caller is not valid");

			int r = ctx->Execute();
			if (r < 0)
				throw Exception("Error while executing " + get_declaration());
			
			if (r == asEXECUTION_EXCEPTION)
			{
				if (throwOnException)
					throw Exception(std::string("Script Exception: ") + ctx->GetExceptionString());
			}
		}

		void* return_address()
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
		asIScriptObject* obj;
		asIScriptFunction* func;

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
