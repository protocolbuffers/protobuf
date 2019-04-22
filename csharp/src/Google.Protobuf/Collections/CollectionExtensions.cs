namespace Google.Protobuf.Collections
{

    /// <summary>
    /// Extension Methods for Collections
    /// </summary>
    public static class CollectionExtensions
    {

        /// <summary>
        /// Deep Merge for RepeatedField
        /// </summary>
        public static void MergeFrom<T>(this RepeatedField<T> to, RepeatedField<T> from, MessageParser<T> parser) where T : IMessage<T>
        {
            foreach (var item in from)
            {
                var newItem = parser.CreateTemplate();
                newItem.MergeFrom(item);
                to.Add(newItem);
            }
        }


        /// <summary>
        /// Deep Merge for MapField
        /// </summary>
        public static void MergeFrom<TKey,TValue>(this MapField<TKey, TValue> to, MapField<TKey, TValue> from, MessageParser<TValue> parser) where TValue : IMessage<TValue>
        {
            foreach (var item in from)
            {
                var newItem = parser.CreateTemplate();
                newItem.MergeFrom(item.Value);
                to[item.Key] = newItem;
            }
        }
    }
}