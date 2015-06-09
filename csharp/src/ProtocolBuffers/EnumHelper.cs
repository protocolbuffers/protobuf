using System;
using System.Collections.Generic;
using System.Linq;
using System.Linq.Expressions;
using System.Text;

namespace Google.Protobuf
{
    public class EnumHelper<T> where T : struct, IComparable, IFormattable
    {
        // TODO(jonskeet): For snmall enums, use something lighter-weight than a dictionary?
        private static readonly Dictionary<int, T> _byNumber;
        private static readonly Dictionary<string, T> _byName;
        private static readonly Func<T, long> toRawValue;
        private static readonly Func<long, T> fromRawValue;

        private const long UnknownValueBase = 0x100000000L;

        static EnumHelper()
        {
            if (!typeof(T).IsEnum)
            {
                throw new InvalidOperationException(string.Format("{0} is not an enum type", typeof(T).FullName));
            }
            if (Enum.GetUnderlyingType(typeof(T)) != typeof(long))
            {
                throw new InvalidOperationException(string.Format("{0} does not have long as an underlying type", typeof(T).FullName));
            }
            // It will actually be a T[], but the CLR will let us convert.
            long[] array = (long[])Enum.GetValues(typeof(T));
            _byNumber = new Dictionary<int, T>(array.Length);
            foreach (long i in array)
            {
                _byNumber[(int) i] = (T)(object)i;
            }
            string[] names = (string[])Enum.GetNames(typeof(T));
            _byName = new Dictionary<string, T>();
            foreach (var name in names)
            {
                _byName[name] = (T) Enum.Parse(typeof(T), name, false);
            }

            ParameterExpression param1 = Expression.Parameter(typeof(T), "x");
            ParameterExpression param2 = Expression.Parameter(typeof(long), "x");
            Expression convertedParam1 = Expression.Convert(param1, typeof(long));
            Expression convertedParam2 = Expression.Convert(param2, typeof(T));
            toRawValue = Expression.Lambda<Func<T, long>>(convertedParam1, param1).Compile();
            fromRawValue = Expression.Lambda<Func<long, T>>(convertedParam2, param2).Compile();
        }

        /// <summary>
        /// Converts the given 32-bit integer into a value of the enum type.
        /// If the integer is a known, named value, that is returned; otherwise,
        /// a value which is outside the range of 32-bit integers but which preserves
        /// the original integer is returned.
        /// </summary>
        public static T FromInt32(int value)
        {
            T validValue;
            return _byNumber.TryGetValue(value, out validValue) ? validValue : FromRawValue((long) value | UnknownValueBase);
        }

        /// <summary>
        /// Converts a value of the enum type to a 32-bit integer. This reverses
        /// <see cref="Int32"/>.
        /// </summary>
        public static int ToInt32(T value)
        {
            return (int)GetRawValue(value);
        }

        public static bool IsKnownValue(T value)
        {
            long raw = GetRawValue(value);
            if (raw >= int.MinValue && raw <= int.MaxValue)
            {
                return _byNumber.ContainsKey((int)raw);
            }
            return false;
        }

        private static long GetRawValue(T value)
        {
            return toRawValue(value);
        }

        private static T FromRawValue(long value)
        {
            return fromRawValue(value);
        }

    }
}
