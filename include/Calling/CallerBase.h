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
	//	//! Default line callback fn. - Fires OnLineCallback
	//	void LineCallback(asIScriptContext *ctx);
	//	//! Default exception callback fn. - Fires OnScriptException
	//	void ExceptionCallback(asIScriptContext *ctx);

	//private:
	//	CallerBase *m_Targer;
	//};

	void CallerLineCallback(asIScriptContext *ctx, void *obj);

	void CallerExceptionCallback(asIScriptContext *ctx, void *obj);

	//! Base class for callers
	/*!
	* \todo Allow line-callbacks, provide LineDebugger as the default CB implementation
	*/
	class CallerBase
	{
	public:
		//! Default constructor
		/*!
		* Constructs an invalid Caller - only provided for container support
		*/
		CallerBase()
			: _ok(false), _funcId(-1), _ctx(NULL),
			_throwOnException(false),
			OnScriptException(new exception_signal),
			OnLineCallback(new line_signal)
		{
		}

		//! Constructor for object methods
		CallerBase(asIScriptObject * obj, const char * decl)
			: _decl(decl), _ok(true),
			_throwOnException(false),
			OnScriptException(new exception_signal),
			OnLineCallback(new line_signal)
		{
			asIScriptEngine *engine = obj->GetEngine();

			_ctx = engine->CreateContext();

			asIObjectType *type = obj->GetObjectType();
			_funcId = type->GetMethodIdByDecl(decl);
			check_asreturn(_funcId);

			check_asreturn( _ctx->Prepare(_funcId) );
			check_asreturn( _ctx->SetObject(obj) );

			InitCallbacks();
		}

		//! Constructor for global methods (functions)
		CallerBase(asIScriptModule *module, const char * decl)
			: _decl(decl), _ok(true),
			_throwOnException(false),
			OnScriptException(new exception_signal),
			OnLineCallback(new line_signal)
		{
			asIScriptEngine *engine = module->GetEngine();

			_ctx = engine->CreateContext();

			_funcId = module->GetFunctionIdByDecl(decl);
			check_asreturn(_funcId);

			check_asreturn( _ctx->Prepare(_funcId) );

			InitCallbacks();
		}

		//! Constructor for global methods (functions)
		CallerBase(asIScriptEngine *engine, int funcId)
			: _funcId(funcId), _ok(true),
			_throwOnException(false),
			OnScriptException(new exception_signal),
			OnLineCallback(new line_signal)
		{
			_ctx = engine->CreateContext();

			if (check_asreturn(_funcId))
				_decl = engine->GetFunctionDescriptorById(_funcId)->GetDeclaration();

			check_asreturn( _ctx->Prepare(_funcId) );

			InitCallbacks();
		}

		//! Copy constructor
		CallerBase(const CallerBase &other)
		{
			_ctx = other._ctx;
			_decl = other._decl;
			_funcId = other._funcId;
			_ok = other._ok;

			// Signal ptrs
			OnLineCallback = other.OnLineCallback;
			//  Exception handling
			_throwOnException = other._throwOnException;
			OnScriptException = other.OnScriptException;

			if (_ctx != NULL)
				_ctx->AddRef();
		}

		//! Destructor
		virtual ~CallerBase()
		{
			if (_ctx != NULL)
				_ctx->Release();
		}

		//! Copy assignement operator
		CallerBase & operator=(const CallerBase &other)
		{
			// Release this object's reference to the context
			//  - it doesn't matter if this invalidates this caller
			//  (sets _ok to false, nullifies _ctx), because _ok and
			//  _ctx are about to be copied from the other caller
			release();
			// Copy the other object's ctx
			if (other._ctx != NULL)
				other._ctx->AddRef();
			_ctx = other._ctx;

			_decl = other._decl;
			_funcId = other._funcId;
			_ok = other._ok;

			// Signal ptrs
			OnLineCallback = other.OnLineCallback;
			//  Exception handling
			_throwOnException = other._throwOnException;
			OnScriptException = other.OnScriptException;

			return *this;
		}

		//! Releases the internal asIScriptContext
		virtual void release()
		{
			if (_ctx != NULL && _ctx->Release() == 0)
			{
				_ctx = NULL;
				_ok = false;
			}
		}

		//! Returns true if the fn. is prepared to execute
		virtual bool ok()
		{
			return _ok;
		}

		//! Sets the object for this caller (if it wasn't set before, or needs to be changed)
		bool set_object(asIScriptObject *obj)
		{
			return check_asreturn( _ctx->SetObject(obj) );
		}

		//! Throw a ScriptUtils#Caller#ScriptException if a script exception occors during execution
		void SetThrowOnException(bool should_throw)
		{
			_throwOnException = should_throw;
		}

		typedef boost::function<void (asIScriptContext*)> script_callback_fn;

		typedef boost::signals2::signal<void (asIScriptContext*)> line_signal;
		typedef boost::signals2::signal<void (asIScriptContext*)> exception_signal;

		typedef std::tr1::shared_ptr<line_signal> line_signal_ptr;
		typedef std::tr1::shared_ptr<exception_signal> exception_signal_ptr;

		//! Connects a line callback slot
		boost::signals2::connection ConnectLineCallback(script_callback_fn fn)
		{
			return OnLineCallback->connect(fn);
		}

		//! Connects an exception callback
		boost::signals2::connection ConnectExceptionCallback(script_callback_fn fn)
		{
			return OnScriptException->connect(fn);
		}

		void InitCallbacks()
		{
			//CallerContextCallbacksPtr callbackObj(new CallerContextCallbacks(this));
			_ctx->SetLineCallback(asFUNCTION(CallerLineCallback), OnLineCallback.get(), asCALL_CDECL);
			_ctx->SetExceptionCallback(asFUNCTION(CallerExceptionCallback), OnScriptException.get(), asCALL_CDECL);
		}

		//! Change the line callback
		/*!
		* The default line callback fires the OnLineCallback signal -
		* calling this function will overwrite that, so OnLineCallback
		* slots will no longer be executed.
		*/
		//void SetLineCallback(asSFuncPtr fn, void *obj)
		//{
		//	_ctx->SetLineCallback(fn, obj, asCALL_THISCALL);
		//}

		//! Change the exception callback
		/*!
		* The default exception callback fires the OnScriptException signal -
		* calling this function will overwrite that, so OnScriptException
		* slots will no longer be executed.
		*/
		//void SetExceptionCallback(asSFuncPtr fn, void *obj)
		//{
		//	_ctx->SetExceptionCallback(fn, obj, asCALL_THISCALL);
		//}

	protected:
		//! Called after each script line is executed
		line_signal_ptr OnLineCallback;
		//! Called when a script throws an exception during execution
		exception_signal_ptr OnScriptException;
		//script_exception_callback_fn OnScriptException;

		//! Executes the script method
		void execute(void)
		{
			// Make sure there is a valid ctx
			if (_ctx == NULL)
			{
				// Make a new ctx
				//add_ref();
				return;
			}

			if (_ctx->GetState() != asEXECUTION_PREPARED)
				check_asreturn( _ctx->Prepare(_funcId) );

			if (!_ok)
				throw Exception("Can't execute'" + _decl + "' - Caller is not valid");

			int r = _ctx->Execute(); 
			if (r < 0)
				throw Exception("Failed to execute '" + _decl + "'");
			
			if (r == asEXECUTION_EXCEPTION)
			{
				// Fire signal to exception catchers
				//(*OnScriptException)(_ctx);

				//! \todo ScriptException with line number, module, section, etc. properties and ctor taking ctx param
				if (_throwOnException)
					throw Exception(std::string("Script Exception: ") + _ctx->GetExceptionString());
			}
		}

		void * return_address(void)
		{
			return _ctx->GetAddressOfReturnValue();
		}

		template <typename T>
		void set_arg(asUINT arg, T t)
		{
			new (_ctx->GetArgPointer(arg)) CallHelper<T>(t);
		}

		// Sets _ok to false if the result of an AngelScript fn. indicates an error
		bool check_asreturn(int r)
		{
			if (r < 0)
				_ok = false;
			return _ok;
		}

	private:
		asIScriptContext * _ctx;
		std::string _decl;
		int _funcId;

		bool _ok;

		bool _throwOnException;
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
