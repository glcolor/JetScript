
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
		unsigned int _size;
		unsigned int _max;
	public:
		T*	_data=nullptr;

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

		void QuickPop(unsigned int times = 1)
		{
			if (_size < times)
				throw RuntimeException("Tried to pop empty stack!");
			_size -= times;
		}

		T& operator[](unsigned int pos)
		{
			if (pos >= this->_max)
				throw RuntimeException("Bad Stack Index");

			return _data[pos];
		}

		const T&  Peek() const
		{
			return _data[_size-1];
		}

		T Peek()
		{
			return _data[_size - 1];
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
}
#endif