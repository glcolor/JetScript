#include "Value.h"
#include "JetContext.h"

using namespace Jet;
#undef Yield

#ifdef _DEBUG
#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

Generator::Generator(JetContext* context, Closure* closure, unsigned int args)
{
	state = GeneratorState::Suspended;
	this->closure = closure;

	//need to push args onto the stack
	this->stack = new Value[closure->prototype->locals];

	//pass in arguments
	if (args <= closure->prototype->args)
	{
		for (int i = closure->prototype->args-1; i >= 0; i--)
		{
			if (i < (int)args)
				stack[i] = context->stack.Pop();
			else
				stack[i] = Value();
		}
	}
	else if (closure->prototype->vararg)
	{
		stack[closure->prototype->locals-1] = context->NewArray();
		auto arr = &stack[closure->prototype->locals-1]._array->data;
		arr->resize(args - closure->prototype->args);
		for (int i = (int)args - 1; i >= 0; i--)
		{
			if (i < (int)closure->prototype->args)
				stack[i] = context->stack.Pop();
			else
				(*arr)[i] = context->stack.Pop();
		}
	}
	else
	{
		for (int i = (int)args - 1; i >= 0; i--)
		{
			if (i < (int)closure->prototype->args)
				stack[i] = context->stack.Pop();
			else
				context->stack.Pop();
		}
	}

	this->curiptr = 0;//set current position to start of function
}

void Generator::Yield(JetContext* context, unsigned int iptr)
{
	this->state = GeneratorState::Suspended;
	//store the iptr
	this->curiptr = iptr+1;

	this->lastyielded = context->stack.Peek();

	//store stack
	for (unsigned int i = 0; i < this->closure->prototype->locals; i++)
		this->stack[i] = context->sptr[i];
}

unsigned int Generator::Resume(JetContext* context)
{
	if (this->state == GeneratorState::Dead)
		throw RuntimeException("Cannot resume dead generator");
	if (this->state == GeneratorState::Running)
		throw RuntimeException("Cannot resume active generator");

	this->state = GeneratorState::Running;

	//restore stack
	for (unsigned int i = 0; i < this->closure->prototype->locals; i++)
		context->sptr[i] = this->stack[i];

	if (this->curiptr == 0)
		context->stack.Pop();

	return this->curiptr;
}

//---------------------------------------------------------------------
Jet::Value Jet::Value::Empty;
Jet::Value Jet::Value::Zero(0);
Jet::Value Jet::Value::One(1);

Value::Value()
{
	this->type = ValueType::Null;
	this->int_value = 0;
}

Value::Value(JetString* str)
{
	if (str == 0)
		return;

	type = ValueType::String;
	length = strlen(str->data);
	_string = str;
}

Value::Value(JetObject* obj)
{
	this->type = ValueType::Object;
	this->_object = obj;
}

Value::Value(JetArray* arr)
{
	type = ValueType::Array;
	this->_array = arr;
}

Value::Value(double val)
{
	type = ValueType::Real;
	value = val;
}

Value::Value(int val)
{
	type = ValueType::Int;
	int_value = val;
}

Value::Value(int64_t val)
{
	type = ValueType::Int;
	int_value = val;
}

Value::Value(JetNativeFunc a)
{
	type = ValueType::NativeFunction;
	func = a;
}

Value::Value(Closure* func)
{
	type = ValueType::Function;
	_function = func;
}

Value::Value(JetUserdata* userdata, JetObject* prototype)
{
	this->type = ValueType::Userdata;
	this->_userdata = userdata;
	this->_userdata->prototype = prototype;
}

JetObject* Value::GetPrototype()
{
	//add defaults for string and array
	switch (type)
	{
	case ValueType::Array:
		return 0;
	case ValueType::Object:
		return this->_object->prototype;
	case ValueType::String:
		return 0;
	case ValueType::Userdata:
		return this->_userdata->prototype;
	default:
		return 0;
	}
}

void Value::AddRef()
{
	switch (type)
	{
	case ValueType::Array:
	case ValueType::Object:
		if (this->_object->refcount == 0)
			this->_object->context->gc.nativeRefs.push_back(*this);

	case ValueType::String:
	case ValueType::Userdata:
		if (this->_object->refcount == 255)
			throw RuntimeException("Tried to addref when count was at the maximum of 255!");

		this->_object->refcount++;
		break;
	case ValueType::Function:
		if (this->_function->refcount == 0)
			this->_function->prototype->context->gc.nativeRefs.push_back(*this);

		if (this->_function->refcount == 255)
			throw RuntimeException("Tried to addref when count was at the maximum of 255!");

		this->_function->refcount++;
	}
}

