#ifndef _VALUE_HEADER
#define _VALUE_HEADER

#include "JetExceptions.h"
#include "JetInstructions.h"

#include <map>
#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#undef Yield

namespace Jet
{
	class JetContext;
	class GarbageCollector;

	/// <summary>
	/// 值的类型
	/// </summary>
	enum class ValueType:char
	{
		//keep all garbage collectable types towards the end after NativeFunction
		//this is used for the GC being able to tell what it is quickly
		Null = 0,
		Int,
		Real,
		NativeFunction,
		String,
		Object,
		Array,
		Function,
		Userdata,
		Capture,		//you will never see this in the VM
	};
	
	/// <summary>
	/// GC值
	/// </summary>
	template<class t>
	struct GCVal
	{
		JetContext* context = nullptr;
		bool		mark;
		bool		grey;
		ValueType	type : 8;
		unsigned char refcount;//used for native functions
		t			data;

		GCVal() { }

		GCVal(t tt)
		{
			data = tt;
		}
	};

	struct Value;


	static const char* ValueTypes[] = { "Null", "Int", "Real", "NativeFunction", "String" , "Object", "Array", "Function", "Userdata"};

	struct String
	{
		bool mark, grey;
		unsigned char refcount;

		unsigned int length;
		unsigned int hash;
		//string data would be stored after this point
	};

	class JetObject;
	typedef std::vector<Value> _JetArrayBacking;
	struct JetArray
	{
		bool mark, grey;
		ValueType type : 8;
		unsigned char refcount;
		
		JetContext* context;

		_JetArrayBacking data;
	};
	//typedef GCVal<_JetArrayBacking> JetArray;
	typedef _JetArrayBacking::iterator _JetArrayIterator;

	struct JetUserdata
	{
		bool mark, grey;
		ValueType type : 8;
		unsigned char refcount;

		void* data;
		JetObject* prototype;

		JetUserdata(void* d, JetObject* o)
		{
			data = d;
			prototype = o;
		}
	};

	typedef GCVal<char*> JetString;

	typedef void _JetFunction;
	typedef Value(*JetNativeFunc)(JetContext*,Value*, int);

	class JetContext;
	struct Function;
	//each instruction can have a double, or two integers
	struct Instruction
	{
		InstructionType instruction;
		//data part
		union
		{
			struct
			{
				int value;

				union
				{
					int value2;

					Function* func;
					JetString* strlit;
					const char* string;
				};
			};

			int64_t int_lit;	//int literal for pushing numbers
			double	lit;		//double literal for pushing numbers
		};
	};

	struct Function
	{
		~Function()
		{
			for (auto ii: this->instructions)
				if (ii.instruction != InstructionType::CLoad 
					&& ii.instruction != InstructionType::CInit
					&& ii.instruction != InstructionType::LoadFunction 
					&& ii.instruction != InstructionType::LdStr 
					&& ii.instruction != InstructionType::LdInt
					&& ii.instruction != InstructionType::LdReal 
					&& ii.instruction != InstructionType::Call
					&& ii.instruction != InstructionType::Load
					&& ii.instruction != InstructionType::Store
					&& ii.instruction != InstructionType::CStore)
					delete[] ii.string;
		}

		unsigned int args, locals, upvals;
		bool vararg; bool generator;
		JetContext* context;//context where this function was created
		std::vector<Instruction> instructions;//list of all instructions in the function

		//debug info
		std::string name;//the name of the function in code
		
		struct DebugInfo
		{
			unsigned int code;
			std::string file;
			unsigned int line;
		};
		std::vector<DebugInfo> debuginfo;//instruction->line number mappings
		std::vector<std::string> debuglocal;//local variable debug info
		std::vector<std::string> debugcapture;//capture variable debug info
	};

	
	struct Capture;
	struct Generator;
	struct Closure
	{
		bool mark;
		bool grey;
		Jet::ValueType type : 8;
		unsigned char refcount;

		Function* prototype;//details about the function
		Generator* generator;

		unsigned char numupvals;
		Capture** upvals;//captured values
	
		Closure* prev;//parent closure, used for searching for captures
	};

	struct _JetObject;
	struct Value
	{
		ValueType				type;
		union
		{
			double				value;
			int64_t				int_value;
			//this is the main struct
			struct
			{
				union
				{
					JetString*	_string;
					JetObject*	_object;
					JetArray*	_array;
					JetUserdata* _userdata;
					Closure*	_function;	//jet function
				};
				union
				{
					unsigned int length;	//used for strings
				};
			};

