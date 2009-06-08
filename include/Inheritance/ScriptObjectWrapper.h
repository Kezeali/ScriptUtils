/*
* ScriptUtils
* By Elliot Hayward
* Public Domain
*/

#ifndef H_SCRIPTUTILS_SCRIPTOBJECTWRAPPER
#define H_SCRIPTUTILS_SCRIPTOBJECTWRAPPER

#include "../Calling/Caller.h"
#include "TypeTraits.h"

#include <memory>
#include <unordered_map>


namespace ScriptUtils { namespace Inheritance
{

	//! Helps class wrappers call script functions
	class ScriptObjectWrapper
	{
	public:
		//! Constructor
		ScriptObjectWrapper(asIScriptObject * obj)
			: _obj(obj)
		{
			init();
		}
		//! Constructor, with compatibility check
		/*!
		* If the type of obj doesn't implement the interface named
		* in the iface param, an exception is thrown.
		*
		* \param[in] obj
		* The script object to wrap.
		*
		* \param[in] iface
		* The name of the interface type <code>obj</code> must implement.
		*/
		ScriptObjectWrapper(asIScriptObject * obj, const char * iface)
			: _obj(obj), _iface(iface)
		{
			init();
		}
		//! Destructor
		virtual ~ScriptObjectWrapper()
		{
			if (_obj != NULL)
			{
				_obj->Release();
				_obj = NULL;
			}
		}

		//! Creates a Caller for the given method
		Calling::Caller get_caller(const char * decl)
		{
			return findOrCreate_caller(decl);
		}

		asIScriptObject *get_script_object() const
		{
			return _obj;
		}

	private:
		//! To be run during CTOR
		/*!
		* If _iface is set, the type of _obj is checked to make sure
		* it implements the interface type (called _iface).
		*/
		void init()
		{
			if (_obj != NULL)
			{
				_obj->AddRef();
				
				// Check that this type has the expected interface
				if (!_iface.empty())
				{
					// Try to get the interface type
					int ifaceId = _obj->GetEngine()->GetTypeIdByDecl(_iface.c_str());
					if (ifaceId < 0)
						throw Exception(_iface + " isn't a registered interface type-name");
					asIObjectType *ifaceType = _obj->GetEngine()->GetObjectTypeById(ifaceId);

					// Do the actual check
					if (!base_implements(_obj->GetObjectType(), ifaceType))
						throw Exception(std::string(_obj->GetObjectType()->GetName()) + " doesn't implement " + _iface);
				}
			}
		}

		Calling::Caller findOrCreate_caller(const std::string &decl)
		{
			if (_obj == NULL)
				return Calling::Caller();
			//caller_map::iterator _where = m_Callers.find(decl);
			//if (_where == m_Callers.end())
			//{
			//	Calling::Caller caller(_obj, decl.c_str());
			//	m_Callers[decl] = caller;

			//	return caller;
			//}
			//else
			//{
			//	return (_where->second);
			//}
			return Calling::Caller(_obj, decl.c_str());
		}

	protected:
		void set_obj(asIScriptObject *obj)
		{
			_obj = obj;
			if (obj != NULL)
				obj->AddRef();
		}
		asIScriptObject * _obj;
		std::string _iface;

	private:
		//typedef std::tr1::shared_ptr<Calling::Caller> CallerPtr;
		typedef std::tr1::unordered_map<std::string, Calling::Caller> caller_map;

		//! Prevent copying
		ScriptObjectWrapper(const ScriptObjectWrapper &);
		//! Prevent copying
		ScriptObjectWrapper & operator=(const ScriptObjectWrapper &);

		caller_map m_Callers;
	};

}}

#endif