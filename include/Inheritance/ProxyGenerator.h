/*
* ScriptUtils
* By Elliot Hayward
* Public Domain
*/

#ifndef H_SCRIPTUTILS_PROXYGENERATOR
#define H_SCRIPTUTILS_PROXYGENERATOR

#include <angelscript.h>

#include <string>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "../Exception.h"

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
//#include <boost/algorithm/string.hpp>


namespace ScriptUtils { namespace Inheritance
{

	class ProxyGenerator;

	//! Passthrough for generating entire class hierarchies in one line.
	/*!
	* Makes a ProxyGenerator keep it's _inheritedDeclarations only as long as this object is alive.
	*
	* \see ProxyGenerator#Generate()
	*/
	class MaintainHierarchy
	{
	private:
		ProxyGenerator *_generator;

	public:
		//! CTOR
		MaintainHierarchy(ProxyGenerator *gen);
		//! DTOR
		~MaintainHierarchy();

		// Calls ProxyGenerator#GenerateDerived()
		MaintainHierarchy Begets(const char *type_name, const char *interface_names = NULL);

		// Accesses the stored generator
		ProxyGenerator* operator->() const;

	};

	//! Generates an AngelScript class which wraps a registered application class
	/*!
	* AngelScript doesn't have built-in support for script classes inheriting from
	* application registered classes. This class generates an AngleScript class,
	* with the same methods as the registered class, which can be inherited from.
	* These methods simply call the equivilant method on an internally held
	* instance of the registered class.<br>
	* The generated wrappers look something like:<br>
	* (e.g. for application class 'MyClass')
	* \code
	* class ScriptMyClass
	* {
	*   MyClass@ __inner;
	*   MyClass@ GetInner() { return __inner; }
	*   void SetInner(MyClass@ inner_) { __inner = inner_; }
	*
	*   int AMethod(const string &in p1) { return _inner.AMethod(p1); }
	* }
	* \endcode
	*/
	class ProxyGenerator
	{
		friend class MaintainHierarchy;

	public:
		enum OutputMode
		{
			script,
			cheader
		};

	protected:
		//typedef std::tr1::unordered_set<std::string> typename_set;
		typedef std::tr1::unordered_set<std::string> inherited_decl_set;
		// Interface (comma seperated) list tokenizer
		typedef boost::char_separator<char> char_sep_func;
		typedef boost::tokenizer<char_sep_func> char_sep_tokenizer;
		// Interface type list
		typedef std::vector<asIObjectType*> interface_list;

	public:
		//! Constructor
		/*!
		* Sets the name of the file to write the wrapper class definitions to.
		* Multiple definitions can be written to a single file - call Generate
		* multiple times on the same InstanceWrapperGenerator.
		*
		* \param[in] filename Name of the file to write.
		*
		* \param[in] file_mode Mode to open the file stream in.
		*
		* \param[in] output_type
		* Type of file to create.
		*
		* \param[in] manual_init
		* Sets when the start and end of the script data are added to the file: <br>
		* false - during constructor/destructor<br>
		* true - manually, by calling <code>init()</code> / <code>deinit()</code>
		*/
		ProxyGenerator(const std::string &filename, std::ios_base::open_mode file_mode = std::ios::out,
			OutputMode output_type = script, 
			bool manual_init = false)
			: file(filename.c_str(), file_mode),
			_output_type(output_type),
			_manual_init(manual_init),
			_typePrefix("Script"),
			_identPrefix("p"),
			_innerPrefix("__inner"),
			_outputIndent(""),
			_baseCount(0)
		{
			if (!_manual_init) init();
		}

		//! Constructor
		/*!
		* Sets file to write the wrapper class definitions to.
		* Multiple definitions can be written to a single file - call Generate
		* multiple times on the same InstanceWrapperGenerator.
		*/
		/*ProxyGenerator(std::ofstream &output_stream, OutputMode output_type = script)
		: file(output_stream),
		_output_type(output_type),
		_typePrefix("Script"),
		_identPrefix("p")
		{
		if (_output_type == cheader && file)
		{
		_linebegin = "\t\"";
		_lineend = "\\n\"\n";
		file << "const char * script =" << std::endl;
		}
		else
		{
		_linebegin = "";
		_lineend = "\n";
		}
		}*/

