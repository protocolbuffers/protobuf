
using System;
#if CLIENTPROFILE
namespace Microsoft.VisualStudio.TestTools.UnitTesting
{
    [AttributeUsage(AttributeTargets.Class, Inherited = true, AllowMultiple = false)]
    public sealed class TestClassAttribute : NUnit.Framework.TestFixtureAttribute
    {
    }

    [AttributeUsage(AttributeTargets.Method, Inherited = true, AllowMultiple = false)]
    public sealed class TestMethodAttribute : NUnit.Framework.TestAttribute
    {
    }

    [AttributeUsage(AttributeTargets.Method, Inherited = true, AllowMultiple = false)]
    public sealed class TestInitializeAttribute : NUnit.Framework.SetUpAttribute
    {
    }

    [AttributeUsage(AttributeTargets.Method, Inherited = true, AllowMultiple = false)]
    public sealed class IgnoreAttribute : NUnit.Framework.IgnoreAttribute
    {
    }

    [AttributeUsage(AttributeTargets.Method, Inherited = true, AllowMultiple = false)]
    public sealed class ExpectedExceptionAttribute : NUnit.Framework.ExpectedExceptionAttribute
    {
        public ExpectedExceptionAttribute(Type type) : base(type)
        { }
    }

    public class Assert : NUnit.Framework.Assert
    {
        [Obsolete("Do not use AreEqual on Byte[], use TestUtil.AssertBytesEqual(,)")]
        public static void AreEqual(byte[] b1, byte[] b2)
        {
            NUnit.Framework.Assert.AreEqual(b1, b2);
        }

        [Obsolete("No not use assert with miss-matched types.")]
        public static new void AreEqual(object b1, object b2)
        {
            NUnit.Framework.Assert.AreEqual(b1, b2);
        }

        //Allowed if the types match
        public static void AreEqual<T>(T b1, T b2)
        {
            NUnit.Framework.Assert.AreEqual(b1, b2);
        }
    }
}
#endif