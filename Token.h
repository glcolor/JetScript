#ifndef _TOKEN_HEADER
#define _TOKEN_HEADER

#include <string>

namespace Jet
{
	enum class TokenType
	{
		Name,
		IntNumber,
		RealNumber,
		String,
		BlockString,
		Assign,		// =

		Dot,		// .
			
		Minus,		// -
		Plus,		// +
		Asterisk,	// *
		Slash,		// /
		Modulo,		// %
		Or,			// ||
		And,		// &&
		BOr,		// |
		BAnd,		// &
		Xor,		// ^
		BNot,		// ~
		LeftShift,	// <<
		RightShift,	// >>


		AddAssign,			// +=
		SubtractAssign,		// -=
		MultiplyAssign,		// *=
		DivideAssign,		// /=
		AndAssign,			// &=
		OrAssign,			// |=
		XorAssign,			// ^=

		NotEqual,			// !=
		Equals,				// ==
				
		LessThan,			// <
		GreaterThan,		// >
		LessThanEqual,		// <=
		GreaterThanEqual,	// >=

		RightParen,			// )
		LeftParen,			// (

		LeftBrace,			// {
		RightBrace,			// }

		LeftBracket,		// [
		RightBracket,		// ]

		While,		// while
		If,			// if
		ElseIf,		// elseif
		Else,		// else

		Colon,		//:
		Semicolon,	//;
		Comma,		//,
		Ellipses,	//...

		Null,		// null

		Function,	// function
		For,		// for
		Local,		// var »ò local
		Global,		// global
		Break,		// break
		Continue,	// continue
		Yield,		// yield
		Resume,		// resume

		Class,		// class
		New,		// new
		Base,		// base

		Const,		// consy

		Swap,		// <>

		Ret,		// return

		Increment,	// ++
		Decrement,	// --

		Operator,	// operator

		LineComment,
		CommentBegin,
		CommentEnd,

		EoF
	};

	char* Operator(TokenType t);

	struct Token
	{
		TokenType type;
		std::string text;
		unsigned int line;

		Token()
		{

		}

		Token(unsigned int line, TokenType type, std::string txt)
		{
			this->type = type;
			this->text = txt;
			this->line = line;
		}

		TokenType getType()
		{
			return type;
		}

		std::string getText()
		{
			return text;
		}
	};
}

#endif