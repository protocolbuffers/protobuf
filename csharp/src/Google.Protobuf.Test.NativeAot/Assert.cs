#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2016 Google Inc.  All rights reserved.
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

namespace Google.Protobuf.Test.NativeAot
{
    /// <summary>
    /// A slim version of the Assert class to avoid the dependency on NUnits framework.
    /// </summary>
    internal static class Assert
    {
        public static void IsNull(object value)
        {
            if (value != null)
            {
                throw new Exception("Expected null");
            }
        }

        public static void IsNotNull(object value)
        {
            if (value == null)
            {
                throw new Exception("Expected not null");
            }
        }

        public static void AreEqual<T>(T expected, T actual)
        {
            if (!Equals(expected, actual))
            {
                throw new Exception($"Expected {expected} but was {actual}");
            }
        }

        public static void AreNotEqual<T>(T expected, T actual)
        {
            if (Equals(expected, actual))
            {
                throw new Exception($"Expected not {expected}");
            }
        }

        public static TActual Throws<TActual>(Action code) where TActual : Exception
        {
            try
            {
                code();
            }
            catch (TActual ex)
            {
                return ex;
            }
            catch (Exception ex)
            {
                throw new Exception($"Expected {typeof(TActual)} but was {ex.GetType()}", ex);
            }

            throw new Exception($"Expected {typeof(TActual)} but no exception was thrown");
        }
    }
}
