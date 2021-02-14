#pragma once
#include <google/protobuf/arena.h>
#include <google/protobuf/io/coded_stream.h>
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
		ptr_ = 0;
	}

	constexpr LazyMessage(nullptr_t n)
	{
		ptr_ = 0;
	}

	void CopyFrom(const LazyMessage& m)
	{
		if (m.ptr_ & 1)
		{
			lazy_message_ = new std::string(*(const std::string*)(m.ptr_ & ~1));
			ptr_ |= 1; 
		}
		else
		{
			message_ = new MessageType(*m.message_);
		}
	}

	~LazyMessage()
	{
		assert(ptr_ == 0);
	}

	LazyMessage& operator = (nullptr_t n)
	{
		ptr_ = 0;
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

	void Delete()
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
		ptr_ = 0;
	}

	MessageType* GetMessage(Arena* arena) const
	{
		if (ptr_ & 1)
		{
			MessageType* new_message = CreateMessage(arena);
			std::string* str = (std::string*)(ptr_ & ~1);
			new_message->ParseFromString(*str);
			const_cast<LazyMessage*>(this)->message_ = new_message;
		}
		return message_;
	}

	MessageType* MutableMessage(Arena* arena)
	{
		MessageType* m = GetMessage(arena);
		if (m == nullptr)
		{
			m = message_ = CreateMessage(arena);
		}
		return m;
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

	int GetCachedSize() const
	{
		if (ptr_ & 1)
		{
			std::string* str = (std::string*)(ptr_ & ~1);
			return str->length();
		}
		else
		{
			return message_->GetCachedSize();
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
			message_->Clear();
		}
	}

	LazyMessage* Release(Arena* arena)
	{
		LazyMessage* m = GetMessage(arena);
		ptr_ = 0;
		return m;
	}

	uint8* _InternalSerialize(
		uint8* target, io::EpsCopyOutputStream* stream) const
	{
		if (ptr_ & 1)
		{
			std::string* str = (std::string*)(ptr_ & ~1);
			return stream->WriteRaw(str->data(), str->length(), target);
		}
		else
		{
			return message_->_InternalSerialize(target, stream);
		}
	}

private:
	static MessageType* CreateMessage(Arena* arena)
	{
		if (arena != nullptr)
		{
			return MessageType::CreateMaybeMessage< MessageType>(arena);
		}
		else
		{
			return new MessageType();
		}
	}
};

}}