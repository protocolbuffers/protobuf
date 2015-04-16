#if NOSERIALIZABLE && !COMPACT_FRAMEWORK

namespace System
{
    [AttributeUsage(AttributeTargets.Class)]
    public class SerializableAttribute : Attribute
    {
        public SerializableAttribute () : base() { }
    }
}

#endif
