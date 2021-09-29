#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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
using Google.Protobuf;
using Google.Protobuf.Reflection;
using System.Buffers;
using pb = global::Google.Protobuf;
using pbr = global::Google.Protobuf.Reflection;
using NUnit.Framework;
using System.IO;
using System;
using Google.Protobuf.Buffers;

namespace Google.Protobuf
{
    public class LegacyGeneratedCodeTest
    {
        [Test]
        public void IntermixingOfNewAndLegacyGeneratedCodeWorksWithCodedInputStream()
        {
            var message = new ParseContextEnabledMessageB
            {
                A = new LegacyGeneratedCodeMessageA
                {
                    Bb = new ParseContextEnabledMessageB { OptionalInt32 = 12345 }
                },
                OptionalInt32 = 6789
            };
            var data = message.ToByteArray();

            // when parsing started using CodedInputStream and a message with legacy generated code
            // is encountered somewhere in the parse tree, we still need to be able to use its
            // MergeFrom(CodedInputStream) method to parse correctly.
            var codedInput = new CodedInputStream(data);
            var parsed = new ParseContextEnabledMessageB();
            codedInput.ReadRawMessage(parsed);
            Assert.IsTrue(codedInput.IsAtEnd);

            Assert.AreEqual(12345, parsed.A.Bb.OptionalInt32);
            Assert.AreEqual(6789, parsed.OptionalInt32);
        }

        [Test]
        public void LegacyGeneratedCodeThrowsWithReadOnlySequence()
        {
            var message = new ParseContextEnabledMessageB
            {
                A = new LegacyGeneratedCodeMessageA
                {
                    Bb = new ParseContextEnabledMessageB { OptionalInt32 = 12345 }
                },
                OptionalInt32 = 6789
            };
            var data = message.ToByteArray();

            // if parsing started using ReadOnlySequence and we don't have a CodedInputStream
            // instance at hand, we cannot fall back to the legacy MergeFrom(CodedInputStream)
            // method and parsing will fail. As a consequence, one can only use parsing
            // from ReadOnlySequence if all the messages in the parsing tree have their generated
            // code up to date.
            var exception = Assert.Throws<InvalidProtocolBufferException>(() =>
            {
                ParseContext.Initialize(new ReadOnlySequence<byte>(data), out ParseContext parseCtx);
                var parsed = new ParseContextEnabledMessageB();
                ParsingPrimitivesMessages.ReadRawMessage(ref parseCtx, parsed);
            });
            Assert.AreEqual($"Message {typeof(LegacyGeneratedCodeMessageA).Name} doesn't provide the generated method that enables ParseContext-based parsing. You might need to regenerate the generated protobuf code.", exception.Message);
        }

        [Test]
        public void IntermixingOfNewAndLegacyGeneratedCodeWorksWithCodedOutputStream()
        {
            // when serialization started using CodedOutputStream and a message with legacy generated code
            // is encountered somewhere in the parse tree, we still need to be able to use its
            // WriteTo(CodedOutputStream) method to serialize correctly.
            var ms = new MemoryStream();
            var codedOutput = new CodedOutputStream(ms);
            var message = new ParseContextEnabledMessageB
            {
                A = new LegacyGeneratedCodeMessageA
                {
                    Bb = new ParseContextEnabledMessageB { OptionalInt32 = 12345 }
                },
                OptionalInt32 = 6789
            };
            message.WriteTo(codedOutput);
            codedOutput.Flush();

            var codedInput = new CodedInputStream(ms.ToArray());
            var parsed = new ParseContextEnabledMessageB();
            codedInput.ReadRawMessage(parsed);
            Assert.IsTrue(codedInput.IsAtEnd);

            Assert.AreEqual(12345, parsed.A.Bb.OptionalInt32);
            Assert.AreEqual(6789, parsed.OptionalInt32);
        }

        [Test]
        public void LegacyGeneratedCodeThrowsWithIBufferWriter()
        {
            // if serialization started using IBufferWriter and we don't have a CodedOutputStream
            // instance at hand, we cannot fall back to the legacy WriteTo(CodedOutputStream)
            // method and serializatin will fail. As a consequence, one can only use serialization
            // to IBufferWriter if all the messages in the parsing tree have their generated
            // code up to date.
            var message = new ParseContextEnabledMessageB
            {
                A = new LegacyGeneratedCodeMessageA
                {
                    Bb = new ParseContextEnabledMessageB { OptionalInt32 = 12345 }
                },
                OptionalInt32 = 6789
            };
            var exception = Assert.Throws<InvalidProtocolBufferException>(() =>
            {
                WriteContext.Initialize(new TestArrayBufferWriter<byte>(), out WriteContext writeCtx);
                ((IBufferMessage)message).InternalWriteTo(ref writeCtx);
            });
            Assert.AreEqual($"Message {typeof(LegacyGeneratedCodeMessageA).Name} doesn't provide the generated method that enables WriteContext-based serialization. You might need to regenerate the generated protobuf code.", exception.Message);
        }

