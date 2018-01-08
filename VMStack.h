
#ifndef _VMSTACK
#define _VMSTACK

#include "JetExceptions.h"

namespace Jet
{
	const int MaxStackSize = 1024;

	template<class T>
	class VMStack
	{
		const char* overflow_error;
		unsigned int _max;
	public:
		T*	_data=nullptr;
		unsigned int _size;

		VMStack()
		{
			overflow_error = "Stack Overflow";
			_size = 0;
			_max = MaxStackSize;
			_data = new T[_max];
		}

		VMStack(unsigned int size)
		{
			_size = 0;
			_max = size;
			overflow_error = "Stack Overflow";
			_data = new T[_max];
		}

		VMStack(unsigned int size, const char* error)
		{
			_size = 0;
			_max = size;
			overflow_error = error;
			_data = new T[_max];
		}

		VMStack<T> Copy()
		{
			VMStack<T> ns;
			for (unsigned int i = 0; i < this->_size; i++)
				ns._data[i] = this->_data[i];

			ns._size = this->_size;
			return std::move(ns);
		}

		~VMStack()
		{
			if (_data!=nullptr)
			{
				delete[] this->_data;
			}
		}

		T Pop()
		{
			if (_size == 0)
				throw RuntimeException("Tried to pop empty stack!");
			return _data[--_size];
		}

		void Pop(T& v)
		{
			if (_size == 0)
				throw RuntimeException("Tried to pop empty stack!");
			v= _data[--_size];
		}

		inline void QuickPop(unsigned int times = 1)
		{
			_size -= times;
		}

		T& operator[](unsigned int pos)
		{
			if (pos >= this->_max)
				throw RuntimeException("Bad Stack Index");

			return _data[pos];
		}

		T& Peek(int offset=0)
		{
			return _data[_size - 1-offset];
		}

		void Peek(T& v) const
		{
			v= _data[_size - 1];
		}

		void Push(const T& item)
		{
			if (_size >= _max)
				throw RuntimeException(overflow_error);

			_data[_size++] = item;
		}

		unsigned int size() const
		{
			return _size;
		}
	};

	// use macro to avoid function call
#define vmstack_peek(stack) stack._data[stack._size - 1]
#define vmstack_peekn(stack,n) stack._data[stack._size - n]
#define vmstack_pop(stack) --stack._size
#define vmstack_popn(stack,n) stack._size-=n
#define vmstack_push(stack,v) stack._data[stack._size++] = v
#define vmstack_push_top(stack) stack._data[stack._size++] = stack._data[stack._size - 1];
}
#endif