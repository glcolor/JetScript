#ifndef _JET_INSTRUCTIONS_HEADER
#define _JET_INSTRUCTIONS_HEADER

namespace Jet
{
	const static char* Instructions[] = {
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
		"LdNum",
		"LdNull",
		"LdStr",
		"LoadFunction",
		"Jump",
		"JumpTrue",
		"JumpFalse",
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
		"Comment",
		"DebugLine",
		"Function"
	};
	enum class InstructionType
	{
		Add,
		Mul,
		Div,
		Sub,
		Modulus,

		Negate,

		BAnd,
		BOr,
		Xor,
		BNot,
		LeftShift,
		RightShift,

		Eq,
		NotEq,
		Lt,
		Gt,
		LtE,
		GtE,

		Incr,
		Decr,

		Dup,
		Pop,

		LdNum,
		LdNull,
		LdStr,
		LoadFunction,

		Jump,
		JumpTrue,
		JumpFalse,

		NewArray,
		NewObject,

		Store,
		Load,
		//local vars
		LStore,
		LLoad,
		//captured vars
		CStore,
		CLoad,
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
		Comment,
		DebugLine,
		Function
	};
}

#endif