#pragma once
#include <google/protobuf/arena.h>
namespace google { namespace protobuf {

template <typename MessageType>
struct LazyMessage
{
	union
	{
		MessageType* message_;
		std::string* lazy_message_;
		uintptr_t ptr_;
	};

	LazyMessage()
	{
		ptr_ = nullptr;
	}

	constexpr LazyMessage(nullptr_t n)
	{
		ptr_ = nullptr;
	}

	~LazyMessage()
	{
		Destroy();
	}

	LazyMessage& operator = (nullptr_t n)
	{
		Destroy();
		return *this;
	}

	LazyMessage& operator = (MessageType* m)
	{
		message_ = m;
		return *this;
	}

	bool operator == (nullptr_t other) const
	{
		return ptr_ == 0;
	}

	bool operator != (nullptr_t other) const
	{
		return ptr_ != 0;
	}

	void Destroy()
	{
		if (ptr_ & 1)
		{
			std::string* str = (std::string*)(ptr_ & ~1);
			delete str;
		}
		else
		{
			delete lazy_message_;
		}
		ptr_ = nullptr;
	}

	MessageType* GetMessage(Arena* arena)
	{
		if (ptr_ & 1)
		{
			MessageType* new_message = nullptr;
			if (arena != nullptr)
			{
				new_message = arena->CreateMaybeMessage< MessageType>();
			}
			else
			{
				new_message = new MessageType();
			}
			std::string* str = (std::string*)(ptr_ & ~1);
			new_message->ParseFromString(*str);
			message_ = new_message;
		}
		return message_;
	}

	void Parse(std::string&& content)
	{
		std::string* str = new std::string();
		*str = std::move(content);
		ptr_ = (uintptr_t)str;
		ptr_ |= 1;
	}

	size_t ByteSizeLong() const
	{
		if (ptr_ & 1)
		{
			std::string* str = (std::string*)(ptr_ & ~1);
			return str->length();
		}
		else
		{
			return message_->ByteSizeLong();
		}
	}

	void Clear()
	{
		if (ptr_ & 1)
		{
			std::string* str = (std::string*)(ptr_ & ~1);
			str->clear();
		}
		else
		{
			lazy_message_->Clear();
		}
	}

	LazyMessage* Release(Arena* arena)
	{
		LazyMessage* m = GetMessage(arena);
		ptr_ = nullptr;
		return m;
	}

};

}}