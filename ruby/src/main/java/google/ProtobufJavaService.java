// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package google;

import com.google.protobuf.jruby.*;
import java.io.IOException;
import org.jruby.Ruby;
import org.jruby.runtime.load.BasicLibraryService;

public class ProtobufJavaService implements BasicLibraryService {
  @Override
  public boolean basicLoad(Ruby ruby) throws IOException {
    ruby.defineModule("Google");

    /*
     * The order these happen in is important because we
     * save a static reference to some classes and they
     * need to exist before we try to save a reference to them
     */
    RubyProtobuf.createProtobuf(ruby);
    RubyFileDescriptor.createRubyFileDescriptor(ruby);
    RubyEnumDescriptor.createRubyEnumDescriptor(ruby);
    RubyRepeatedField.createRubyRepeatedField(ruby);
    RubyFieldDescriptor.createRubyFieldDescriptor(ruby);
    RubyMap.createRubyMap(ruby);
    RubyOneofDescriptor.createRubyOneofDescriptor(ruby);
    RubyDescriptor.createRubyDescriptor(ruby);
    RubyMethodDescriptor.createRubyMethodDescriptor(ruby);
    RubyServiceDescriptor.createRubyServiceDescriptor(ruby);
    RubyDescriptorPool.createRubyDescriptorPool(ruby);
    return true;
  }
}