void Value::Release()
{
	switch (type)
	{
	case ValueType::String:
	case ValueType::Userdata:
		if (this->_object->refcount == 0)
			throw RuntimeException("Tried to subtract from reference count of 0!");
		this->_object->refcount--;

		break;
	case ValueType::Array:
	case ValueType::Object:
		if (this->_object->refcount == 0)
			throw RuntimeException("Tried to subtract from reference count of 0!");
		this->_object->refcount--;

		if (this->_object->refcount == 0)
		{
			//remove me from the list
			//swap this and the last then remove the last
			JetContext* context = this->_object->context;
			if (context->gc.nativeRefs.size() > 1)
			{
				for (unsigned int i = 0; i < context->gc.nativeRefs.size(); i++)
				{
					if (context->gc.nativeRefs[i] == *this)
					{
						context->gc.nativeRefs[i] = context->gc.nativeRefs[context->gc.nativeRefs.size()-1];
						break;
					}
				}
			}
			this->_object->context->gc.nativeRefs.pop_back();
		}
		break;
	case ValueType::Function:
		if (this->_function->refcount == 0)
			throw RuntimeException("Tried to subtract from reference count of 0!");

		this->_function->refcount--;

		if (this->_function->refcount == 0)
		{
			//remove me from the list
			//swap this and the last then remove the last
			JetContext* context = this->_function->prototype->context;
			if (context->gc.nativeRefs.size() > 1)
			{
				for (unsigned int i = 0; i < context->gc.nativeRefs.size(); i++)
				{
					if (context->gc.nativeRefs[i] == *this)
					{
						context->gc.nativeRefs[i] = context->gc.nativeRefs[context->gc.nativeRefs.size()-1];
						break;
					}
				}
			}
			context->gc.nativeRefs.pop_back();
		}
	}
}

std::string Value::ToString(int depth) const
{
	switch(this->type)
	{
	case ValueType::Null:
		return "Null";
	case ValueType::Int:
		return std::to_string(this->int_value);
	case ValueType::Real:
		return std::to_string(this->value);
	case ValueType::String:
		return this->_string->data;
	case ValueType::Function:
		return "[Function "+this->_function->prototype->name+" " + std::to_string((unsigned int)this->_function)+"]";
	case ValueType::NativeFunction:
		return "[NativeFunction "+std::to_string((unsigned int)this->func)+"]";
	case ValueType::Array:
		{
			std::string str = "[\n";

			if (depth++ > 3)
				return "[Array " + std::to_string((int)this->_array)+"]";

			int i = 0;
			for (auto ii: this->_array->data)
			{
				str += "\t";
				str += std::to_string(i++);
				str += " = ";
				str += ii.ToString(depth) + "\n";
			}
			str += "]";
			return str;
		}
	case ValueType::Object:
		{
			std::string str = "{\n";

			if (depth++ > 3)
				return "[Object " + std::to_string((int)this->_object)+"]";

			for (auto ii: *this->_object)
			{
				str += "\t";
				str += ii.first.ToString(depth);
				str += " = ";
				str += ii.second.ToString(depth) + "\n";
			}
			str += "}";
			return str;
		}
	case ValueType::Userdata:
		{
			return "[Userdata "+std::to_string((int)this->_userdata)+"]";
		}
	default:
		return "";
	}
}

void Value::SetPrototype(JetObject* obj)
{
	switch (this->type)
	{
	case ValueType::Object:
		this->_object->prototype = obj;
	case ValueType::Userdata:
		this->_userdata->prototype = obj;
	default:
		throw RuntimeException("Cannot set prototype of non-object or non-userdata!");
	}
}

