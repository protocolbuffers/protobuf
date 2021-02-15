#pragma once
#include <google/protobuf/arena.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/parse_context.h>
#include <google/protobuf/stubs/logging.h>

namespace google { namespace protobuf {

class FieldDescriptor;
class Message;
struct LazyMessageBase
{
	uintptr_t ptr_;
	LazyMessageBase():ptr_(0){}
	bool IsLazy() const
	{
		return ptr_ & 1;
	}

	bool IsNull() const
	{
		return ptr_ == 0;
	}

	std::string* GetLazyString() const
	{
		return (std::string*)(ptr_ & ~1);
	}

	void SetLazyString(std::string* str)
	{
		GOOGLE_DCHECK(ptr_ == 0);
		ptr_ = (uintptr_t)str;
		ptr_ |= 1;
	}

	void SetNull()
	{
		ptr_ = 0;
	}

	Message** GetLazyMessage(const Message& m, const FieldDescriptor& descriptor);
};

template <typename MessageType>
struct LazyMessage : LazyMessageBase
{
	LazyMessage()
	{
	}

	constexpr LazyMessage(nullptr_t n)
	{
	}

	void CopyLazyFrom(const LazyMessage& m)
	{
		if (m.IsLazy())
		{
			SetLazyString(new std::string(*m.GetLazyString()));
		}
		else
		{
			GOOGLE_DCHECK(!m.IsNull());
			SetMessage(new MessageType(*m.GetMessage()));
		}
	}

	LazyMessage& operator = (nullptr_t n)
	{
		SetNull();
		return *this;
	}

	LazyMessage& operator = (MessageType* m)
	{
		SetMessage(m);
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

	template <bool lite>
	void Delete()
	{
		if (IsLazy())
		{
			std::string* str = GetLazyString();
			delete str;
		}
		else
		{
			if (lite)
			{
				delete reinterpret_cast<::google::protobuf::MessageLite*>(GetMessage());
			}
			else
			{
				delete GetMessage();
			}
		}
		SetNull();
	}

	MessageType* GetLazyMessage(Arena* arena) const
	{
		return  const_cast<LazyMessage*>(this)->GetLazyLazyMessage(arena).GetMessage();
	}

	LazyMessage& GetLazyLazyMessage(Arena* arena)
	{
		if (IsLazy())
		{
			MessageType* new_message = CreateMessage(arena);
			std::string* str = GetLazyString();
			new_message->ParsePartialFromString(*str);
			delete str;
			SetMessage(new_message);
		}
		return *this;
	}

	const char* _InternalParse(const char* ptr, internal::ParseContext* ctx)
	{
		if (IsNull())
		{
			std::string* str = new std::string();
			ptr = ctx->ReadString(ptr, ctx->size(), str);
			SetLazyString(str);
			return ptr;
		}
		else
		{
			GOOGLE_CHECK(!IsLazy());
			return GetMessage()->_InternalParse(ptr, ctx);
		}
	}

	size_t ByteSizeLong() const
	{
		if (IsLazy())
		{
			std::string* str = GetLazyString();
			return str->length();
		}
		else
		{
			return GetMessage()->ByteSizeLong();
		}
	}

	int GetCachedSize() const
	{
		if (IsLazy())
		{
			std::string* str = GetLazyString();
			return str->length();
		}
		else
		{
			return GetMessage()->GetCachedSize();
		}
	}

	void Clear()
	{
		if (IsLazy())
		{
			std::string* str = GetLazyString();
			delete str;
			SetNull();
		}
		else
		{
			GetMessage()->Clear();
		}
	}

	uint8* _InternalSerialize(
		uint8* target, io::EpsCopyOutputStream* stream) const
	{
		if (IsLazy())
		{
			std::string* str = GetLazyString();
			return stream->WriteRaw(str->data(), str->length(), target);
		}
		else
		{
			return GetMessage()->_InternalSerialize(target, stream);
		}
	}

	void SetMessage(MessageType* m)
	{
		ptr_ = (uintptr_t)m;
	}

	MessageType* GetMessage() const
	{
		return (MessageType*)ptr_;
	}

private:
	static MessageType* CreateMessage(Arena* arena)
	{
		if (arena != nullptr)
		{
			return MessageType::template CreateMaybeMessage<MessageType>(arena);
		}
		else
		{
			return new MessageType();
		}
	}
};

}}

