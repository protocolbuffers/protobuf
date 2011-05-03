#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#endregion

using System;
using Google.ProtocolBuffers;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// This class verifies the correct code is generated from unittest_rpc_interop.proto and provides a small demonstration
    /// of using the new IRpcDispatch to write a client/server
    /// </summary>
    [TestFixture]
    public class TestRpcGenerator
    {
      /// <summary>
      /// A sample implementation of the ISearchService for testing
      /// </summary>
      class ExampleSearchImpl : ISearchService {
        SearchResponse ISearchService.Search(SearchRequest searchRequest) {
            if (searchRequest.CriteriaCount == 0) {
              throw new ArgumentException("No criteria specified.", new InvalidOperationException());
            }
            SearchResponse.Builder resp = SearchResponse.CreateBuilder();
            foreach (string criteria in searchRequest.CriteriaList) {
              resp.AddResults(SearchResponse.Types.ResultItem.CreateBuilder().SetName(criteria).SetUrl("http://search.com").Build());
            }
            return resp.Build();
        }

        SearchResponse ISearchService.RefineSearch(RefineSearchRequest refineSearchRequest) {
            SearchResponse.Builder resp = refineSearchRequest.PreviousResults.ToBuilder();
            foreach (string criteria in refineSearchRequest.CriteriaList) {
              resp.AddResults(SearchResponse.Types.ResultItem.CreateBuilder().SetName(criteria).SetUrl("http://refine.com").Build());
            }
            return resp.Build();
        }
      }

      /// <summary>
      /// An example extraction of the wire protocol
      /// </summary>
      interface IWireTransfer 
      {
          byte[] Execute(string method, byte[] message);
      }

      /// <summary>
      /// An example of a server responding to a wire request
      /// </summary>
      class ExampleServerHost : IWireTransfer
      {
          readonly IRpcServerStub _stub;
          public ExampleServerHost(ISearchService implementation)
          {
              //on the server, we create a dispatch to call the appropriate method by name
              IRpcDispatch dispatch = new SearchService.Dispatch(implementation);
              //we then wrap that dispatch in a server stub which will deserialize the wire bytes to the message
              //type appropriate for the method name being invoked.
              _stub = new SearchService.ServerStub(dispatch);
          }

          byte[] IWireTransfer.Execute(string method, byte[] message)
          {
              //now when we recieve a wire transmission to invoke a method by name with a byte[] or stream payload
              //we just simply call the sub:
              IMessageLite response = _stub.CallMethod(method, CodedInputStream.CreateInstance(message), ExtensionRegistry.Empty);
              //now we return the expected response message:
              return response.ToByteArray();
          }
      }

      /// <summary>
      /// An example of a client sending a wire request
      /// </summary>
      class ExampleClient : IRpcDispatch
      {
          readonly IWireTransfer _wire;
          public ExampleClient(IWireTransfer wire)
          {
              _wire = wire;
          }

          TMessage IRpcDispatch.CallMethod<TMessage, TBuilder>(string method, IMessageLite request, IBuilderLite<TMessage, TBuilder> response)
          {
              byte[] rawResponse = _wire.Execute(method, request.ToByteArray());
              response.MergeFrom(rawResponse);
              return response.Build();
          }
      }

        /// <summary>
        /// Put it all together to create one seamless client/server experience full of rich-type goodness ;)
        /// All you need to do is send/recieve the method name and message bytes across the wire.
        /// </summary>
        [Test]
        public void TestClientServerDispatch()
        {
            ExampleServerHost server = new ExampleServerHost(new ExampleSearchImpl());
            //obviously if this was a 'real' transport we would not use the server, rather the server would be listening, the client transmitting
            IWireTransfer wire = server;

            ISearchService client = new SearchService(new ExampleClient(wire));
            //now the client has a real, typed, interface to work with:
            SearchResponse result = client.Search(SearchRequest.CreateBuilder().AddCriteria("Test").Build());
            Assert.AreEqual(1, result.ResultsCount);
            Assert.AreEqual("Test", result.ResultsList[0].Name);
            Assert.AreEqual("http://search.com", result.ResultsList[0].Url);

            //The test part of this, call the only other method
            result = client.RefineSearch(RefineSearchRequest.CreateBuilder().SetPreviousResults(result).AddCriteria("Refine").Build());
            Assert.AreEqual(2, result.ResultsCount);
            Assert.AreEqual("Test", result.ResultsList[0].Name);
            Assert.AreEqual("http://search.com", result.ResultsList[0].Url);

            Assert.AreEqual("Refine", result.ResultsList[1].Name);
            Assert.AreEqual("http://refine.com", result.ResultsList[1].Url);
        }
    }
}