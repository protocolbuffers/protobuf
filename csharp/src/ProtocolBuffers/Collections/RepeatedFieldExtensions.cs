using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Google.Protobuf.Collections
{
    public static class RepeatedFieldExtensions
    {
        internal static uint CalculateSize<T>(this RepeatedField<T> list, Func<T, int> sizeComputer)
        {
            int size = 0;
            foreach (var item in list)
            {
                size += sizeComputer(item);
            }
            return (uint)size;
        }

        /*
        /// <summary>
        /// Calculates the serialized data size, including one tag per value.
        /// </summary>
        public static int CalculateTotalSize<T>(this RepeatedField<T> list, int tagSize, Func<T, int> sizeComputer)
        {
            if (list.Count == 0)
            {
                return 0;
            }
            return (int)(dataSize + tagSize * list.Count);
        }

        /// <summary>
        /// Calculates the serialized data size, as a packed array (tag, length, data).
        /// </summary>
        public static int CalculateTotalPackedSize(int tagSize)
        {
            if (Count == 0)
            {
                return 0;
            }
            uint dataSize = CalculateSize();
            return tagSize + CodedOutputStream.ComputeRawVarint32Size(dataSize) + (int)dataSize;
        }
        */
    }
}
