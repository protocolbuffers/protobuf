package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import android.annotation.TargetApi;

/** Caches schemas using ClassValue. This class should only be used if ClassValue is available. */
@ExperimentalApi
@CheckReturnValue
final class ClassValueSchemaCache {
  private final ClassValue<Schema<?>> classValue =
      new ClassValue<Schema<?>>() {
        @Override
        protected Schema<?> computeValue(Class<?> type) {
          return schemaFactory.createSchema(type);
        }
      };

  private final SchemaFactory schemaFactory;

  public ClassValueSchemaCache(SchemaFactory schemaFactory) {
    this.schemaFactory = checkNotNull(schemaFactory);
  }

  @SuppressWarnings("unchecked")
  public <T> Schema<T> get(Class<T> messageType) {
    return (Schema<T>) classValue.get(messageType);
  }
}