Value Value::CallMetamethod(JetObject* table, const char* name, const Value* other)
{
	auto node = table->prototype->findNode(name);
	if (node == 0)
	{
		auto obj = table->prototype;
		while(obj)
		{
			node = obj->findNode(name);
			if (node)
				break;
			obj = obj->prototype;
		}
	}

	if (node)
	{
		Value args[2];
		args[0] = *this;
		if (other)
			args[1] = *other;
		return table->prototype->context->Call(&node->second, (Value*)&args, other ? 2 : 1);
	}

	throw RuntimeException("Cannot " + (std::string)(name+1) + " two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other->type]);
}

Value Value::CallMetamethod(const char* name, const Value* other)
{
	auto node = this->_object->prototype->findNode(name);
	if (node == 0)
	{
		auto obj = this->_object->prototype;
		while(obj)
		{
			node = obj->findNode(name);
			if (node)
				break;
			obj = obj->prototype;
		}
	}

	if (node)
	{
		Value args[2];
		args[0] = *this;
		if (other)
			args[1] = *other;
		return this->_object->prototype->context->Call(&node->second, (Value*)&args, other ? 2 : 1);
	}

	throw RuntimeException("Cannot " + (std::string)(name+1) + " two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other->type]);
}

bool Value::TryCallMetamethod(const char* name, const Value* iargs, int numargs, Value* out) const
{
	auto node = this->_object->prototype->findNode(name);
	if (node == 0)
	{
		auto obj = this->_object->prototype;
		while(obj)
		{
			node = obj->findNode(name);
			if (node)
				break;
			obj = obj->prototype;
		}
	}

	if (node)
	{
		//fix this not working with arguments > 1
		Value* args = (Value*)alloca(sizeof(Value)*(numargs+1));
		//Value args[2];
		args[numargs] = *this;
		for (int i = 0; i < numargs; i++)
			args[i] = iargs[i];

		//help, calling this derps up curframe
		*out = this->_object->prototype->context->Call(&node->second, args, numargs+1);
		return true;
	}
	return false;
}

Value Value::operator()(JetContext* context, Value* v, int args)
{
	return context->Call(this, v, args);
}

Value Value::Call(Value* v, int args)
{
	if (this->type == ValueType::Function)
	{
		return this->_function->prototype->context->Call(this, v, args);
	}
	else if (this->type == ValueType::NativeFunction)
	{
		throw RuntimeException("Not implemented!");
	}

	throw RuntimeException("Cannot call non function!");
}

bool Value::operator== (const Value& other) const
{
	if (other.type != this->type)
		return false;

	switch (this->type)
	{
	case ValueType::Int:
		return other.int_value == this->int_value;
	case ValueType::Real:
		return other.value == this->value;
	case ValueType::Array:
		return other._array == this->_array;
	case ValueType::Function:
		return other._function == this->_function;
	case ValueType::NativeFunction:
		return other.func == this->func;
	case ValueType::String:
		return strcmp(other._string->data, this->_string->data) == 0;
	case ValueType::Null:
		return true;
	case ValueType::Object:
		return other._object == this->_object;
	case ValueType::Userdata:
		return other._userdata == this->_userdata;
	}
	return false;
}

Value& Value::operator[] (int64_t key)
{
	switch (type)
	{
	case ValueType::Array:
		return this->_array->data[(size_t)key];
	case ValueType::Object:
		return (*this->_object)[key];
	default:
		throw RuntimeException("Cannot index type " + (std::string)ValueTypes[(int)this->type]);
	}
}

Value& Value::operator[] (const char* key)
{
	switch (type)
	{
	case ValueType::Object:
		{
			return (*this->_object)[key];
		}
	default:
		throw RuntimeException("Cannot index type " + (std::string)ValueTypes[(int)this->type]);
	}
}

Value& Value::operator[] (const Value& key)
{
	switch (type)
	{
	case ValueType::Array:
		{
			return this->_array->data[(int)key.value];
		}
	case ValueType::Object:
		{
			return (*this->_object)[key];
		}
	default:
		throw RuntimeException("Cannot index type " + (std::string)ValueTypes[(int)this->type]);
	}
}

Value Value::operator+( const Value &other )
{
	switch(this->type)
	{
	case ValueType::Int:
		if (other.type == ValueType::Real)
			return Value((double)int_value + other.value);
		else if (other.type == ValueType::Int)
			return Value(int_value + other.int_value);
		else if (other.type == ValueType::String)
		{
			if (other._string->context == nullptr) return *this;
			std::string str = this->ToString() + std::string(other._string->data);
			return other._string->context->NewString(str.c_str(), true);
		}
		break;
	case ValueType::Real:
		if (other.type == ValueType::Real)
			return Value(value + other.value);
		else if (other.type == ValueType::Int)
			return Value(value + (double)other.int_value);
		else if (other.type == ValueType::String)
		{
			if (other._string->context == nullptr) return *this;
			std::string str = this->ToString() + std::string(other._string->data);
			return other._string->context->NewString(str.c_str(), true);
		}
		break;
	case ValueType::String:
	{
		if (this->_string->context == nullptr) return *this;
		std::string str = std::string(this->_string->data) + other.ToString();
		return this->_string->context->NewString(str.c_str(),true);
		break;
	}
	case ValueType::Userdata:
		if (this->_userdata->prototype)
			return this->CallMetamethod(this->_userdata->prototype, "_add", &other);
		break;
	case ValueType::Object:
		if (this->_object->prototype)
			return this->CallMetamethod("_add", &other);
		//throw JetRuntimeException("Cannot Add A String");
		//if (other.type == ValueType::String)
		//return Value((std::string(other.string.data) + std::string(this->string.data)).c_str());
		//else
		//return Value((other.ToString() + std::string(this->string.data)).c_str());
	}

	throw RuntimeException("Cannot add two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator-( const Value &other )
{
	switch(this->type)
	{
	case ValueType::Int:
		if (other.type == ValueType::Real)
			return Value((double)int_value - other.value);
		else if (other.type == ValueType::Int)
			return Value(int_value - other.int_value);
		break;
	case ValueType::Real:
		if (other.type == ValueType::Real)
			return Value(value - other.value);
		else if (other.type == ValueType::Int)
			return Value(value - (double)other.int_value);
		break;
	case ValueType::Userdata:
		if (this->_userdata->prototype)
			return this->CallMetamethod(this->_userdata->prototype, "_sub", &other);
		break;
	case ValueType::Object:
		if (this->_object->prototype)
			return this->CallMetamethod("_sub", &other);
		break;
	}

	throw RuntimeException("Cannot subtract two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator*( const Value &other )
{
	switch(this->type)
	{
	case ValueType::Int:
		if (other.type == ValueType::Real)
			return Value((double)int_value * other.value);
		else if (other.type == ValueType::Int)
			return Value(int_value * other.int_value);
		break;
	case ValueType::Real:
		if (other.type == ValueType::Real)
			return Value(value * other.value);
		else if (other.type == ValueType::Int)
			return Value(value * (double)other.int_value);
		break;
	case ValueType::Userdata:
		if (this->_userdata->prototype)
			return this->CallMetamethod(this->_userdata->prototype, "_mul", &other);
		break;
	case ValueType::Object:
		if (this->_object->prototype)
			return this->CallMetamethod("_mul", &other);
		break;
	}

	throw RuntimeException("Cannot multiply two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator/( const Value &other )
{
	switch(this->type)
	{
	case ValueType::Int:
		if (other.type == ValueType::Real)
			return Value((double)int_value / other.value);
		else if (other.type == ValueType::Int)
			return Value((double)int_value / (double)other.int_value);
		break;
	case ValueType::Real:
		if (other.type == ValueType::Real)
			return Value(value / other.value);
		else if (other.type == ValueType::Int)
			return Value(value / (double)other.int_value);
		break;
	case ValueType::Userdata:
		if (this->_userdata->prototype)
			return this->CallMetamethod(this->_userdata->prototype, "_div", &other);
		break;
	case ValueType::Object:
		if (this->_object->prototype)
			return this->CallMetamethod("_div", &other);
		break;
	}

	throw RuntimeException("Cannot divide two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator%( const Value &other )
{
	switch(this->type)
	{
	case ValueType::Int:
		if (other.type == ValueType::Real)
		{
			return Value(int_value % (int64_t)other.value);
		}
		else if (other.type == ValueType::Int)
		{
			return Value(int_value % other.int_value);
		}
		break;
	case ValueType::Real:
		if (other.type == ValueType::Real)
		{
			return Value((int64_t)value % (int64_t)other.value);
		}
		else if (other.type == ValueType::Int)
		{
			return Value((int64_t)value % other.int_value);
		}
		break;
	case ValueType::Userdata:
		if (this->_userdata->prototype)
			return this->CallMetamethod(this->_userdata->prototype, "_mod", &other);
		break;
	case ValueType::Object:
		if (this->_object->prototype)
			return this->CallMetamethod("_mod", &other);
		break;
	}

	throw RuntimeException("Cannot modulus two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator|( const Value &other )
{
	switch(this->type)
	{
	case ValueType::Int:
		if (other.type == ValueType::Real)
			return Value(int_value |(int64_t) other.value);
		else if (other.type == ValueType::Int)
			return Value(int_value | other.int_value);
		break;
	case ValueType::Real:
		if (other.type == ValueType::Real)
			return Value((int64_t)value | (int64_t)other.value);
		else if (other.type == ValueType::Int)
			return Value((int64_t)value | other.int_value);
		break;
	case ValueType::Userdata:
		if (this->_userdata->prototype)
			return this->CallMetamethod(this->_userdata->prototype, "_bor", &other);
		break;
	case ValueType::Object:
		if (this->_object->prototype)
			return this->CallMetamethod("_bor", &other);
		break;
	}

	throw RuntimeException("Cannot binary or two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator&( const Value &other )
{
	switch(this->type)
	{
	case ValueType::Int:
		if (other.type == ValueType::Real)
			return Value(int_value & (int64_t)other.value);
		else if (other.type == ValueType::Int)
			return Value(int_value & other.int_value);
		break;
	case ValueType::Real:
		if (other.type == ValueType::Real)
			return Value((int64_t)value & (int64_t)other.value);
		else if (other.type == ValueType::Int)
			return Value((int64_t)value & other.int_value);
		break;
	case ValueType::Userdata:
		if (this->_userdata->prototype)
			return this->CallMetamethod(this->_userdata->prototype, "_band", &other);
		break;
	case ValueType::Object:
		if (this->_object->prototype)
			return this->CallMetamethod("_band", &other);
		break;
	}

	throw RuntimeException("Cannot binary and two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator^( const Value &other )
{
	switch(this->type)
	{
	case ValueType::Int:
		if (other.type == ValueType::Real)
			return Value(int_value ^ (int64_t)other.value);
		else if (other.type == ValueType::Int)
			return Value(int_value ^ other.int_value);
		break;
	case ValueType::Real:
		if (other.type == ValueType::Real)
			return Value((int64_t)value ^ (int64_t)other.value);
		else if (other.type == ValueType::Int)
			return Value((int64_t)value ^ other.int_value);
		break;
	case ValueType::Userdata:
		if (this->_userdata->prototype)
			return this->CallMetamethod(this->_userdata->prototype, "_xor", &other);
		break;
	case ValueType::Object:
		if (this->_object->prototype)
			return this->CallMetamethod("_xor", &other);
		break;
	}

	throw RuntimeException("Cannot xor two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator<<( const Value &other )
{
	switch(this->type)
	{
	case ValueType::Int:
		if (other.type == ValueType::Real)
			return Value(int_value << (int64_t)other.value);
		else if (other.type == ValueType::Int)
			return Value(int_value << other.int_value);
		break;
	case ValueType::Real:
		if (other.type == ValueType::Real)
			return Value((int64_t)value << (int64_t)other.value);
		else if (other.type == ValueType::Int)
			return Value((int64_t)value << other.int_value);
		break;
	case ValueType::Userdata:
		if (this->_userdata->prototype)
			return this->CallMetamethod(this->_userdata->prototype, "_ls", &other);
		break;
	case ValueType::Object:
		if (this->_object->prototype)
			return this->CallMetamethod("_ls", &other);
		break;
	}

	throw RuntimeException("Cannot left-shift two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator>>( const Value &other )
{
	switch(this->type)
	{
	case ValueType::Int:
		if (other.type == ValueType::Real)
			return Value(int_value >> (int64_t)other.value);
		else if (other.type == ValueType::Int)
			return Value(int_value >> other.int_value);
		break;
	case ValueType::Real:
		if (other.type == ValueType::Real)
			return Value((int64_t)value >> (int64_t)other.value);
		else if (other.type == ValueType::Int)
			return Value((int64_t)value >> other.int_value);
		break;
	case ValueType::Userdata:
		if (this->_userdata->prototype)
			return this->CallMetamethod(this->_userdata->prototype, "_rs", &other);
		break;
	case ValueType::Object:
		if (this->_object->prototype)
			return this->CallMetamethod("_rs", &other);
		break;
	}

	throw RuntimeException("Cannot right-shift two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};


void Value::operator+=(const Value &other)
{
	switch (this->type)
	{
		case ValueType::Int:
		{
			switch (other.type)
			{
				case ValueType::Real:
				{
					type = ValueType::Real;
					value = (double)int_value + other.value;
					return;
				}
				case ValueType::Int:
				{
					int_value += other.int_value;
					return;
				}
				case ValueType::String:
				{
					if (other._string->context == nullptr) return;
					std::string str = this->ToString() + std::string(other._string->data);
					type = ValueType::String;
					*this = other._string->context->NewString(str.c_str(), true);
					return;
				}
			}
			break;
		}
		case ValueType::Real:
		{
			switch (other.type)
			{
				case ValueType::Real:
				{
					value += other.value;
					break;
				}
				case ValueType::Int:
				{
					value += (double)other.int_value;
					return;
				}
				case ValueType::String:
				{
					if (other._string->context == nullptr) return;
					std::string str = this->ToString() + std::string(other._string->data);
					*this = other._string->context->NewString(str.c_str(), true);
					return;
				}
			}
			break;
		}
		case ValueType::String:
		{
			if (this->_string->context == nullptr) return;
			std::string str = std::string(this->_string->data) + other.ToString();
			*this = this->_string->context->NewString(str.c_str(), true);
			return;
			break;
		}
		case ValueType::Userdata:
		{
			if (this->_userdata->prototype)
			{
				this->CallMetamethod(this->_userdata->prototype, "_add", &other);
				return;
			}
			break;
		}
		case ValueType::Object:
		{
			if (this->_object->prototype)
			{
				this->CallMetamethod("_add", &other);
				return;
			}
		}
	}

	throw RuntimeException("Cannot add two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

void Value::operator-=(const Value &other)
{
	switch (this->type)
	{
		case ValueType::Int:
		{
			if (other.type == ValueType::Real)
			{
				type = ValueType::Real;
				value = (double)int_value - other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				int_value -= other.int_value;
				return;
			}
			break;
		}
		case ValueType::Real:
		{
			if (other.type == ValueType::Real)
			{
				value -= other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				value -= (double)other.value;
				return;
			}
			break;
		}
		case ValueType::Userdata:
		{
			if (this->_userdata->prototype)
			{
				this->CallMetamethod(this->_userdata->prototype, "_sub", &other);
				return;
			}
			break;
		}
		case ValueType::Object:
		{
			if (this->_object->prototype)
			{
				this->CallMetamethod("_sub", &other);
				return;
			}
			break;
		}
	}

	throw RuntimeException("Cannot subtract two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

void Value::operator*=(const Value &other)
{
	switch (this->type)
	{
		case ValueType::Int:
			if (other.type == ValueType::Real)
			{
				type = ValueType::Real;
				value = (double)int_value * other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				int_value *= other.int_value;
				return;
			}
			break;
		case ValueType::Real:
			if (other.type == ValueType::Real)
			{
				value *= other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				value *= (double)other.value;
				return;
			}
			break;
		case ValueType::Userdata:
			if (this->_userdata->prototype)
			{
				this->CallMetamethod(this->_userdata->prototype, "_mul", &other);
				return;
			}
			break;
		case ValueType::Object:
			if (this->_object->prototype)
			{
				this->CallMetamethod("_mul", &other);
				return;
			}
			break;
	}

	throw RuntimeException("Cannot multiply two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

void Value::operator/=(const Value &other)
{
	switch (this->type)
	{
		case ValueType::Int:
			if (other.type == ValueType::Real)
			{
				type = ValueType::Real;
				value = (double)int_value/other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				int_value /= other.int_value;
				return;
			}
			break;
		case ValueType::Real:
			if (other.type == ValueType::Real)
			{
				value /=other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				value /= (double)other.value;
				return;
			}
			break;
		case ValueType::Userdata:
			if (this->_userdata->prototype)
			{
				this->CallMetamethod(this->_userdata->prototype, "_div", &other);
				return;
			}
			break;
		case ValueType::Object:
			if (this->_object->prototype)
			{
				this->CallMetamethod("_div", &other);
				return;
			}
			break;
	}

	throw RuntimeException("Cannot divide two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

void Value::operator%=(const Value &other)
{
	switch (this->type)
	{
		case ValueType::Int:
			if (other.type == ValueType::Real)
			{
				type = ValueType::Real;
				value = fmod((double)int_value, other.value);
				return;
			}
			else if (other.type == ValueType::Int)
			{
				int_value %= other.int_value;
				return;
			}
			break;
		case ValueType::Real:
			if (other.type == ValueType::Real)
			{
				value = fmod(value,other.value);
				return;
			}
			else if (other.type == ValueType::Int)
			{
				value = fmod(value, (double)other.int_value);
				return;
			}
			break;
		case ValueType::Userdata:
			if (this->_userdata->prototype)
			{
				this->CallMetamethod(this->_userdata->prototype, "_mod", &other);
				return;
			}
			break;
		case ValueType::Object:
			if (this->_object->prototype)
			{
				this->CallMetamethod("_mod", &other);
				return;
			}
			break;
	}

	throw RuntimeException("Cannot modulus two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

void Value::operator|=(const Value &other)
{
	switch (this->type)
	{
		case ValueType::Int:
			if (other.type == ValueType::Real)
			{
				int_value |= (int64_t)other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				int_value |= other.int_value;
				return;
			}
			break;
		case ValueType::Real:
			if (other.type == ValueType::Real)
			{
				type = ValueType::Int;
				int_value = (int64_t)value | (int64_t)other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				type = ValueType::Int;
				int_value = (int64_t)value | other.int_value;
				return;
			}
			break;
		case ValueType::Userdata:
			if (this->_userdata->prototype)
			{
				this->CallMetamethod(this->_userdata->prototype, "_or", &other);
				return;
			}
			break;
		case ValueType::Object:
			if (this->_object->prototype)
			{
				this->CallMetamethod("_or", &other);
				return;
			}
			break;
	}

	throw RuntimeException("Cannot binary or two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

void Value::operator&=(const Value &other)
{
	switch (this->type)
	{
		case ValueType::Int:
			if (other.type == ValueType::Real)
			{
				int_value &= (int64_t)other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				int_value &= other.int_value;
				return;
			}
			break;
		case ValueType::Real:
			if (other.type == ValueType::Real)
			{
				type = ValueType::Int;
				int_value = (int64_t)value & (int64_t)other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				type = ValueType::Int;
				int_value = (int64_t)value & other.int_value;
				return;
			}
			break;
		case ValueType::Userdata:
			if (this->_userdata->prototype)
			{
				this->CallMetamethod(this->_userdata->prototype, "_and", &other);
				return;
			}
			break;
		case ValueType::Object:
			if (this->_object->prototype)
			{
				this->CallMetamethod("_and", &other);
				return;
			}
			break;
	}

	throw RuntimeException("Cannot binary and two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

void Value::operator^=(const Value &other)
{
	switch (this->type)
	{
		case ValueType::Int:
			if (other.type == ValueType::Real)
			{
				int_value ^= (int64_t)other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				int_value ^= other.int_value;
				return;
			}
			break;
		case ValueType::Real:
			if (other.type == ValueType::Real)
			{
				type = ValueType::Int;
				int_value = (int64_t)value ^ (int64_t)other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				type = ValueType::Int;
				int_value = (int64_t)value ^ other.int_value;
				return;
			}
			break;
		case ValueType::Userdata:
			if (this->_userdata->prototype)
			{
				this->CallMetamethod(this->_userdata->prototype, "_xor", &other);
				return;
			}
			break;
		case ValueType::Object:
			if (this->_object->prototype)
			{
				this->CallMetamethod("_xor", &other);
				return;
			}
			break;
	}

	throw RuntimeException("Cannot xor two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

void Value::operator<<=(const Value &other)
{
	switch (this->type)
	{
		case ValueType::Int:
			if (other.type == ValueType::Real)
			{
				int_value <<= (int64_t)other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				int_value <<= other.int_value;
				return;
			}
			break;
		case ValueType::Real:
			if (other.type == ValueType::Real)
			{
				type = ValueType::Int;
				int_value = (int64_t)value << (int64_t)other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				type = ValueType::Int;
				int_value = (int64_t)value << other.int_value;
				return;
			}
			break;
		case ValueType::Userdata:
			if (this->_userdata->prototype)
			{
				this->CallMetamethod(this->_userdata->prototype, "_ls", &other);
				return;
			}
			break;
		case ValueType::Object:
			if (this->_object->prototype)
			{
				this->CallMetamethod("_ls", &other);
				return;
			}
			break;
	}

	throw RuntimeException("Cannot left-shift two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

void Value::operator>>=(const Value &other)
{
	switch (this->type)
	{
		case ValueType::Int:
			if (other.type == ValueType::Real)
			{
				int_value >>= (int64_t)other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				int_value >>= other.int_value;
				return;
			}
			break;
		case ValueType::Real:
			if (other.type == ValueType::Real)
			{
				type = ValueType::Int;
				int_value=(int64_t)value >> (int64_t)other.value;
				return;
			}
			else if (other.type == ValueType::Int)
			{
				type = ValueType::Int;
				int_value = (int64_t)value >> other.int_value;
				return;
			}
			break;
		case ValueType::Userdata:
			if (this->_userdata->prototype)
			{
				this->CallMetamethod(this->_userdata->prototype, "_rs", &other);
				return;
			}
			break;
		case ValueType::Object:
			if (this->_object->prototype)
			{
				this->CallMetamethod("_rs", &other);
				return;
			}
			break;
	}

	throw RuntimeException("Cannot right-shift two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator~()
{
	switch(this->type)
	{
	case ValueType::Int:
		return Value(~int_value);
	case ValueType::Real:
		return Value(~(int64_t)value);
	case ValueType::Userdata:
		if (this->_userdata->prototype)
			return this->CallMetamethod(this->_userdata->prototype, "_bnot", 0);
		break;
	case ValueType::Object:
		if (this->_object->prototype)
			return this->CallMetamethod("_bnot", 0);
		break;
	}

	throw RuntimeException("Cannot binary complement non-numeric type! " + (std::string)ValueTypes[(int)this->type]);
};

Value Value::operator-()
{
	switch(this->type)
	{
	case ValueType::Int:
		return Value(-int_value);
	case ValueType::Real:
		return Value(-value);
	case ValueType::Userdata:
		if (this->_userdata->prototype)
			return this->CallMetamethod(this->_userdata->prototype, "_neg", 0);
		break;
	case ValueType::Object:
		if (this->_object->prototype)
			return this->CallMetamethod("_neg", 0);
		break;
	}

	throw RuntimeException("Cannot negate non-numeric type! " + (std::string)ValueTypes[(int)this->type]);
}

void Jet::Value::Negate()
{
	switch (this->type)
	{
		case ValueType::Int:
			int_value = -int_value;
			return;
		case ValueType::Real:
			value = -value;
			return;
		case ValueType::Userdata:
			if (this->_userdata->prototype)
			{
				this->CallMetamethod(this->_userdata->prototype, "_neg", 0);
				return;
			}
			break;
		case ValueType::Object:
			if (this->_object->prototype)
			{
				this->CallMetamethod("_neg", 0);
				return;
			}
			break;
	}

	throw RuntimeException("Cannot negate non-numeric type! " + (std::string)ValueTypes[(int)this->type]);
}

void Jet::Value::Increase()
{
	switch (this->type)
	{
		case ValueType::Int:
			++int_value;
			return;
		case ValueType::Real:
			++value;
			return;
		case ValueType::Userdata:
			if (this->_userdata->prototype)
			{
				this->CallMetamethod(this->_userdata->prototype, "_incr", 0);
				return;
			}
			break;
		case ValueType::Object:
			if (this->_object->prototype)
			{
				this->CallMetamethod("_incr", 0);
				return;
			}
			break;
	}

	throw RuntimeException("Cannot incr non-numeric type! " + (std::string)ValueTypes[(int)this->type]);
}

void Jet::Value::Decrease()
{
	switch (this->type)
	{
	case ValueType::Int:
	{
		--int_value;
		return;
	}
	case ValueType::Real:
	{
		--value;
		return;
	}
	case ValueType::Userdata:
		if (this->_userdata->prototype)
		{
			this->CallMetamethod(this->_userdata->prototype, "_decr", 0);
			return;
		}
		break;
	case ValueType::Object:
		if (this->_object->prototype)
		{
			this->CallMetamethod("_decr", 0);
			return;
		}
		break;
	}

	throw RuntimeException("Cannot decr non-numeric type! " + (std::string)ValueTypes[(int)this->type]);
}
