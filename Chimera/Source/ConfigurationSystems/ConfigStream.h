#pragma once
#include <iostream>
#include "ParameterSystem/Expression.h"
#include "Version.h"
#include "Scripts/ScriptStream.h"
#include <type_traits>
#include <typeinfo>

/* a small wrapper to handle some special things for configuration files, like handling empty strings.*/
class ConfigStream : public ScriptStream{
	public:
		ConfigStream ();
		explicit ConfigStream(std::ifstream& file);
		explicit ConfigStream (std::string, bool isAddress=false);
		ConfigStream& operator>>(std::string& outputString);
		ConfigStream& operator>>(Expression& expression);
		template <typename T>
		ConfigStream& operator<<(const T& value);
		template<typename type>
		ConfigStream& operator>> (type& output);
		std::string jumpline();
		std::string getline ();
		std::string getline (char delim);
		const static std::string emptyStringTxt;
		Version ver;
private:
		std::string streamText;
};

// This helper is not used for now, but potentially useful to have. 
// https://stackoverflow.com/questions/27687389/how-do-we-use-void-t-for-sfinae
// Helper trait to detect if T can be streamed to std::ostream
template <typename T, typename = void>
struct is_streamable : std::false_type {};

template <typename T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>>
	: std::true_type {};

// Convenience variable template
template <typename T>
constexpr bool is_streamable_v = is_streamable<T>::value;


template<typename T>
inline ConfigStream& ConfigStream::operator<<(const T& value)
{
	if constexpr (std::is_same_v<T, char[1]> || 
		std::is_same_v<T, const char*> || 
		std::is_same_v<T, std::string>) {
		// Use stringstream's operator<< for strings
		std::string inputString(value);
		if (inputString == "") {
			inputString = ConfigStream::emptyStringTxt;
		}
		static_cast<std::ostream&>(*this) << inputString;
	}
	else {
		static_cast<std::ostream&>(*this) << value;
	}
	//else if constexpr (is_streamable_v<T>) {
	//	// Use ostream's operator<< for types that have it
	//	static_cast<std::ostream&>(*this) << value;
	//}
	//else {
	//	// Fallback to stringstream's operator<<
	//	std::stringstream::operator<<(value);
	//}
	return *this;
}

template<typename type>
ConfigStream& ConfigStream::operator>> (type& output){
	std::string tempString;
	ScriptStream::operator>>(tempString);
	//if (tempString == ConfigStream::emptyStringTxt) {
	//	tempString = "";
	//}
	try{
		output = boost::lexical_cast<type>(tempString);
	}
	catch (boost::bad_lexical_cast){
		throwNested ("Scriptstream Failed to convert the text \"" + tempString + "\" to the requested type! Requested "
			"type was \"" + typeid(output).name () + "\".");
	}
	return *this;
}

std::ostream& operator<<(std::ostream& os, const Expression& expr);
//ConfigStream& operator<<(ConfigStream& os, const std::string& inputString);