		//! Destructor
		~ProxyGenerator()
		{
			if (!_manual_init) deinit();
		}

		virtual void init()
		{
			if (_output_type == cheader && file)
			{
				_linebegin = _outputIndent + "\t\"";
				_lineend = "\\n\"\n";
				_tab = "";
				file << _outputIndent << "const char * script =" << std::endl;
			}
			else
			{
				_linebegin = _outputIndent + "";
				_lineend = "\n";
				_tab = "\t";
				_emptyline = _linebegin + _lineend;
			}
		}

		virtual void deinit()
		{
			if (_output_type == cheader && file)
			{
				file << _outputIndent << ";" << std::endl;
			}
		}

		//! Sets a string to be prepended generated classname.
		/*!
		* For example, if the <code>type_name</code> param passed to Generate()
		* is 'MyClass', the generated wrapper class will be called
		* 'prefixMyClass'. <br>
		* The default prefix is 'Script'.
		*
		* \param[in] prefix
		* The prefix to append to the generated classname
		*/
		void SetClassPrefix(const std::string &prefix)
		{
			_typePrefix = prefix;
		}
		//! Returns the current class prefix
		const std::string & GetClassPrefix() const { return _typePrefix; }

		//! Sets the parameter identifier prefix.
		/*!
		* The default prefix is 'p'. <br>
		* For example, if a registered class method has the declartion
		* <code>'int AMethod(const string&in)'</code>
		* the generated method in the wrapper class will be
		* \code
		* int AMethod(const string&in p1, int p2) { return __inner.AMethod(p1, p2); }
		* \endcode
		*
		* \param[in] prefix
		* The prefix to append to the generated parameter identifiers
		*/
		void SetIdentifierPrefix(const std::string &prefix)
		{
			_identPrefix = prefix;
		}
		//! Returns the current ident. prefix
		const std::string & GetIdentifierPrefix() const { return _identPrefix; }

		//! Sets the number of tabs before each line of the output
		void SetIndentLevel(size_t tab_count)
		{
			if (tab_count < _outputIndent.length())
				_outputIndent.resize(tab_count);
			else
			{
				for (size_t i = _outputIndent.length(); i < tab_count; i++)
					_outputIndent += "\t";
			}
		}

		//! Returns the number of tabs which will be inserted before each output line
		size_t GetIndentLevel() const
		{
			return _outputIndent.length();
		}

