#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.Collections;
using System;

namespace Google.Protobuf
{
    internal interface IExtensionValue : IEquatable<IExtensionValue>, IDeepCloneable<IExtensionValue>
    {
        void MergeFrom(ref ParseContext ctx);

        void MergeFrom(IExtensionValue value);
        void WriteTo(ref WriteContext ctx);
        int CalculateSize();
        bool IsInitialized();
        object GetValue();
    }

    internal sealed class ExtensionValue<T> : IExtensionValue
    {
        private T field;
        private readonly FieldCodec<T> codec;

        internal ExtensionValue(FieldCodec<T> codec)
        {
            this.codec = codec;
            field = codec.DefaultValue;
        }

        public int CalculateSize()
        {
            return codec.CalculateUnconditionalSizeWithTag(field);
        }

        public IExtensionValue Clone()
        {
            return new ExtensionValue<T>(codec)
            {
                field = field is IDeepCloneable<T> ? (field as IDeepCloneable<T>).Clone() : field
            };
        }

        public bool Equals(IExtensionValue other)
        {
            if (ReferenceEquals(this, other))
                return true;

            return other is ExtensionValue<T>
                && codec.Equals((other as ExtensionValue<T>).codec)
                && Equals(field, (other as ExtensionValue<T>).field);
            // we check for equality in the codec since we could have equal field values however the values could be written in different ways
        }

        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = hash * 31 + field.GetHashCode();
                hash = hash * 31 + codec.GetHashCode();
                return hash;
            }
        }

        public void MergeFrom(ref ParseContext ctx)
        {
            codec.ValueMerger(ref ctx, ref field);
        }

        public void MergeFrom(IExtensionValue value)
        {
            if (value is ExtensionValue<T>)
            {
                var extensionValue = value as ExtensionValue<T>;
                codec.FieldMerger(ref field, extensionValue.field);
            }
        }

        public void WriteTo(ref WriteContext ctx)
        {
            ctx.WriteTag(codec.Tag);
            codec.ValueWriter(ref ctx, field);
            if (codec.EndTag != 0)
            {
                ctx.WriteTag(codec.EndTag);
            }
        }

        public T GetValue() => field;

        object IExtensionValue.GetValue() => field;

        public void SetValue(T value)
        {
            field = value;
        }

        public bool IsInitialized()
        {
            if (field is IMessage)
            {
                return (field as IMessage).IsInitialized();
            }
            else
            {
                return true;
            }
        }
    }

    internal sealed class RepeatedExtensionValue<T> : IExtensionValue
    {
        private RepeatedField<T> field;
        private readonly FieldCodec<T> codec;

        internal RepeatedExtensionValue(FieldCodec<T> codec)
        {
            this.codec = codec;
            field = new RepeatedField<T>();
        }

        public int CalculateSize()
        {
            return field.CalculateSize(codec);
        }

        public IExtensionValue Clone()
        {
            return new RepeatedExtensionValue<T>(codec)
            {
                field = field.Clone()
            };
        }

        public bool Equals(IExtensionValue other)
        {
            if (ReferenceEquals(this, other))
                return true;

            return other is RepeatedExtensionValue<T> 
                && field.Equals((other as RepeatedExtensionValue<T>).field) 
                && codec.Equals((other as RepeatedExtensionValue<T>).codec);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = hash * 31 + field.GetHashCode();
                hash = hash * 31 + codec.GetHashCode();
                return hash;
            }
        }

        public void MergeFrom(ref ParseContext ctx)
        {
            field.AddEntriesFrom(ref ctx, codec);
        }

        public void MergeFrom(IExtensionValue value)
        {
            if (value is RepeatedExtensionValue<T>)
            {
                field.Add((value as RepeatedExtensionValue<T>).field);
            }
        }

        public void WriteTo(ref WriteContext ctx)
        {
            field.WriteTo(ref ctx, codec);
        }

        public RepeatedField<T> GetValue() => field;

        object IExtensionValue.GetValue() => field;

        public bool IsInitialized()
        {
            for (int i = 0; i < field.Count; i++)
            {
                var element = field[i];
                if (element is IMessage)
                {
                    if (!(element as IMessage).IsInitialized())
                    {
                        return false;
                    }
                }
                else
                {
                    break;
                }
            }

            return true;
        }
    }
}
