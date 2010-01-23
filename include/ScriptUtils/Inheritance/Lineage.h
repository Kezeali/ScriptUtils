/*
* ScriptUtils
* By Elliot Hayward
* Public Domain
*/

#ifndef H_SCRIPTUTILS_LINEAGE
#define H_SCRIPTUTILS_LINEAGE

#include <angelscript.h>

#include "ProxyGenerator.h"

//#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/find_iterator.hpp>


namespace ScriptUtils { namespace Inheritance
{
	
	class Lineage
	{
	public:
		Lineage(ProxyGenerator *gen)
			: _gen(gen);
		{}

		void DefineLineage(const std::string &lineage)
		{
		}

		void AddInterfaces(const std::string &class, const std::string &interfaces)
		{
		}

		void Generate()
		{
			for(split_iter it = boost::make_find_iterator(str1, first_finder(",", is_iequal()));
				it != split_iter(); ++it)
			{
				cout << copy_range<std::string>(*It) << endl;
			}
		}
	protected:
		typedef boost::split_iterator<std::string::iterator> split_iter;

		ProxyGenerator *_gen;
	};

}}

#endif