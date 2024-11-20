// created by Mark O. Brown
#pragma once
#include "ParameterSystem/Expression.h"
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <vector>
#include <string>

/*
 This class is designed to hold the text for a script that the code reads from.
*/
class ScriptStream : public std::stringstream
{
	public:
		explicit ScriptStream::ScriptStream (std::string buf);
		ScriptStream::ScriptStream() : std::stringstream() {}
		void setCase (bool alwaysLowerCase);
		/* 
		 The main purpose of this class is really to simplify my repeated use of >> 
		 using the following overload.
		 Note in standard C++ basic_istream >>, it will ignore C++ standard white space character, see isspace. And after 
		 >>, the get pointer will be moved to the first white space after. But then the subsequent >> will ignore those white space so it is fine.
		 But if were to use getline, need to call get() first to clear this white space char properly.
		 */
		ScriptStream & operator>>( std::string& outputString );
		ScriptStream& operator>>(Expression& Expression);
		void loadReplacements (std::vector<std::pair<std::string, std::string>> args, std::vector<parameterType>& params,
			std::string paramDecoration, std::string replCallScope, std::string funcScope);
		void clearReplacements();
		virtual std::string getline();
		virtual std::string getline(char delim);
private:
		// just for debugging purposes. 
		std::string initialContents;
		void eatComments();
		bool isNotPartOfName( char test );
		std::vector<std::pair<std::string, std::string>> replacements;
		bool alwaysLowerCase=true;
		std::string lastComment;
		std::string lastOutput;
};

/*
template<typename type>
ScriptStream& ScriptStream::operator<<(type& input)
{
	std::stringstream::operator<<(str(input));
	return *this;
}*/
