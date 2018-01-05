#ifndef _JET_INSTRUCTIONS_HEADER
#define _JET_INSTRUCTIONS_HEADER

//if FORCE_USING_GLOBAL be set to 1,all global vars must be declared by 'global',for example: global x=0.0;
#define  FORCE_USING_GLOBAL 0

namespace Jet
{
	const static char* Instructions[] = 
	{
		"Add",
		"Mul",
		"Div",
		"Sub",
		"Modulus",

		"Negate",

		"BAnd",
		"BOr",
		"Xor",
		"BNot",
		"LeftShift",
		"RightShift",

		"Eq",
		"NotEq",
		"Lt",
		"Gt",
		"LtE",
		"GtE",
		"Incr",
		"Decr",
		"Dup",
		"Pop",
		"LdInt",
		"LdReal",
		"LdNull",
		"LdStr",
		"LoadFunction",
		"Jump",
		"JumpTrue",
		"JumpTruePeek",
		"JumpFalse",
		"JumpFalsePeek",
		"NewArray",
		"NewObject",

		"Store",
		"Load",
		//local vars
		"LStore",
		"LLoad",
		//captured vars
		"CStore",
		"CLoad",
		"CInit",

		"ForEach",

		//index functions
		"LoadAt",
		"StoreAt",

		//these all work on the last value in the stack
		"ECall",

		"Call",
		"Return",
		"Resume",
		"Yield",
		"Close",

		//dummy instructions for the assembler/debugging
		"Label",
		"Local",
		"Global",
		"Capture",
		"DebugLine",
		"Function"
	};

	enum class InstructionType
	{
		Add, Mul, Div, Sub, Modulus,
		Negate,

		BAnd, BOr, Xor, BNot,
		LeftShift, RightShift,

		Eq, NotEq,
		Lt, Gt,
		LtE, GtE,

		Incr,
		Decr,

		Dup, Pop,//stack operations

		LdInt,
		LdReal,
		LdNull,
		LdStr,
		LoadFunction,

		Jump,
		JumpTrue,  JumpTruePeek,
		JumpFalse, JumpFalsePeek,

		NewArray,
		NewObject,

		Store,Load,		//globals
		LStore,LLoad,	//local vars
		CStore,CLoad,	//captures

		CInit, //to setup captures

		//loop instruction
		ForEach,

		//index functions
		LoadAt,
		StoreAt,

		//these all work on the last value in the stack
		ECall,
		
		Call,
		Return,

		//generator stuff
		Resume,
		Yield,

		Close, //closes all opened closures in a function

		//dummy instructions for the assembler/debugging
		Label,
		Local,
		Global,
		Capture,
		DebugLine,
		Function
	};
}

#endif