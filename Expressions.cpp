#include "Expressions.h"
#include "Compiler.h"
#include "Parser.h"

using namespace Jet;

void PrefixExpression::Compile(CompilerContext* context)
{
	context->Line(this->_operator.line);

	right->Compile(context);

	context->UnaryOperation(this->_operator.type);

	switch (this->_operator.type)
	{
	case TokenType::BNot:
	case TokenType::Minus:
		{
			if (dynamic_cast<BlockExpression*>(this->Parent))
				context->Pop();
			break;
		}
	default://operators that also do a store, like ++ and --
		{
			auto location = dynamic_cast<IStorableExpression*>(this->right);
			if (location)
			{
				if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
					context->Duplicate();

				location->CompileStore(context);
			}
			else if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
				context->Pop();
		}
	}
}

void PostfixExpression::Compile(CompilerContext* context)
{
	context->Line(this->_operator.line);

	left->Compile(context);

	if (dynamic_cast<BlockExpression*>(this->Parent) == 0 && dynamic_cast<IStorableExpression*>(this->left))
		context->Duplicate();

	context->UnaryOperation(this->_operator.type);

	if (dynamic_cast<IStorableExpression*>(this->left))
		dynamic_cast<IStorableExpression*>(this->left)->CompileStore(context);
	else if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
		context->Pop();
}

void IndexExpression::Compile(CompilerContext* context)
{
	context->Line(token.line);

	left->Compile(context);
	//if the index is constant compile to a special instruction carying that constant
	if (auto string = dynamic_cast<StringExpression*>(index))
	{
		context->LoadIndex(string->GetValue().c_str());
	}
	else
	{
		index->Compile(context);
		context->LoadIndex();
	}

	if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
		context->Pop();
}

void IndexExpression::CompileStore(CompilerContext* context)
{
	context->Line(token.line);

	left->Compile(context);
	//if the index is constant compile to a special instruction carying that constant
	if (auto string = dynamic_cast<StringExpression*>(index))
	{
		context->StoreIndex(string->GetValue().c_str());
	}
	else
	{
		index->Compile(context);
		context->StoreIndex();
	}
}

void ObjectExpression::Compile(CompilerContext* context)
{
	int count = 0;
	if (this->inits)
	{
		count = (int)this->inits->size();
		//set these up
		for (auto ii: *this->inits)
		{
			context->String(ii.first);
			ii.second->Compile(context);
		}
	}
	context->NewObject(count);

	//pop off if we dont need the result
	if (dynamic_cast<BlockExpression*>(this->Parent))
		context->Pop();
}

void ArrayExpression::Compile(CompilerContext* context)
{
	int count = (int)this->initializers.size();
	for (auto i: this->initializers)
		i->Compile(context);

	context->NewArray(count);

	//pop off if we dont need the result
	if (dynamic_cast<BlockExpression*>(this->Parent))
		context->Pop();
}

void StringExpression::Compile(CompilerContext* context)
{
	context->String(this->value);

	//pop off if we dont need the result
	if (dynamic_cast<BlockExpression*>(this->Parent))
		context->Pop();
}

void NullExpression::Compile(CompilerContext* context)
{
	context->Null();

	//pop off if we dont need the result
	if (dynamic_cast<BlockExpression*>(this->Parent))
		context->Pop();
}

void Jet::IntNumberExpression::Compile(CompilerContext* context)
{
	context->IntNumber(this->value);

	//pop off if we dont need the result
	if (dynamic_cast<BlockExpression*>(this->Parent))
		context->Pop();
}

void RealNumberExpression::Compile(CompilerContext* context)
{
	context->RealNumber(this->value);

	//pop off if we dont need the result
	if (dynamic_cast<BlockExpression*>(this->Parent))
		context->Pop();
}

void SwapExpression::Compile(CompilerContext* context)
{
	right->Compile(context);
	left->Compile(context);

	if (auto rstorable = dynamic_cast<IStorableExpression*>(this->right))
		rstorable->CompileStore(context);

	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();

	if (auto lstorable = dynamic_cast<IStorableExpression*>(this->left))
		lstorable->CompileStore(context);
}

void AssignExpression::Compile(CompilerContext* context)
{
	this->right->Compile(context);

	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();//if my parent is not block expression, we need the result, so push it

	if (auto storable = dynamic_cast<IStorableExpression*>(this->left))
		storable->CompileStore(context);
}

void CallExpression::Compile(CompilerContext* context)
{
	context->Line(token.line);

	//need to check if left is a local, or a captured value before looking at globals
	if (dynamic_cast<NameExpression*>(left) && context->IsLocal(dynamic_cast<NameExpression*>(left)->GetName()) == false)
	{
		//push args onto stack
		for (auto i: *args)
			i->Compile(context);

		context->Call(dynamic_cast<NameExpression*>(left)->GetName(), (unsigned int)args->size());
	}
	else// if (dynamic_cast<IStorableExpression*>(left) != 0)
	{
		auto index = dynamic_cast<IndexExpression*>(left);
		if (index && index->token.type == TokenType::Colon)//its a "self" call
		{
			index->left->Compile(context);//push object as the first argument

			//push args onto stack
			for (auto i: *args)
				i->Compile(context);//pushes args

			//compile left I guess?
			left->Compile(context);//pushes function

			//increase number of args
			context->ECall((unsigned int)args->size() + 1);
		}
		else
		{
			//push args onto stack
			for (auto i: *args)
				i->Compile(context);

			//compile left I guess?
			left->Compile(context);

			context->ECall((unsigned int)args->size());
		}
	}
	//else
	//{
	//throw ParserException(token.filename, token.line, "Error: Cannot call an expression that is not a name");
	//}
	//help, how should I handle this for multiple returns
	//pop off return value if we dont need it
	if (dynamic_cast<BlockExpression*>(this->Parent))
		context->Pop();//if my parent is block expression, we dont the result, so pop it
}

