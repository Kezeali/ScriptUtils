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
			: _ok(false), _obj(NULL), _funcId(-1), _ctx(NULL),
			_throwOnException(false),
			OnScriptException(new exception_signal),
			OnLineCallback(new line_signal)
		{
		}

		//! Constructor for object methods
		CallerBase(asIScriptObject * obj, const char * decl)
			: _obj(obj), _decl(decl), _ok(true),
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
			check_asreturn( _ctx->SetObject(_obj) );
			//_obj->AddRef();

			InitCallbacks();
		}

		//! Constructor for global methods (functions)
		CallerBase(asIScriptModule *module, const char * decl)
			: _obj(NULL), _decl(decl), _ok(true),
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
			: _obj(NULL), _funcId(funcId), _ok(true),
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
			_obj = other._obj;
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
			release();
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
			_obj = other._obj;
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
			if (_ctx != NULL)
			{
				bool refAdded = false;
				if (_ctx->GetState() == asEXECUTION_PREPARED && _obj != NULL)
				{
					//_obj->AddRef();
					_ctx->SetObject(NULL);
					refAdded = true;
				}
				if ( _ctx->Release() == 0)
				{
					_ctx = NULL;
					_ok = false;
				}
				else if (refAdded)
				{
					//_obj->Release();
					_ctx->SetObject(_obj);
				}
			}
		}

		//! Returns true if the fn. is prepared to execute
		virtual bool ok()
		{
			return _ok;
		}

		virtual bool refresh()
		{
			if (_ctx->GetState() != asEXECUTION_PREPARED)
			{
				asIScriptEngine *engine = _ctx->GetEngine();
				_ctx->Release();
				_ctx = engine->CreateContext();

				if (_ctx == nullptr)
				{
					_ok = false;
					return false;
				}

				check_asreturn( _ctx->Prepare(_funcId) );
				if (_obj != nullptr)
					check_asreturn( _ctx->SetObject(_obj) );
			}
			return ok();
		}

		//! Sets the object for this caller (if it wasn't set before, or needs to be changed)
		bool set_object(asIScriptObject *obj)
		{
			//if (_obj != NULL)
			//	_obj->Release();
			_obj = obj;
			//if (obj != NULL)
			//	obj->AddRef();
			if (_ctx->GetState() & asEXECUTION_PREPARED)
			{
				return check_asreturn( _ctx->SetObject(obj) );
			}
			else
				return true;
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

		//! Sets the given arg
		/*!
		* Use when args need to be set iteratively - otherwise use Caller#operator().
		*/
		template <typename T>
		int set_arg(asUINT arg, T t)
		{
			new (_ctx->GetAddressOfArg(arg)) CallHelper<T>(t);
			return 0;
		}

		//template <typename T>
		//void set_arg(asUINT arg, T* t)
		//{
		//	_ctx->SetArgObject(arg, (void*)t);
		//}

		template <>
		int set_arg(asUINT arg, asDWORD t)
		{
			return _ctx->SetArgDWord(arg, t);
		}

		template <>
		int set_arg(asUINT arg, asQWORD t)
		{
			return _ctx->SetArgQWord(arg, t);
		}

		template <>
		int set_arg(asUINT arg, float t)
		{
			return _ctx->SetArgFloat(arg, t);
		}

		template <>
		int set_arg(asUINT arg, double t)
		{
			return _ctx->SetArgDouble(arg, t);
		}

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
			if (_ctx == nullptr || _ctx->GetState() != asEXECUTION_PREPARED)
				throw Exception("Can't execute'" + _decl + "' - Caller is not prepared to execute");

			if (!_ok)
				throw Exception("Can't execute'" + _decl + "' - Caller is not valid");

			int r = _ctx->Execute();
			if (r < 0)
				throw Exception("Failed to execute '" + _decl + "'");
			
			if (r == asEXECUTION_EXCEPTION)
			{
				//! \todo ScriptException with line number, module, section, etc. properties and ctor taking ctx param
				if (_throwOnException)
					throw Exception(std::string("Script Exception: ") + _ctx->GetExceptionString());
			}
		}

		void * return_address(void)
		{
			return _ctx->GetAddressOfReturnValue();
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
		asIScriptObject *_obj;
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
