#include <google/protobuf/lazy_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

namespace google { namespace protobuf {

Message*& LazyMessageBase::GetLazyMessage(const Message& m, const FieldDescriptor& descriptor)
{
	if (IsLazy())
	{
		const Message* default_message = m.GetReflection()->GetDefaultMessageInstance(&descriptor);
		Message* new_message = default_message->New(m.GetArena());
		std::string* str = GetLazyString();
		new_message->ParseFromString(*str);
		delete str;
		SetNull();
		ptr_ = (uintptr_t)new_message;
	}
	return (Message*&)ptr_;
}

}}