		//! Generates a wrapper class to allow inheriting from the given application class
		/*!
		* Interfaces should be passed as a string containing a comma seperated list.
		* <p>
		* Calling
		* \code
		* // Note that all the interfaces are passed in one string, containing
		* //  commas (not multiple parameters)
		* mygen.Generate(engine, "MyClass", "ScriptClass", "ISomethingA,ISomethingB");
		* \endcode
		* will generate the class
		* \code
		* class MyClass : ScriptClass, IClass, ISomething
		* \endcode
		* ... assuming <code>mygen.Generate(engine, "ScriptClass");</code> was called earlier,
		* otherwise it trying to call generate on a class derrived from ScriptClass will throw.
		* </p>
		*
		* \param[in] engine
		* The AS engine in which the types have been registered.
		*
		* \param[in] type_name
		* The name of the type to generate a wrapper class for.
		*
		* \param[in] basetype_name
		* The name of the base type of type_name, or NULL. IMPORTANT: Generate must be for this class
		* as type_name before it is used as a basetype_name.
		*
		* \param[in] interface_names
		* A comma seperated list comprising the interfaces for this class, or NULL.
		*
		* \returns
		* An object that allows Generate()-chaining.<br>e.g.
		* \code
		* gen.Generate(engine, "BaseClass", NULL, "IClass")
		*    .Begets("DerrivedClass");
		* \endcode
		*
		* \todo Check that the type type_name has implicit cast operator to basetype_name
		*/
		MaintainHierarchy Generate(asIScriptEngine *engine, const char *type_name, const char *basetype_name = NULL, const char *interface_names = NULL)
		{
			if (!file)
				throw Exception("File not available - either the path doesn't exist or there was a write error.");

			_engine = engine;

			int typeId = engine->GetTypeIdByDecl(type_name);
			if (typeId < 0)
				throw Exception(std::string(type_name) + " isn't registered - register it first then call ProxyGenerator::Generate");

			// Get the type-definition object for the class
			asIObjectType *type = engine->GetObjectTypeById(typeId);
			//_parsedClasses.insert(type_name);
			// Get the type-definition object for the base-class
			if (basetype_name == NULL)
			{
				// New chain of inheritance
				//_parsedClasses.clear();
				_inheritedDeclarations.clear();
			}
			if (basetype_name != NULL && _parsedLast != basetype_name)
			{
				// If the requested base-class wasn't the last one parsed, the inheritance
				//  chain has been broken
				throw Exception(std::string(basetype_name) +
					" wasn't the last class in the in the inheritance chain - you need to call Generate on each class in order.");
			}
			// Store the typename as the next valid base-class
			_parsedLast = type_name;

			// Get the type-definition objects for the interfaces
			interface_list ifaceTypeList;
			if (interface_names != NULL)
				listInterfaces(ifaceTypeList, interface_names);

			// Write the standard stuff to the beginning of class
			// Class type declaration
			file << _linebegin << "class " + _typePrefix + type_name;
			if (basetype_name != NULL || interface_names != NULL)
			{
				file << " : ";
				if (basetype_name != NULL)
				{
					file << _typePrefix << basetype_name;
					if (interface_names != NULL)
						file << ", ";
				}
				if (interface_names != NULL)
					file << interface_names;
				file << _lineend;
			}
			else
				file << _lineend;
			// Open the class scope
			file << _linebegin << "{" << _lineend;

			// Generate unique identifier for the wrapped-type-handle member
			_inner = _innerPrefix;
			if (basetype_name != NULL)
			{
				_inner = "cast<";
				_inner += type_name;
				_inner += ">("+_innerPrefix+")";
				/*_inner += "_";
				_inner += type->GetName();
				boost::to_lower(_inner);*/
			}

			// Wrapped type handle
			if (basetype_name == NULL) // Inner is only defined for most basic type
				file << _linebegin << _tab << type_name << "@ "+_innerPrefix+";" << _lineend;
			// App obj initializing CTOR
			if (basetype_name == NULL) // base type
				file << _linebegin << _tab << _typePrefix+type_name+"("<<type_name<<"@ appObj) { @"+_innerPrefix+" = @appObj; }" << _lineend;
			else // derived type - call super ctor
				file << _linebegin << _tab << _typePrefix+type_name+"("<<type_name<<"@ appObj) { super(appObj); }" << _lineend;
			// App obj. property methods
			//  Note that _SetAppObject uses _innerPrefix (i.e. the raw identifier) rather than than _inner (i.e. the casted ident)
			file << _linebegin << _tab << "void _SetAppObject("<<type_name<<"@ newInner) { @"+_innerPrefix+" = @newInner; }" << _lineend;
			file << _linebegin << _tab << type_name << "@ _GetAppObject() { return "+_inner+"; }" << _lineend << _emptyline;

			// Wrapped type methods
			writeMethods(type);

			// Interface methods
			for (interface_list::iterator it = ifaceTypeList.begin(), end = ifaceTypeList.end(); it != end; ++it)
			{
				writeInterfaceMethods(*it);
			}

			// Close the class scope
			file << _linebegin << "}" << _lineend;

			return MaintainHierarchy(this);
		}

