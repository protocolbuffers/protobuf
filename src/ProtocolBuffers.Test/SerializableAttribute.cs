#if SILVERLIGHT

namespace System
{
    [AttributeUsage(AttributeTargets.Class)]
    public class SerializableAttribute : Attribute
    {
        public SerializableAttribute () : base() { }
    }
}

#endif
