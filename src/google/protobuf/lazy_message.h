#pragma once
#include <google/protobuf/arena.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/parse_context.h>
#include <google/protobuf/stubs/logging.h>

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

	void CopyLazyFrom(const LazyMessage& m)
	{
		if (m.IsLazy())
		{
			SetLazyString(new std::string(*m.GetLazyString()));
		}
		else
		{
			GOOGLE_DCHECK(m.message_ != nullptr);
			SetMessage(new MessageType(*m.message_));
		}
	}

	LazyMessage& operator = (nullptr_t n)
	{
		SetNull();
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
		if (IsLazy())
		{
			std::string* str = GetLazyString();
			delete str;
		}
		else
		{
			if (lite)
			{
				delete reinterpret_cast<::google::protobuf::MessageLite*>(message_);
			}
			else
			{
				delete message_;
			}
		}
		SetNull();
	}

	MessageType* GetLazyMessage(Arena* arena) const
	{
		return  const_cast<LazyMessage*>(this)->GetLazyLazyMessage(arena).message_;
	}

	LazyMessage& GetLazyLazyMessage(Arena* arena)
	{
		if (IsLazy())
		{
			MessageType* new_message = CreateMessage(arena);
			std::string* str = GetLazyString();
			new_message->ParseFromString(*str);
			delete str;
			SetNull();
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
			return message_->_InternalParse(ptr, ctx);
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
			return message_->ByteSizeLong();
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
			return message_->GetCachedSize();
		}
	}

	void Clear()
	{
		if (IsLazy())
		{
			std::string* str = GetLazyString();
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
		SetNull();
		return m;
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
			return message_->_InternalSerialize(target, stream);
		}
	}

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
		lazy_message_ = str;
		ptr_ |= 1;
	}

	void SetMessage(MessageType* m)
	{
		GOOGLE_DCHECK(ptr_ == 0);
		message_ = m;
	}

	void SetNull()
	{
		ptr_ = 0;
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