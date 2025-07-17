package com.google.protobuf;

import com.google.protobuf.Descriptors.DescriptorPool;

interface ProtoClassLoaderValue {

  DescriptorPool getDescriptorPool();
}
