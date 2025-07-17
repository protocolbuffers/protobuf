package com.google.protobuf;

import com.google.protobuf.Descriptors.DescriptorPool;

/** */
public abstract class ProtoClassLoader extends ClassLoader implements ProtoClassLoaderValue {

  private final DescriptorPool pool = new DescriptorPool(this, true);

  protected ProtoClassLoader() {
    super();
  }

  protected ProtoClassLoader(ClassLoader parent) {
    super(parent);
  }

  protected ProtoClassLoader(String name, ClassLoader parent) {
    super(name, parent);
  }

  @Override
  public final DescriptorPool getDescriptorPool() {
    return pool;
  }
}