        // hand-modified version of a generated message that only provides the legacy
        // MergeFrom(CodedInputStream) method and doesn't implement IBufferMessage.
        private sealed partial class LegacyGeneratedCodeMessageA : pb::IMessage {
          private pb::UnknownFieldSet _unknownFields;

          pbr::MessageDescriptor pb::IMessage.Descriptor => throw new System.NotImplementedException();

          /// <summary>Field number for the "bb" field.</summary>
          public const int BbFieldNumber = 1;
          private ParseContextEnabledMessageB bb_;
          public ParseContextEnabledMessageB Bb {
            get { return bb_; }
            set {
              bb_ = value;
            }
          }

          public void WriteTo(pb::CodedOutputStream output) {
            if (bb_ != null) {
              output.WriteRawTag(10);
              output.WriteMessage(Bb);
            }
            if (_unknownFields != null) {
              _unknownFields.WriteTo(output);
            }
          }

          public int CalculateSize() {
            int size = 0;
            if (bb_ != null) {
              size += 1 + pb::CodedOutputStream.ComputeMessageSize(Bb);
            }
            if (_unknownFields != null) {
              size += _unknownFields.CalculateSize();
            }
            return size;
          }

          public void MergeFrom(pb::CodedInputStream input) {
            uint tag;
            while ((tag = input.ReadTag()) != 0) {
              switch(tag) {
                default:
                  _unknownFields = pb::UnknownFieldSet.MergeFieldFrom(_unknownFields, input);
                  break;
                case 10: {
                  if (bb_ == null) {
                    Bb = new ParseContextEnabledMessageB();
                  }
                  input.ReadMessage(Bb);
                  break;
                }
              }
            }
          }
        }

        // hand-modified version of a generated message that does provide
        // the new InternalMergeFrom(ref ParseContext) method.
        private sealed partial class ParseContextEnabledMessageB : pb::IBufferMessage {
          private pb::UnknownFieldSet _unknownFields;

          pbr::MessageDescriptor pb::IMessage.Descriptor => throw new System.NotImplementedException();

          /// <summary>Field number for the "a" field.</summary>
          public const int AFieldNumber = 1;
          private LegacyGeneratedCodeMessageA a_;
          public LegacyGeneratedCodeMessageA A {
            get { return a_; }
            set {
              a_ = value;
            }
          }

          /// <summary>Field number for the "optional_int32" field.</summary>
          public const int OptionalInt32FieldNumber = 2;
          private int optionalInt32_;
          public int OptionalInt32 {
            get { return optionalInt32_; }
            set {
              optionalInt32_ = value;
            }
          }

          public void WriteTo(pb::CodedOutputStream output) {
            output.WriteRawMessage(this);
          }

          void pb::IBufferMessage.InternalWriteTo(ref pb::WriteContext output)
          {
            if (a_ != null)
            {
              output.WriteRawTag(10);
              output.WriteMessage(A);
            }
            if (OptionalInt32 != 0)
            {
              output.WriteRawTag(16);
              output.WriteInt32(OptionalInt32);
            }
            if (_unknownFields != null)
            {
              _unknownFields.WriteTo(ref output);
            }
          }

          public int CalculateSize() {
            int size = 0;
            if (a_ != null) {
              size += 1 + pb::CodedOutputStream.ComputeMessageSize(A);
            }
            if (OptionalInt32 != 0) {
              size += 1 + pb::CodedOutputStream.ComputeInt32Size(OptionalInt32);
            }
            if (_unknownFields != null) {
              size += _unknownFields.CalculateSize();
            }
            return size;
          }
          public void MergeFrom(pb::CodedInputStream input) {
            input.ReadRawMessage(this);
          }

          void pb::IBufferMessage.InternalMergeFrom(ref pb::ParseContext input) {
            uint tag;
            while ((tag = input.ReadTag()) != 0) {
              switch(tag) {
                default:
                  _unknownFields = pb::UnknownFieldSet.MergeFieldFrom(_unknownFields, ref input);
                  break;
                case 10: {
                  if (a_ == null) {
                    A = new LegacyGeneratedCodeMessageA();
                  }
                  input.ReadMessage(A);
                  break;
                }
                case 16: {
                  OptionalInt32 = input.ReadInt32();
                  break;
                }
              }
            }
          }
        }
    }
}