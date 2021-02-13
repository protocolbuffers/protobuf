#pragma once

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

	LazyMessage()
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
	}

	LazyMessage* GetMessage()
	{
		if (ptr_ & 1)
		{
			MessageType* new_message = new MessageType();
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
};
