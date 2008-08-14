using System;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;
using Rhino.Mocks;
using Rhino.Mocks.Constraints;

namespace Google.ProtocolBuffers {

  /// <summary>
  /// Tests for generated service classes.
  /// TODO(jonskeet): Convert the mocking tests using Rhino.Mocks.
  /// </summary>
  [TestFixture]
  public class ServiceTest {

    delegate void Action<T1, T2>(T1 t1, T2 t2);

    private static readonly MethodDescriptor FooDescriptor = TestService.Descriptor.Methods[0];
    private static readonly MethodDescriptor BarDescriptor = TestService.Descriptor.Methods[1];

    [Test]
    public void GetRequestPrototype() {
      TestService service = new TestServiceImpl();

      Assert.AreSame(service.GetRequestPrototype(FooDescriptor), FooRequest.DefaultInstance);
      Assert.AreSame(service.GetRequestPrototype(BarDescriptor), BarRequest.DefaultInstance);
    }

    [Test]
    public void GetResponsePrototype() {
      TestService service = new TestServiceImpl();

      Assert.AreSame(service.GetResponsePrototype(FooDescriptor), FooResponse.DefaultInstance);
      Assert.AreSame(service.GetResponsePrototype(BarDescriptor), BarResponse.DefaultInstance);
    }

    [Test]
    public void CallMethodFoo() {
      MockRepository mocks = new MockRepository();
      FooRequest fooRequest = FooRequest.CreateBuilder().Build();
      FooResponse fooResponse = FooResponse.CreateBuilder().Build();
      IRpcController controller = mocks.StrictMock<IRpcController>();

      bool fooCalled = false;

      TestService service = new TestServiceImpl((request, responseAction) => {
        Assert.AreSame(fooRequest, request);
        fooCalled = true;
        responseAction(fooResponse);
      }, null, controller);

      bool doneHandlerCalled = false;
      Action<IMessage> doneHandler = (response => {
        Assert.AreSame(fooResponse, response);
        doneHandlerCalled = true;          
      });

      using (mocks.Record()) {
        // No mock interactions to record
      }

      service.CallMethod(FooDescriptor, controller, fooRequest, doneHandler);

      Assert.IsTrue(doneHandlerCalled);
      Assert.IsTrue(fooCalled);
      mocks.VerifyAll();
    }

    delegate void CallFooDelegate(MethodDescriptor descriptor, IRpcController controller,
        IMessage request, IMessage response, Action<IMessage> doneHandler);

    /// <summary>
    /// Tests the generated stub handling of Foo. By this stage we're reasonably confident
    /// that the choice between Foo and Bar is arbitrary, hence the lack of a corresponding Bar
    /// test.
    /// </summary>
    [Test]
    public void GeneratedStubFooCall() {
      FooRequest fooRequest = FooRequest.CreateBuilder().Build();      
      MockRepository mocks = new MockRepository();
      IRpcChannel mockChannel = mocks.StrictMock<IRpcChannel>();
      IRpcController mockController = mocks.StrictMock<IRpcController>();
      TestService service = TestService.CreateStub(mockChannel);
      Action<FooResponse> doneHandler = mocks.StrictMock<Action<FooResponse>>();

      using (mocks.Record()) {
        
        // Nasty way of mocking out "the channel calls the done handler".
        Expect.Call(() => mockChannel.CallMethod(null, null, null, null, null))
            .IgnoreArguments()
            .Constraints(Is.Same(FooDescriptor), Is.Same(mockController), Is.Same(fooRequest), 
                         Is.Same(FooResponse.DefaultInstance), Is.Anything())
            .Do((CallFooDelegate) ((p1, p2, p3, response, done) => done(response)));
        doneHandler.Invoke(FooResponse.DefaultInstance);
      }

      service.Foo(mockController, fooRequest, doneHandler);

      mocks.VerifyAll();
    }

    [Test]
    public void CallMethodBar() {
      MockRepository mocks = new MockRepository();
      BarRequest barRequest = BarRequest.CreateBuilder().Build();
      BarResponse barResponse = BarResponse.CreateBuilder().Build();
      IRpcController controller = mocks.StrictMock<IRpcController>();

      bool barCalled = false;

      TestService service = new TestServiceImpl(null, (request, responseAction) => {
        Assert.AreSame(barRequest, request);
        barCalled = true;
        responseAction(barResponse);
      }, controller);

      bool doneHandlerCalled = false;
      Action<IMessage> doneHandler = (response => {
        Assert.AreSame(barResponse, response);
        doneHandlerCalled = true;
      });

      using (mocks.Record()) {
        // No mock interactions to record
      }

      service.CallMethod(BarDescriptor, controller, barRequest, doneHandler);

      Assert.IsTrue(doneHandlerCalled);
      Assert.IsTrue(barCalled);
      mocks.VerifyAll();
    }
    
    
    class TestServiceImpl : TestService {
      private readonly Action<FooRequest, Action<FooResponse>> fooHandler;
      private readonly Action<BarRequest, Action<BarResponse>> barHandler;
      private readonly IRpcController expectedController;

      internal TestServiceImpl() {
      }

      internal TestServiceImpl(Action<FooRequest, Action<FooResponse>> fooHandler,
          Action<BarRequest, Action<BarResponse>> barHandler,
          IRpcController expectedController) {
        this.fooHandler = fooHandler;
        this.barHandler = barHandler;
        this.expectedController = expectedController;
      }

      public override void Foo(IRpcController controller, FooRequest request, Action<FooResponse> done) {
        Assert.AreSame(expectedController, controller);
        fooHandler(request, done);
      }

      public override void Bar(IRpcController controller, BarRequest request, Action<BarResponse> done) {
        Assert.AreSame(expectedController, controller);
        barHandler(request, done);
      }
    }    
  }
}
