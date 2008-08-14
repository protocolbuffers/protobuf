using System;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {

  /// <summary>
  /// Tests for generated service classes.
  /// TODO(jonskeet): Convert the mocking tests using Rhino.Mocks.
  /// </summary>
  [TestFixture]
  public class ServiceTest {

    private static readonly MethodDescriptor FooDescriptor = TestService.Descriptor.Methods[0];
    private static readonly MethodDescriptor BarDescriptor = TestService.Descriptor.Methods[1];

    [Test]
    public void GetRequestPrototype() {
      TestService mockService = new TestServiceImpl();

      Assert.AreSame(mockService.GetRequestPrototype(FooDescriptor), FooRequest.DefaultInstance);
      Assert.AreSame(mockService.GetRequestPrototype(BarDescriptor), BarRequest.DefaultInstance);
    }

    [Test]
    public void GetResponsePrototype() {
      TestService mockService = new TestServiceImpl();

      Assert.AreSame(mockService.GetResponsePrototype(FooDescriptor), FooResponse.DefaultInstance);
      Assert.AreSame(mockService.GetResponsePrototype(BarDescriptor), BarResponse.DefaultInstance);
    }
    
    class TestServiceImpl : TestService {
      public override void Foo(IRpcController controller, FooRequest request, Action<FooResponse> done) {
        throw new System.NotImplementedException();
      }

      public override void Bar(IRpcController controller, BarRequest request, Action<BarResponse> done) {
        throw new System.NotImplementedException();
      }
    }
  }
}
