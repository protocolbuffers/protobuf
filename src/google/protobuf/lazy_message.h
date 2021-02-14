#pragma once
#include <google/protobuf/arena.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/parse_context.h>

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

	template <typename bool lite>
	void Delete()
	{
		if (ptr_ & 1)
		{
			std::string* str = (std::string*)(ptr_ & ~1);
			delete str;
		}
		else
		{
			if (lite)
			{
				delete reinterpret_cast<::google::protobuf::MessageLite*>(lazy_message_);
			}
			else
			{
				delete lazy_message_;
			}
		}
		ptr_ = 0;
	}

	MessageType* GetLazyMessage(Arena* arena) const
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

	const char* _InternalParse(const char* ptr, internal::ParseContext* ctx)
	{
		return ptr;
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