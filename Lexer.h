#ifndef _JET_LEXER_HEADER
#define _JET_LEXER_HEADER

#include "Token.h"
#include <map>
#include <vector>

namespace Jet
{
	bool IsLetter(char c);
	bool IsNumber(char c);

	class Lexer
	{
		unsigned int index;
		std::istream* stream;

		std::string text;

		unsigned int linenumber;

	public:
		Lexer(std::istream* input, std::string filename);
		Lexer(std::string text, std::string filename);

		Token Next();

		std::string filename;

		static std::map<TokenType, std::string> TokenToString;
	private:
		char ConsumeChar();
		char MatchAndConsumeChar(char c);
		char PeekChar();
	};
}
#endif