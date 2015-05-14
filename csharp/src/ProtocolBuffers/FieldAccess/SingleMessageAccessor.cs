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
using System;
using System.Reflection;

namespace Google.ProtocolBuffers.FieldAccess
{
    /// <summary>
    /// Accessor for fields representing a non-repeated message value.
    /// </summary>
    internal sealed class SingleMessageAccessor<TMessage, TBuilder> : SinglePrimitiveAccessor<TMessage, TBuilder>
        where TMessage : IMessage<TMessage, TBuilder>
        where TBuilder : IBuilder<TMessage, TBuilder>
    {
        /// <summary>
        /// The static method to create a builder for the property type. For example,
        /// in a message type "Foo", a field called "bar" might be of type "Baz". This
        /// method is Baz.CreateBuilder.
        /// </summary>
        private readonly Func<IBuilder> createBuilderDelegate;

        internal SingleMessageAccessor(string name) : base(null, name, true)
        {
            MethodInfo createBuilderMethod = ClrType.GetMethod("CreateBuilder", ReflectionUtil.EmptyTypes);
            if (createBuilderMethod == null)
            {
                throw new ArgumentException("No public static CreateBuilder method declared in " + ClrType.Name);
            }
            createBuilderDelegate = ReflectionUtil.CreateStaticUpcastDelegate(createBuilderMethod);
        }

        /// <summary>
        /// Creates a message of the appropriate CLR type from the given value,
        /// which may already be of the right type or may be a dynamic message.
        /// </summary>
        private object CoerceType(object value)
        {
            ThrowHelper.ThrowIfNull(value, "value");
            // If it's already of the right type, we're done
            if (ClrType.IsInstanceOfType(value))
            {
                return value;
            }

            // No... so let's create a builder of the right type, and merge the value in.
            IMessageLite message = (IMessageLite) value;
            return CreateBuilder().WeakMergeFrom(message).WeakBuild();
        }

        public override void SetValue(TBuilder builder, object value)
        {
            base.SetValue(builder, CoerceType(value));
        }

        public override IBuilder CreateBuilder()
        {
            return createBuilderDelegate();
        }
    }
}