			JetNativeFunc		func;		//native func
		};

		Value();

		Value(JetString* str);
		Value(JetObject* obj);
		Value(JetArray* arr);
		
		Value(double val);
		Value(int val);
		Value(int64_t val);

		Value(JetNativeFunc a);
		Value(Closure* func);

		inline void	SetBool(bool v)
		{
			type = ValueType::Int;
			int_value = v ? 1 : 0;
		}

		explicit Value(JetUserdata* userdata, JetObject* prototype);

		Value& operator= (const JetNativeFunc& func)
		{
			return *this = Value(func);
		}

		void SetPrototype(JetObject* obj);

		std::string ToString(int depth = 0) const;

		template<class T>
		inline T*& GetUserdata()
		{
			return (T*&)this->_userdata->data;
		}

		const char* Type() const
		{
			return ValueTypes[(int)this->type];
		}

		operator int()
		{
			if (type == ValueType::Int)		return (int)int_value;
			else if (type == ValueType::Real)	return (int)value;
			else if (type == ValueType::String) return atoi(_string->data);
			throw RuntimeException("Cannot convert type " + (std::string)ValueTypes[(int)this->type] + " to int!");
		}

		operator int64_t()
		{
			if (type == ValueType::Int)		return int_value;
			if (type == ValueType::Real)	return (int64_t)value;
			else if (type == ValueType::String) return _atoi64(_string->data);
			throw RuntimeException("Cannot convert type " + (std::string)ValueTypes[(int)this->type] + " to int!");
		}

		operator double()
		{
			if (type == ValueType::Int)		return (double)int_value;
			if (type == ValueType::Real)	return value;
			else if (type == ValueType::String) return atof(_string->data);

			throw RuntimeException("Cannot convert type " + (std::string)ValueTypes[(int)this->type] + " to real!");
		}
		
		Value operator() (JetContext* context, Value* v = 0, int args = 0);
		Value Call(Value* v = 0, int args = 0);//not recommended, but works for non native functions

		inline bool IsGenerator()
		{
			if (this->type == ValueType::Function)
				return this->_function->prototype->generator;
			return false;
		}

		JetObject* GetPrototype();

		//reference counting stuff
		void AddRef();
		void Release();

		//this massively redundant case is only here because
		//c++ operator overloading resolution is dumb
		//and wants to do integer[pointer-to-object]
		//rather than value[(implicit value)const char*]
		Value& operator[] (int64_t key);
		Value& operator[] (const char* key);
		Value& operator[] (const Value& key);

		//binary operators
		bool operator==(const Value& other) const;

		Value operator+( const Value &other );
		Value operator-( const Value &other );

		Value operator*( const Value &other );
		Value operator/( const Value &other );

		Value operator%( const Value &other );

		Value operator|( const Value &other );
		Value operator&( const Value &other );
		Value operator^( const Value &other );

		Value operator<<( const Value &other );
		Value operator>>( const Value &other );

		void operator+=(const Value &other);
		void operator-=(const Value &other);
		void operator*=(const Value &other);
		void operator/=(const Value &other);
		void operator%=(const Value &other);
		void operator|=(const Value &other);
		void operator&=(const Value &other);
		void operator^=(const Value &other);
		void operator<<=(const Value &other);
		void operator>>=(const Value &other);

		//unary operators
		Value operator~();
		Value operator-();

		// -
		void	Negate();
		// ++
		void	Increase();
		// --
		void	Decrease();

		//空值
		static Value	Empty;
		static Value	Zero;
		static Value	One;
	private:
		Value CallMetamethod(const char* name, const Value* other);
		Value CallMetamethod(JetObject* table, const char* name, const Value* other);

		bool TryCallMetamethod(const char* name, const Value* args, int numargs, Value* out) const;

		friend class JetContext;
	};

// use macro to avoid function call
#define set_value_bool(v,b)	v.type=ValueType::Int;v.int_value=b?1:0;

	struct Capture
	{
		//garbage collector header
		bool mark;
		bool grey;
		Jet::ValueType type : 8;
		unsigned char refcount;

		Value* v;//points to self when closed, or stack when open
		Value value;
		bool closed;

		//debug info
#ifdef _DEBUG
		int usecount;
		Closure* owner;
#endif
	};

	struct ObjNode
	{
		Value first;
		Value second;

		ObjNode* next;
	};

	template <class T>
	class ObjIterator
	{
		typedef ObjNode Node;
		typedef ObjIterator<T> Iterator;
		Node* ptr;
		JetObject* parent;
	public:
		ObjIterator()
		{
			this->parent = 0;
			this->ptr = 0;
		}

