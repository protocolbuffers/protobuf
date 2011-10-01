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
using Google.ProtocolBuffers.Serialization.Http;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;
using System.IO;
using Google.ProtocolBuffers.Serialization;
using System.Text;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// This class verifies the correct code is generated from unittest_rpc_interop.proto and provides a small demonstration
    /// of using the new IRpcDispatch to write a client/server
    /// </summary>
    [TestFixture]
    public class TestRpcForMimeTypes
    {
        /// <summary>
        /// A sample implementation of the ISearchService for testing
        /// </summary>
        private class ExampleSearchImpl : ISearchService
        {
            SearchResponse ISearchService.Search(SearchRequest searchRequest)
            {
                if (searchRequest.CriteriaCount == 0)
                {
                    throw new ArgumentException("No criteria specified.", new InvalidOperationException());
                }
                SearchResponse.Builder resp = SearchResponse.CreateBuilder();
                foreach (string criteria in searchRequest.CriteriaList)
                {
                    resp.AddResults(
                        SearchResponse.Types.ResultItem.CreateBuilder().SetName(criteria).SetUrl("http://search.com").
                            Build());
                }
                return resp.Build();
            }

            SearchResponse ISearchService.RefineSearch(RefineSearchRequest refineSearchRequest)
            {
                SearchResponse.Builder resp = refineSearchRequest.PreviousResults.ToBuilder();
                foreach (string criteria in refineSearchRequest.CriteriaList)
                {
                    resp.AddResults(
                        SearchResponse.Types.ResultItem.CreateBuilder().SetName(criteria).SetUrl("http://refine.com").
                            Build());
                }
                return resp.Build();
            }
        }

        /// <summary>
        /// An example extraction of the wire protocol
        /// </summary>
        private interface IHttpTransfer
        {
            void Execute(string method, string contentType, Stream input, string acceptType, Stream output);
        }

        /// <summary>
        /// An example of a server responding to a web/http request
        /// </summary>
        private class ExampleHttpServer : IHttpTransfer
        {
            public readonly MessageFormatOptions Options =
                new MessageFormatOptions
                {
                    ExtensionRegistry = ExtensionRegistry.Empty,
                    FormattedOutput = true,
                    XmlReaderOptions = XmlReaderOptions.ReadNestedArrays,
                    XmlReaderRootElementName = "request",
                    XmlWriterOptions = XmlWriterOptions.OutputNestedArrays,
                    XmlWriterRootElementName = "response"
                };

            private readonly IRpcServerStub _stub;

            public ExampleHttpServer(ISearchService implementation)
            {
                //on the server, we create a dispatch to call the appropriate method by name
                IRpcDispatch dispatch = new SearchService.Dispatch(implementation);
                //we then wrap that dispatch in a server stub which will deserialize the wire bytes to the message
                //type appropriate for the method name being invoked.
                _stub = new SearchService.ServerStub(dispatch);
            }

            void IHttpTransfer.Execute(string method, string contentType, Stream input, string acceptType, Stream output)
            {
                //Extension for: Google.ProtocolBuffers.Serialization.Http.ServiceExtensions.HttpCallMethod(_stub,
                _stub.HttpCallMethod(
                    method, Options,
                    contentType, input,
                    acceptType, output
                    );
            }
        }

        /// <summary>
        /// An example of a client sending a wire request
        /// </summary>
        private class ExampleClient : IRpcDispatch
        {
            public readonly MessageFormatOptions Options =
                new MessageFormatOptions
                {
                    ExtensionRegistry = ExtensionRegistry.Empty,
                    FormattedOutput = true,
                    XmlReaderOptions = XmlReaderOptions.ReadNestedArrays,
                    XmlReaderRootElementName = "response",
                    XmlWriterOptions = XmlWriterOptions.OutputNestedArrays,
                    XmlWriterRootElementName = "request"
                };


            private readonly IHttpTransfer _wire;
            private readonly string _mimeType;

            public ExampleClient(IHttpTransfer wire, string mimeType)
            {
                _wire = wire;
                _mimeType = mimeType;
            }

            TMessage IRpcDispatch.CallMethod<TMessage, TBuilder>(string method, IMessageLite request,
                                                                 IBuilderLite<TMessage, TBuilder> response)
            {
                MemoryStream input = new MemoryStream();
                MemoryStream output = new MemoryStream();

                //Write to _mimeType format
                request.WriteTo(Options, _mimeType, input);

                input.Position = 0;
                _wire.Execute(method, _mimeType, input, _mimeType, output);

                //Read from _mimeType format
                output.Position = 0;
                response.MergeFrom(Options, _mimeType, output);
                
                return response.Build();
            }
        }

        /// <summary>
        /// Test sending and recieving messages via text/json
        /// </summary>
        [Test]
        public void TestClientServerWithJsonFormat()
        {
            ExampleHttpServer server = new ExampleHttpServer(new ExampleSearchImpl());
            //obviously if this was a 'real' transport we would not use the server, rather the server would be listening, the client transmitting
            IHttpTransfer wire = server;

            ISearchService client = new SearchService(new ExampleClient(wire, "text/json"));
            //now the client has a real, typed, interface to work with:
            SearchResponse result = client.Search(SearchRequest.CreateBuilder().AddCriteria("Test").Build());
            Assert.AreEqual(1, result.ResultsCount);
            Assert.AreEqual("Test", result.ResultsList[0].Name);
            Assert.AreEqual("http://search.com", result.ResultsList[0].Url);

            //The test part of this, call the only other method
            result =
                client.RefineSearch(
                    RefineSearchRequest.CreateBuilder().SetPreviousResults(result).AddCriteria("Refine").Build());
            Assert.AreEqual(2, result.ResultsCount);
            Assert.AreEqual("Test", result.ResultsList[0].Name);
            Assert.AreEqual("http://search.com", result.ResultsList[0].Url);

            Assert.AreEqual("Refine", result.ResultsList[1].Name);
            Assert.AreEqual("http://refine.com", result.ResultsList[1].Url);
        }

        /// <summary>
        /// Test sending and recieving messages via text/json
        /// </summary>
        [Test]
        public void TestClientServerWithXmlFormat()
        {
            ExampleHttpServer server = new ExampleHttpServer(new ExampleSearchImpl());
            //obviously if this was a 'real' transport we would not use the server, rather the server would be listening, the client transmitting
            IHttpTransfer wire = server;

            ISearchService client = new SearchService(new ExampleClient(wire, "text/xml"));
            //now the client has a real, typed, interface to work with:
            SearchResponse result = client.Search(SearchRequest.CreateBuilder().AddCriteria("Test").Build());
            Assert.AreEqual(1, result.ResultsCount);
            Assert.AreEqual("Test", result.ResultsList[0].Name);
            Assert.AreEqual("http://search.com", result.ResultsList[0].Url);

            //The test part of this, call the only other method
            result =
                client.RefineSearch(
                    RefineSearchRequest.CreateBuilder().SetPreviousResults(result).AddCriteria("Refine").Build());
            Assert.AreEqual(2, result.ResultsCount);
            Assert.AreEqual("Test", result.ResultsList[0].Name);
            Assert.AreEqual("http://search.com", result.ResultsList[0].Url);

            Assert.AreEqual("Refine", result.ResultsList[1].Name);
            Assert.AreEqual("http://refine.com", result.ResultsList[1].Url);
        }

        /// <summary>
        /// Test sending and recieving messages via text/json
        /// </summary>
        [Test]
        public void TestClientServerWithProtoFormat()
        {
            ExampleHttpServer server = new ExampleHttpServer(new ExampleSearchImpl());
            //obviously if this was a 'real' transport we would not use the server, rather the server would be listening, the client transmitting
            IHttpTransfer wire = server;

            ISearchService client = new SearchService(new ExampleClient(wire, "application/x-protobuf"));
            //now the client has a real, typed, interface to work with:
            SearchResponse result = client.Search(SearchRequest.CreateBuilder().AddCriteria("Test").Build());
            Assert.AreEqual(1, result.ResultsCount);
            Assert.AreEqual("Test", result.ResultsList[0].Name);
            Assert.AreEqual("http://search.com", result.ResultsList[0].Url);

            //The test part of this, call the only other method
            result =
                client.RefineSearch(
                    RefineSearchRequest.CreateBuilder().SetPreviousResults(result).AddCriteria("Refine").Build());
            Assert.AreEqual(2, result.ResultsCount);
            Assert.AreEqual("Test", result.ResultsList[0].Name);
            Assert.AreEqual("http://search.com", result.ResultsList[0].Url);

            Assert.AreEqual("Refine", result.ResultsList[1].Name);
            Assert.AreEqual("http://refine.com", result.ResultsList[1].Url);
        }

        /// <summary>
        /// Test sending and recieving messages via text/json
        /// </summary>
        [Test]
        public void TestClientServerWithCustomFormat()
        {
            ExampleHttpServer server = new ExampleHttpServer(new ExampleSearchImpl());
            //Setup our custom mime-type format as the only format supported:
            server.Options.MimeInputTypes.Clear();
            server.Options.MimeInputTypes.Add("foo/bar", CodedInputStream.CreateInstance);
            server.Options.MimeOutputTypes.Clear();
            server.Options.MimeOutputTypes.Add("foo/bar", CodedOutputStream.CreateInstance);

            //obviously if this was a 'real' transport we would not use the server, rather the server would be listening, the client transmitting
            IHttpTransfer wire = server;

            ExampleClient exclient = new ExampleClient(wire, "foo/bar");
            //Add our custom mime-type format
            exclient.Options.MimeInputTypes.Add("foo/bar", CodedInputStream.CreateInstance);
            exclient.Options.MimeOutputTypes.Add("foo/bar", CodedOutputStream.CreateInstance);
            ISearchService client = new SearchService(exclient);

            //now the client has a real, typed, interface to work with:
            SearchResponse result = client.Search(SearchRequest.CreateBuilder().AddCriteria("Test").Build());
            Assert.AreEqual(1, result.ResultsCount);
            Assert.AreEqual("Test", result.ResultsList[0].Name);
            Assert.AreEqual("http://search.com", result.ResultsList[0].Url);

            //The test part of this, call the only other method
            result =
                client.RefineSearch(
                    RefineSearchRequest.CreateBuilder().SetPreviousResults(result).AddCriteria("Refine").Build());
            Assert.AreEqual(2, result.ResultsCount);
            Assert.AreEqual("Test", result.ResultsList[0].Name);
            Assert.AreEqual("http://search.com", result.ResultsList[0].Url);

            Assert.AreEqual("Refine", result.ResultsList[1].Name);
            Assert.AreEqual("http://refine.com", result.ResultsList[1].Url);
        }

        /// <summary>
        /// Test sending and recieving messages via text/json
        /// </summary>
        [Test]
        public void TestServerWithUriFormat()
        {
            ExampleHttpServer server = new ExampleHttpServer(new ExampleSearchImpl());
            //obviously if this was a 'real' transport we would not use the server, rather the server would be listening, the client transmitting
            IHttpTransfer wire = server;

            MemoryStream input = new MemoryStream(Encoding.UTF8.GetBytes("?Criteria=Test&Criteria=Test+of%20URI"));
            MemoryStream output = new MemoryStream();

            //Call the server
            wire.Execute("Search",
                         MessageFormatOptions.ContentFormUrlEncoded, input,
                         MessageFormatOptions.ContentTypeProtoBuffer, output
                         );

            SearchResponse result = SearchResponse.ParseFrom(output.ToArray());
            Assert.AreEqual(2, result.ResultsCount);
            Assert.AreEqual("Test", result.ResultsList[0].Name);
            Assert.AreEqual("http://search.com", result.ResultsList[0].Url);

            Assert.AreEqual("Test of URI", result.ResultsList[1].Name);
            Assert.AreEqual("http://search.com", result.ResultsList[1].Url);
        }

        /// <summary>
        /// Test sending and recieving messages via text/json
        /// </summary>
        [Test, ExpectedException(typeof(ArgumentOutOfRangeException))]
        public void TestInvalidMimeType()
        {
            ExampleHttpServer server = new ExampleHttpServer(new ExampleSearchImpl());
            //obviously if this was a 'real' transport we would not use the server, rather the server would be listening, the client transmitting
            IHttpTransfer wire = server;

            MemoryStream input = new MemoryStream();
            MemoryStream output = new MemoryStream();

            //Call the server
            wire.Execute("Search",
                         "bad/mime", input,
                         MessageFormatOptions.ContentTypeProtoBuffer, output
                         );
            Assert.Fail();
        }

        /// <summary>
        /// Test sending and recieving messages via text/json
        /// </summary>
        [Test]
        public void TestDefaultMimeType()
        {
            ExampleHttpServer server = new ExampleHttpServer(new ExampleSearchImpl());
            
            //obviously if this was a 'real' transport we would not use the server, rather the server would be listening, the client transmitting
            IHttpTransfer wire = server;


            MemoryStream input = new MemoryStream(new SearchRequest.Builder().AddCriteria("Test").Build().ToByteArray());
            MemoryStream output = new MemoryStream();

            //With this default set, any invalid/unknown mime-type will be mapped to use that format
            server.Options.DefaultContentType = MessageFormatOptions.ContentTypeProtoBuffer;

            wire.Execute("Search",
                         "foo", input,
                         "bar", output
                         );

            SearchResponse result = SearchResponse.ParseFrom(output.ToArray());
            Assert.AreEqual(1, result.ResultsCount);
            Assert.AreEqual("Test", result.ResultsList[0].Name);
            Assert.AreEqual("http://search.com", result.ResultsList[0].Url);
        }
    }
}