	protected:
		//! Adds identifiers (where necessary) to the params of a declaration
		/*!
		* If the decl already has an identifier for a given param, it will be kept - 
		* <p>
		* i.e. <code>void SetSize(int size)</code> becomes <code>void SetSize(int size)</code>
		* (no change), not <code>void SetSize(int p1)</code> (original identifier replaced with
		* p1)
		* </p>
		* - I'm not sure how usefull this is for generating wrapper methods from Registered 
		* functions (i.e. when the decl passed is from <code>function->GetDeclaration()</code>),
		* since I think registered function declarations have all their param ident.s
		* stripped. But it would be useful for generating wrappers for function
		* declarations from other sources.
		*
		* \param[in] decl
		* The registered declaration for this method
		*
		* \param[out] paramNames
		* Returns a comma seperated list containing the identifiers that were used
		* to expand the declaration.
		*
		* \returns
		* The expanded declaration (i.e. with identifiers for all the params)
		*/
		std::string expandDeclaration(const std::string &decl, std::string &paramNames)
		{
			std::stringstream expandedDecl;

			int numParams = 0, numIdents = 0;
			bool hasParams = false, noMissingIdents = true;
			int tokenLength = 0;
			std::string::size_type decl_pos = decl.find('(') + 1, token_pos;

			// Check for reference return type (script methods can't return references)
			//  and convert them to handles - i.e. '&' to '@'
			std::string::size_type refmodifierPos = decl.find('&');

			if (decl_pos != std::string::npos && refmodifierPos != std::string::npos &&
				refmodifierPos < decl_pos)
			{
				// Add the return type identifier, replacing the '&' modifier
				expandedDecl << decl.substr(0, refmodifierPos) << "@";
				// Add some or all of the rest of the decl (depening on whether it has params)
				if (decl[decl_pos] != ')')
				{
					expandedDecl << decl.substr(refmodifierPos + 1, decl_pos-(refmodifierPos+1));
				}
				else // If the next character after the '(' found above is a closing ')', decl has no params
				{
					expandedDecl << decl.substr(refmodifierPos + 1);
					return expandedDecl.str();
				}
			}
			else
			{
				if (decl[decl_pos] == ')') // decl has no params
					return decl;

				// Push everything up to the beginning of the params
				//  (e.g. 'void myfunc(') into the stream
				expandedDecl << decl.substr(0, decl_pos);
			}

			int state = 0;
			while (decl_pos < decl.length())
			{
				asETokenClass tt = _engine->ParseToken(decl.data() + decl_pos, decl.length() - decl_pos, &tokenLength);
				token_pos = decl_pos; // The position of the token that was juat read
				decl_pos += tokenLength; // Move the read cursor to the end of the current token

				if (tt == asTC_IDENTIFIER)
				{
					if (state == 0)
						state = 1;
					else if (state == 2)
						state = 3;
				}
				else if (tt == asTC_WHITESPACE)
				{
					if (state == 1)
						state = 2;
				}

				// If there is an identifier, it is a param type OR
				// If there is a keyword that isn't a bracket, it is a param type
				if (tt == asTC_IDENTIFIER || (tt == asTC_KEYWORD && decl[token_pos] != '(' && decl[token_pos] != ')'))
				{
					hasParams = true;
				}

				// All existing tokens* are written to the stream -
				//  remember, we are only tokenizing to find missing param
				//  identifiers
				// *except comments
				if (tt == asTC_IDENTIFIER || tt == asTC_KEYWORD || tt == asTC_WHITESPACE)
				{
					expandedDecl << decl.substr(token_pos, tokenLength);
				}

				// if this param has an identifier
				//  (i.e. <ident token><whitespace toke><ident token> was found within a param)
				if (state == 3)
				{
					numIdents++;
					state = 0;
				}

				if (decl_pos < decl.length())
				{
					if (decl[decl_pos] == ',') // finished reading a param
					{
						state = 0;
						numParams++;

						if (numParams > numIdents) // no ident. token for this param
						{
							expandedDecl << " "+_identPrefix << numParams; // add a new ident token to the decl
							// List the new param ident. in the output list
							paramNames += _identPrefix + boost::lexical_cast<std::string>(numParams) + ", ";
						}
						else // param ident. token exists
						{
							// The following line is commented out (i.e. not needed) because
							//  the existing ident token would have already been added
							//  (since all existing tokens are added)
							//expandedDecl << decl.substr(token_pos, tokenLength);

							// List the existing param ident. in the output list
							paramNames += decl.substr(token_pos, tokenLength);
						}

						//expandedDecl << ", ";
					}

					if (hasParams && decl[decl_pos] == ')')
					{
						state = 0;
						numParams++;

						if (numParams > numIdents)
						{
							expandedDecl << " "+_identPrefix << numParams;
							paramNames += _identPrefix + boost::lexical_cast<std::string>(numParams);
						}
						else
						{
							paramNames += decl.substr(token_pos, tokenLength);;
						}
					}
				}

				if (numParams > numIdents)
					noMissingIdents = false;
			}

			return expandedDecl.str();
		}

