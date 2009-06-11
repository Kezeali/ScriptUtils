/*
* ScriptUtils
* By Elliot Hayward
* Public Domain
*/

#ifndef H_SCRIPTUTILS_COMPLETEHEADERGENERATOR
#define H_SCRIPTUTILS_COMPLETEHEADERGENERATOR

#include "ProxyGenerator.h"

#include <boost/algorithm/string.hpp>


namespace ScriptUtils { namespace Inheritance
{

	class CompleteHeaderGenerator : public ProxyGenerator
	{
	public:
		std::string _tag;
		unsigned int _namespaceCount;

		CompleteHeaderGenerator(const std::string &filename,
			const std::string &tag,
			const std::string &includes = "<angelscript.h>", const std::string &namespaces = "")
			: ProxyGenerator(filename, std::ios::out, ProxyGenerator::cheader, true),
			_tag(tag),
			_namespaceCount(0)
		{
			init(includes, namespaces);
		}

		~CompleteHeaderGenerator()
		{
			deinit();
		}

		virtual void init(const std::string &includes, const std::string &namespaces)
		{
			if (file)
			{
				file << "// This is a generated file\n" << std::endl;

				// add include-once preprecessor stuff
				file << "#ifndef H_SCRIPT_" << boost::to_upper_copy(_tag) << std::endl;
				file << "#define H_SCRIPT_" << boost::to_upper_copy(_tag) << std::endl << std::endl;

				// add namespaces
				if (!includes.empty())
				{
					char_sep_tokenizer tokenizer(includes, char_sep_func(","));
					for (char_sep_tokenizer::iterator tok = tokenizer.begin(); tok != tokenizer.end(); tok++)
					{
						file << "#include " << *tok;
					}
					file << std::endl << std::endl;
				}

				// add namespaces
				if (!namespaces.empty())
				{
					char_sep_tokenizer tokenizer(namespaces, char_sep_func(","));
					for (char_sep_tokenizer::iterator tok = tokenizer.begin(); tok != tokenizer.end(); tok++)
					{
						file << "namespace " << *tok << " { ";
						_namespaceCount++; // Used to close all the namespace scopes in deinit()
					}
					file << std::endl;
				}

				// Define the function
				file << "\tint Add"+_tag+"ScriptSection(asIScriptEngine *engine, const char *module_name)" << std::endl;
				file << "\t{" << std::endl;
			}

			// Indent the outputted script string by two-tabs: one for the namespace indent, one for the function
			if (!namespaces.empty())
				this->SetIndentLevel(2);
			else // no namespace scope - no namespace indent
				this->SetIndentLevel(1);

			// init the code generation (also adds 'const char *script = ')
			ProxyGenerator::init();
		}

		virtual void deinit()
		{
			// Finish code generation
			ProxyGenerator::deinit();

			if (file)
			{
				// Don't need to copy the script
				file << "\t\tbool copy = engine->GetEngineProperty(asEP_COPY_SCRIPT_SECTIONS) == 1;" << std::endl;
				file << "\t\tengine->SetEngineProperty(asEP_COPY_SCRIPT_SECTIONS, false);" << std::endl;
				// Add the engine call to add the script section
				file << "\t\tint r = engine->GetModule(module_name, asGM_CREATE_IF_NOT_EXISTS)->AddScriptSection(\""+_tag+"\", script, std::strlen(script));" << std::endl;
				// Restore the copy-script engine property
				file << "\t\tengine->SetEngineProperty(asEP_COPY_SCRIPT_SECTIONS, copy);" << std::endl;
				file << "\t\treturn r;\n\t}" << std::endl;

				for (unsigned int i = 0; i < _namespaceCount; i++)
					file << "}";

				file << std::endl;

				file << "\n#endif" << std::endl;
			}
		}

	};

}}

#endif
