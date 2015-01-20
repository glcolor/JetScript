#include "Value.h"
#include "JetContext.h"

using namespace Jet;

Value::Value()
{
	this->type = ValueType::Null;
}

/*Value(const char* str)
{
if (str == 0)
return;

type = ValueType::NativeString;
length = strlen(str);
_nativestring = (char*)str;
}*/

//plz dont delete my string
//crap, this is gonna be hell to figure out
Value::Value(JetString* str)
{
	if (str == 0)
		return;

	type = ValueType::String;
	length = strlen(str->ptr);
	_string = str;
}

Value::Value(JetObject* obj)
{
	this->type = ValueType::Object;
	this->_object = obj;
	//this->prototype = 0;
}

Value::Value(JetArray* arr)
{
	type = ValueType::Array;
	this->_array = arr;
}

Value::Value(double val)
{
	type = ValueType::Number;
	value = val;
}

Value::Value(int val)
{
	type = ValueType::Number;
	value = val;
}

Value::Value(_JetNativeFunc a)
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
	this->_userdata->ptr.second = prototype;
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
		return this->_userdata->ptr.second;//prototype;
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
			if (this->_object->context->gc.nativeRefs.size() > 1)
			{
				for (int i = 0; i < this->_object->context->gc.nativeRefs.size(); i++)
				{
					if (this->_object->context->gc.nativeRefs[i] == *this)
					{
						this->_object->context->gc.nativeRefs[i] = this->_object->context->gc.nativeRefs[this->_object->context->gc.nativeRefs.size()-1];
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
	}
}

std::string Value::ToString(int depth) const
{
	switch(this->type)
	{
	case ValueType::Null:
		return "Null";
	case ValueType::Number:
		return std::to_string(this->value);
	case ValueType::String:
		return this->_string->ptr;
	case ValueType::Function:
		return "[Function "+this->_function->prototype->name+"]";
	case ValueType::NativeFunction:
		return "[NativeFunction "+std::to_string((unsigned int)this->func)+"]";
	case ValueType::Array:
		{
			std::string str = "[\n";

			if (depth++ > 3)
				return "[Array " + std::to_string((int)this->_array)+"]";

			int i = 0;
			for (auto ii: *this->_array->ptr)
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
		this->_userdata->ptr.second = obj;
	default:
		throw RuntimeException("Cannot set prototype of non-object or non-userdata!");
	}
}

Value Value::CallMetamethod(const char* name, const Value* other)
{
	auto node = this->_object->prototype->findNode(name);
	if (node)
		return this->_object->prototype->context->Call(&node->second, (Value*)other, other ? 1 : 0);
	else
	{
		auto obj = this->_object->prototype;
		while(obj)
		{
			node = obj->findNode(name);
			if (node)
				return this->_object->prototype->context->Call(&node->second, (Value*)other, other ? 1 : 0);
			obj = obj->prototype;
		}
	}

	throw RuntimeException("Cannot " + (std::string)(name+1) + " two non-numeric types! " + (std::string)ValueTypes[(int)other->type] + " and " + (std::string)ValueTypes[(int)this->type]);
}

bool Value::operator== (const Value& other) const
{
	if (other.type != this->type)
		return false;

	switch (this->type)
	{
	case ValueType::Number:
		return other.value == this->value;
	case ValueType::Array:
		return other._array == this->_array;
	case ValueType::Function:
		return other._function == this->_function;
	case ValueType::NativeFunction:
		return other.func == this->func;
	case ValueType::String:
		return strcmp(other._string->ptr, this->_string->ptr) == 0;
	case ValueType::Null:
		return true;
	case ValueType::Object:
		return other._object == this->_object;
	case ValueType::Userdata:
		return other._userdata == this->_userdata;
	}
}

Value& Value::operator[] (int key)
{
	switch (type)
	{
	case ValueType::Array:
		{
			return (*this->_array->ptr)[key];
		}
	case ValueType::Object:
		{
			return (*this->_object)[key];
		}
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
			return (*this->_array->ptr)[(int)key.value];
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
	case ValueType::Number:
		if (other.type == ValueType::Number)
			return Value(value+other.value);
		break;
	case ValueType::Object:
		{
			if (this->_object->prototype)
				return this->CallMetamethod("_add", &other);
			//throw JetRuntimeException("Cannot Add A String");
			//if (other.type == ValueType::String)
			//return Value((std::string(other.string.data) + std::string(this->string.data)).c_str());
			//else
			//return Value((other.ToString() + std::string(this->string.data)).c_str());
		}
	}

	throw RuntimeException("Cannot add two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator-( const Value &other )
{
	if (type == ValueType::Number && other.type == ValueType::Number)
		return Value(value-other.value);
	else if (type == ValueType::Object)
	{
		if (this->_object->prototype)
			return this->CallMetamethod("_sub", &other);
	}

	throw RuntimeException("Cannot subtract two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator*( const Value &other )
{
	if (type == ValueType::Number && other.type == ValueType::Number)
		return Value(value*other.value);
	else if (type == ValueType::Object)
	{
		if (this->_object->prototype)
			return this->CallMetamethod("_mul", &other);
	}

	throw RuntimeException("Cannot multiply two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator/( const Value &other )
{
	if (type == ValueType::Number && other.type == ValueType::Number)
		return Value(value/other.value);
	else if (type == ValueType::Object)
	{
		if (this->_object->prototype)
			return this->CallMetamethod("_div", &other);
	}

	throw RuntimeException("Cannot divide two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator%( const Value &other )
{
	if (type == ValueType::Number && other.type == ValueType::Number)
		return Value((int)value%(int)other.value);
	else if (type == ValueType::Object)
	{
		if (this->_object->prototype)
			return this->CallMetamethod("_mod", &other);
	}

	throw RuntimeException("Cannot modulus two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator|( const Value &other )
{
	if (type == ValueType::Number && other.type == ValueType::Number)
		return Value((int)value|(int)other.value);
	else if (type == ValueType::Object)
	{
		if (this->_object->prototype)
			return this->CallMetamethod("_bor", &other);
	}

	throw RuntimeException("Cannot binary or two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator&( const Value &other )
{
	if (type == ValueType::Number && other.type == ValueType::Number)
		return Value((int)value&(int)other.value);
	else if (type == ValueType::Object)
	{
		if (this->_object->prototype)
			return this->CallMetamethod("_band", &other);
	}

	throw RuntimeException("Cannot binary and two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator^( const Value &other )
{
	if (type == ValueType::Number && other.type == ValueType::Number)
		return Value((int)value^(int)other.value);
	else if (type == ValueType::Object)
	{
		if (this->_object->prototype)
			return this->CallMetamethod("_xor", &other);
	}

	throw RuntimeException("Cannot xor two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator<<( const Value &other )
{
	if (type == ValueType::Number && other.type == ValueType::Number)
		return Value((int)value<<(int)other.value);
	else if (type == ValueType::Object)
	{
		if (this->_object->prototype)
			return this->CallMetamethod("_ls", &other);
	}

	throw RuntimeException("Cannot left-shift two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator>>( const Value &other )
{
	if (type == ValueType::Number && other.type == ValueType::Number)
		return Value((int)value>>(int)other.value);
	else if (type == ValueType::Object)
	{
		if (this->_object->prototype)
			return this->CallMetamethod("_rs", &other);
	}

	throw RuntimeException("Cannot right-shift two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
};

Value Value::operator~()
{
	if (type == ValueType::Number)
		return Value(~(int)value);
	else if (type == ValueType::Object)
	{
		if (this->_object->prototype)
			return this->CallMetamethod("_bnot", 0);
	}

	throw RuntimeException("Cannot binary complement non-numeric type! " + (std::string)ValueTypes[(int)this->type]);
};

Value Value::operator-()
{
	if (type == ValueType::Number)
	{
		return Value(-value);
	}
	else if (type == ValueType::Object)
	{
		if (this->_object->prototype)
			return this->CallMetamethod("_neg", 0);
	}

	throw RuntimeException("Cannot negate non-numeric type! " + (std::string)ValueTypes[(int)this->type]);
}