		//! Creates a list of asIObjectType objects which describes the comma seperated list given
		/*!
		* \param[out] list Returns the object-type list
		*
		* \param[in] interface_names
		* The comma seperated list of interfaces (as it would appear in the
		* class definition.)
		*/
		void listInterfaces(interface_list &list, const std::string &interface_names)
		{
			if (!interface_names.empty())
			{
				char_sep_tokenizer tokenizer(interface_names, char_sep_func(","));
				for (char_sep_tokenizer::iterator tok = tokenizer.begin(); tok != tokenizer.end(); tok++)
				{
					int ifaceId = _engine->GetTypeIdByDecl(tok->c_str());
					if (ifaceId >= 0)
						list.push_back( _engine->GetObjectTypeById(ifaceId) );
				}
			}
		}

		//! Writes method members of the given type to the file
		//! \todo Param for _inner (rather than making that a member variable)
		void writeMethods(asIObjectType *type)
		{
			asIScriptFunction *method;
			for (int i = 0, count = type->GetMethodCount(); i < count; i++)
			{
				method = type->GetMethodDescriptorByIndex(i);

				if (_inheritedDeclarations.insert(method->GetDeclaration(false)).second)
				{
					// Write the decl
					std::string params;
					file << _linebegin << _tab << expandDeclaration(method->GetDeclaration(false), params);
					// Write the definition
					file << " { ";
					//  ... make the fn. return if necessary
					int retId = method->GetReturnTypeId();
					if (retId != asTYPEID_VOID)
						file << "return ";
					file << _inner+"." << method->GetName() << "(" << params << "); }" << _lineend;
				}
			}
		}

		//! Writes interface methods to the file
		/*!
		* \see writeMethods()
		*/
		void writeInterfaceMethods(asIObjectType *iface_type)
		{
			asIScriptFunction *method;
			for (int i = 0, count = iface_type->GetMethodCount(); i < count; i++)
			{
				method = iface_type->GetMethodDescriptorByIndex(i);

				// Write the decl
				std::string params;
				file << _linebegin << _tab << expandDeclaration(method->GetDeclaration(false), params);
				// Write the definition
				file << " { ";
				//  ... make the fn. return if necessary
				int retId = method->GetReturnTypeId();
				if (retId != asTYPEID_VOID)
					file << "return " << _engine->GetTypeDeclaration(retId) << "(); ";
				file << "}" << _lineend;
			}
		}

		//! Generates a class derived from the last one generated
		MaintainHierarchy GenerateDerived(const char *type_name, const char *interface_names)
		{
			// Have to copy this or it changes
			//  when basetype changes (since it's passed to Generate as a char*)
			std::string basetype_name = _parsedLast;
			return Generate(_engine, type_name, basetype_name.c_str(), interface_names);
		}

	protected:
		std::ofstream file;

		asIScriptEngine *_engine;

		std::string _parsedLast;
		//typename_set _parsedClasses;
		inherited_decl_set _inheritedDeclarations;

		unsigned int _baseCount;
		unsigned int incHierarchy()
		{
			return ++_baseCount;
		}
		unsigned int decHierarchy()
		{
			if (_baseCount > 0 && --_baseCount == 0)
			{
				_inheritedDeclarations.clear();
				_parsedLast.clear();
			}
			return _baseCount;
		}

	private:
		OutputMode _output_type;

		// Prevents the dtor from calling deinit
		bool _manual_init;

		// Prefixes - stuff appened to words throughout the file
		std::string _typePrefix;
		std::string _identPrefix;
		// This should be '_baseHandleIdent'
		std::string _innerPrefix;
		// Entire inner identifier for the current class - may be just _innerPrefix, may be _innerPrefix + classname
		//  This should be '_derivedHandleIdent'
		std::string _inner;

		// Characters before each line of the output
		std::string _outputIndent;

		// Strings used throughout the file
		std::string _linebegin;
		std::string _lineend;
		std::string _tab;
		std::string _emptyline;

	};

	MaintainHierarchy::MaintainHierarchy(ProxyGenerator *gen)
		: _generator(gen)
	{
		_generator->incHierarchy();
	}

	MaintainHierarchy::~MaintainHierarchy()
	{
		_generator->decHierarchy();
	}

	MaintainHierarchy MaintainHierarchy::Begets(const char *type_name, const char *interface_names)
	{
		return _generator->GenerateDerived(type_name, interface_names);
	}

	ProxyGenerator* MaintainHierarchy::operator->() const
	{
		return _generator;
	}

}}

#endif
