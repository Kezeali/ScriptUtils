/*
* ScriptUtils
* by Elliot Hayward
* Public Domain
*/


#ifndef H_SCRIPTUTILS_EXCEPTION
#define H_SCRIPTUTILS_EXCEPTION

#include <string>

namespace ScriptUtils
{

	//! Exception class
	class Exception
	{
	public:
		//! Constructor
		Exception(const std::string &message)
			: m_Message(message)
		{}

	public:
		//! Gets the message of the exception
		const std::string &GetMessage() const { return m_Message; }

		std::string m_Message;
	};

}

#endif
