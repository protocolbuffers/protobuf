package com.google.protobuf;

import java.io.IOException;
import java.util.Iterator;
import java.util.Map.Entry;

/**
 * InternalLazyField encapsulates the logic of lazily parsing message fields. It stores the message
 * in a ByteString initially and then parses it on-demand.
 *
 * <p>The invariants of InternalLazyField are that 1) once value or bytes is set, they will not be
 * changed; 2) value and bytes cannot be null at the same time; 3) If corrupted is true, value must
 * be null.
 */
class InternalLazyField {

  // Each lazy field must have a default instance of the message type, which is used to parse the
  // bytes.
  private final MessageLite defaultInstance;

  // The extension registry for this lazy field to parse the bytes.
  private final ExtensionRegistryLite extensionRegistry;

  // The bytes for this lazy field. It can represent the unparsed bytes or the memoized bytes of the
  // serialized message.
  private volatile ByteString bytes;

  // The parsed message value.
  protected volatile MessageLite value;

  // Whether the lazy field (i.e. {@link bytes}) was corrupted, and if so, {@link value} must be
  // `null`.
  private volatile boolean corrupted;

  /** Constructor for InternalLazyField. All arguments cannot be null. */
  InternalLazyField(
      MessageLite defaultInstance, ExtensionRegistryLite extensionRegistry, ByteString bytes) {
    if (defaultInstance == null) {
      throw new IllegalArgumentException("defaultInstance cannot be null");
    }
    if (extensionRegistry == null) {
      throw new IllegalArgumentException("extensionRegistry cannot be null");
    }
    if (bytes == null) {
      throw new IllegalArgumentException("bytes cannot be null");
    }
    this.defaultInstance = defaultInstance;
    this.extensionRegistry = extensionRegistry;
    this.bytes = bytes;
    this.corrupted = false;
    this.value = null;
  }

  /**
   * Constructor for InternalLazyField that takes in a message value. The argument cannot be null.
   */
  InternalLazyField(MessageLite message) {
    if (message == null) {
      throw new IllegalArgumentException("message cannot be null");
    }
    this.value = message;
    this.defaultInstance = message.getDefaultInstanceForType();
    this.extensionRegistry = ExtensionRegistryLite.getEmptyRegistry();
    this.bytes = null;
    this.corrupted = false;
  }

  private boolean containsEmptyBytes() {
    return bytes == null || bytes.isEmpty();
  }

  private void initializeValue() {
    if (value != null) {
      return;
    }

    synchronized (this) {
      if (corrupted) {
        return;
      }
      try {
        if (!containsEmptyBytes()) {
          value = defaultInstance.getParserForType().parseFrom(bytes, extensionRegistry);
        } else {
          value = defaultInstance;
        }
      } catch (InvalidProtocolBufferException e) {
        corrupted = true;
        value = null;
      }
    }
  }

  MessageLite getValue() {
    initializeValue();
    return value == null ? defaultInstance : value;
  }

  int getSerializedSize() {
    if (bytes != null) {
      return bytes.size();
    }
    return value.getSerializedSize();
  }

  ByteString toByteString() {
    if (bytes != null) {
      return bytes;
    }
    synchronized (this) {
      if (bytes != null) {
        return bytes;
      }
      bytes = value.toByteString();
      return bytes;
    }
  }

  int computeSizeNoTag() {
    return CodedOutputStream.computeLengthDelimitedFieldSize(getSerializedSize());
  }

  int computeSize(final int fieldNumber) {
    return CodedOutputStream.computeTagSize(fieldNumber) + computeSizeNoTag();
  }

  int computeMessageSetExtensionSize(final int fieldNumber) {
    return CodedOutputStream.computeTagSize(WireFormat.MESSAGE_SET_ITEM) * 2
        + CodedOutputStream.computeUInt32Size(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber)
        + computeSize(WireFormat.MESSAGE_SET_MESSAGE);
  }

  void writeTo(Writer writer, int fieldNumber) throws IOException {
    if (bytes != null) {
      writer.writeBytes(fieldNumber, bytes);
    } else {
      writer.writeMessage(fieldNumber, value);
    }
  }

  @Override
  public int hashCode() {
    return getValue().hashCode();
  }

  @Override
  public boolean equals(
          Object obj) {
    return getValue().equals(obj);
  }

  @Override
  @SuppressWarnings("LiteProtoToString") // Keep the old behavior of toString().
  public String toString() {
    return getValue().toString();
  }

  /**
   * LazyEntry and LazyIterator are used to encapsulate the lazy field, when users iterate all
   * fields from FieldSet.
   */
  static class LazyEntry<K> implements Entry<K, Object> {
    private final Entry<K, InternalLazyField> entry;

    private LazyEntry(Entry<K, InternalLazyField> entry) {
      this.entry = entry;
    }

    @Override
    public K getKey() {
      return entry.getKey();
    }

    @Override
    public Object getValue() {
      InternalLazyField field = entry.getValue();
      if (field == null) {
        return null;
      }
      return field.getValue();
    }

    public InternalLazyField getField() {
      return entry.getValue();
    }

    @Override
    @SuppressWarnings("PatternMatchingInstanceof")
    public Object setValue(Object value) {
      if (!(value instanceof MessageLite)) {
        throw new IllegalArgumentException("Lazy field only supports MessageLite values.");
      }
      // No need to initialize the value.
      // TODO: b/473034710 - This may be weird as it can return null. Consider returning the parsed
      // value or the lazy field directly as it's only facing internal usage.
      Object oldValue = entry.getValue().value;
      entry.setValue(new InternalLazyField((MessageLite) value));
      return oldValue;
    }
  }

  static class LazyIterator<K> implements Iterator<Entry<K, Object>> {
    private Iterator<Entry<K, Object>> iterator;

    public LazyIterator(Iterator<Entry<K, Object>> iterator) {
      this.iterator = iterator;
    }

    @Override
    public boolean hasNext() {
      return iterator.hasNext();
    }

    @Override
    @SuppressWarnings("unchecked")
    public Entry<K, Object> next() {
      Entry<K, ?> entry = iterator.next();
      if (entry.getValue() instanceof InternalLazyField) {
        return new LazyEntry<K>((Entry<K, InternalLazyField>) entry);
      }
      return (Entry<K, Object>) entry;
    }

    @Override
    public void remove() {
      iterator.remove();
    }
  }
}