void NameExpression::Compile(CompilerContext* context)
{
	//add load variable instruction
	//todo make me detect if this is a local or not
	context->Load(name);

	if (dynamic_cast<BlockExpression*>(this->Parent))
		context->Pop();
}

void OperatorAssignExpression::Compile(CompilerContext* context)
{
	context->Line(token.line);
	//https://dl.dropboxusercontent.com/u/675786/ShareX/2015-02/08_22-33-22.png fix this
	this->left->Compile(context);
	this->right->Compile(context);
	context->BinaryOperation(token.type);

	//insert store here
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();//if my parent is not block expression, we need the result, so push it

	if (auto storable = dynamic_cast<IStorableExpression*>(this->left))
		storable->CompileStore(context);
}

void OperatorExpression::Compile(CompilerContext* context)
{
	context->Line(this->_operator.line);

	if (this->_operator.type == TokenType::And)
	{
		std::string label = "_endand"+context->GetUUID();
		this->left->Compile(context);
		context->JumpFalsePeek(label.c_str());//jump to endand if false
		context->Pop();
		this->right->Compile(context);
		context->Label(label);//put endand label here
		return;
	}

	if (this->_operator.type == TokenType::Or)
	{
		std::string label = "_endor"+context->GetUUID();
		this->left->Compile(context);
		context->JumpTruePeek(label.c_str());//jump to endor if true
		context->Pop();
		this->right->Compile(context);
		context->Label(label);//put endor label here
		return;
	}

	this->left->Compile(context);
	this->right->Compile(context);
	context->BinaryOperation(this->_operator.type);

	//pop off if we dont need the result
	if (dynamic_cast<BlockExpression*>(this->Parent))
		context->Pop();
}

void FunctionExpression::Compile(CompilerContext* context)
{
	context->Line(token.line);

	std::string fname;
	if (name)
		fname = static_cast<NameExpression*>(name)->GetName();
	else
		fname = "_lambda_id_";

	CompilerContext* function = context->AddFunction(fname, (unsigned int)this->args->size(), this->varargs != nullptr);
	//ok, kinda hacky
	int start = (int)context->out.size();

	//ok push locals, in opposite order
	for (unsigned int i = 0; i < this->args->size(); i++)
	{
		auto aname = static_cast<NameExpression*>((*this->args)[i]);
		function->RegisterLocal(aname->GetName());
	}
	if (this->varargs)
		function->RegisterLocal(this->varargs->GetName());

	block->Compile(function);

	//if last instruction was a return, dont insert another one
	if (block->statements.size() > 0)
	{
		if (dynamic_cast<ReturnExpression*>(block->statements.at(block->statements.size()-1)) == 0)
		{
			function->Null();//return nil
			function->Return();
		}
	}
	else
	{
		function->Null();//return nil
		function->Return();
	}

	context->FinalizeFunction(function);

	//only named functions need to be stored here
	if (name)
		context->Store(static_cast<NameExpression*>(name)->GetName());

	//vm will pop off locals when it removes the call stack
}


void Jet::LocalExpression::Compile(CompilerContext* context)
{
	context->Line((*defines)[0].m_Name.line);

	for (auto v : *this->defines)
	{
		//计算表达式的值
		if (v.m_Experssion != nullptr)
		{
			v.m_Experssion->Compile(context);
		}

		//创建变量
		if (context->RegisterLocal(v.m_Name.getText()) == false)
		{
			throw CompilerException(context->filename, v.m_Name.line, "Duplicate Local Variable '" + v.m_Name.text + "'");
		}

		//将数据存储到变量
		if (v.m_Experssion != nullptr)
		{
			context->StoreLocal(v.m_Name.text);
		}
	}
}


void GlobalExpression::Compile(CompilerContext* context)
{
	context->Line((*defines)[0].m_Name.line);

	for (auto v : *this->defines)
	{
		//计算表达式的值
		if (v.m_Experssion != nullptr)
		{
			v.m_Experssion->Compile(context);
		}

		//将数据存储到变量
		if (v.m_Experssion != nullptr)
		{
			context->StoreGlobal(v.m_Name.text);
		}
	}
}

void Jet::ClassExpression::Compile(CompilerContext* context)
{
	printf("class %s 拥有%d个字段，%d个成员函数\n", m_Name.c_str(), (int)m_Fields.size(), (int)m_Functions.size());
	//每个class对应一个以Class为名的Object对象，这个对象包含了class的全部数据，并且根据需要添加默认构造函数。注意：需要考虑继承的问题
	//new class时，构造函数的返回值是一个Object，并将自身的成员变量和成员函数复制给该Object（字段需要clone，函数可以直接复制）
	/*for (auto& i : m_Functions)
	{
	i.second->Compile(context);
	}

	for (auto& i : m_Fields)
	{
	if (i.m_Experssion != nullptr)
	{
	i.m_Experssion->Compile(context);
	}
	}*/
}