		ObjIterator(JetObject* p)
		{
			this->parent = p;
			this->ptr = 0;
		}

		ObjIterator(JetObject* p, Node* node)
		{
			this->parent = p;
			this->ptr = node;
		}

		bool operator==(const Iterator& other)
		{
			return ptr == other.ptr;
		}

		bool operator!=(const Iterator& other)
		{
			return this->ptr != other.ptr;
		}

		Iterator& operator++()
		{
			if (ptr && ((this->ptr-this->parent->nodes) < ((int)this->parent->nodecount-1)))
			{
				do
				{
					this->ptr++;
					if (ptr->first.type != ValueType::Null)
						return *this;
				}
				while ((this->ptr-this->parent->nodes) < ((int)this->parent->nodecount-1));
			}
			this->ptr = 0;

			return *this;
		};

		Node*& operator->() const
		{	// return pointer to class object
			return (Node*&)this->ptr;//this does pointer magic and gives a reference to the pair containing the key and value
		}

		Node& operator*()
		{
			return *this->ptr;
		}
	};

	class JetContext;
	class JetObject
	{
		friend class ObjIterator<Value>;
		friend struct Value;
		friend class GarbageCollector;
		friend class JetContext;

		//gc header
		bool mark, grey;
		Jet::ValueType type;
		unsigned char refcount;

		JetContext* context;
		ObjNode* nodes;
		JetObject* prototype;

		unsigned int Size;
		unsigned int nodecount;
	public:
		typedef ObjIterator<Value> Iterator;

		JetObject(JetContext* context);
		~JetObject();

		std::size_t key(const Value* v) const;

		Iterator find(const Value& key)
		{
			ObjNode* node = this->findNode(&key);
			return Iterator(this, node);
		}

		Iterator find(const char* key)
		{
			ObjNode* node = this->findNode(key);
			return Iterator(this, node);
		}

		//this are faster versions used in the VM
		Value get(const Value& key)
		{
			auto node = this->findNode(&key);
			return node ? node->second : Value();
		}
		Value get(const char* key)
		{
			auto node = this->findNode(key);
			return node ? node->second : Value();
		}

		//just looks for a node
		ObjNode* findNode(const Value* key);
		ObjNode* findNode(const char* key);

		//finds node for key or creates one if doesnt exist
		ObjNode* getNode(const Value* key);
		ObjNode* getNode(const char* key);

		//try not to use these in the vm
		Value& operator [](const Value& key);
		Value& operator [](const char* key);//special operator for strings to deal with insertions
		
		Iterator end()
		{
			return Iterator(this);
		}

		Iterator begin()
		{
			for (unsigned int i = 0; i < this->nodecount; i++)
				if (this->nodes[i].first.type != ValueType::Null)
					return Iterator(this, &nodes[i]);
			return end();
		}

		inline size_t size()
		{
			return this->Size;
		}

		inline void SetPrototype(JetObject* obj)
		{
			this->prototype = obj;
		}

		void DebugPrint();

	private:
		//finds an open node in the table for inserting
		ObjNode* getFreePosition();

		//increases the size to fit new keys
		void resize();

		//memory barrier
		void Barrier();
	};

	//basically a unique_ptr for values
	class ValueRef
	{
		Value v;
	public:
		ValueRef(const Value& value)
		{
			this->v = value;
			this->v.AddRef();
		}

		ValueRef(ValueRef&& other)
		{
			this->v = other.v;
			other.v = Value();
		}

		~ValueRef()
		{
			this->v.Release();
		}

		inline operator Value()
		{
			return this->v;
		}

		inline Value* operator ->()
		{
			return &this->v;
		}

		inline Value& operator [](const char* c)
		{
			return this->v[c];
		}

		inline Value& operator [](int i)
		{
			return this->v[i];
		}

		inline Value& operator [](const Value& c)
		{
			return this->v[c];
		}
	};

	struct Generator
	{
		enum class GeneratorState
		{
			Running,
			Suspended,
			Dead,
		};

		Generator(JetContext* context, Closure* closure, unsigned int args);

		void Yield(JetContext* context, unsigned int iptr);

		unsigned int Resume(JetContext* context);

		void Kill()//what happens when you return
		{
			this->state = GeneratorState::Dead;
			//restore iptr
		}

		int curiptr;
		GeneratorState state;
		Closure* closure;
		Value* stack;
		Value lastyielded;//used for acting like an iterator
	};
}

#endif