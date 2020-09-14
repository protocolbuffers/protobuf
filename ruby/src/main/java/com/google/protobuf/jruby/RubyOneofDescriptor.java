package com.google.protobuf.jruby;

import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyModule;
import org.jruby.RubyObject;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.Block;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;

@JRubyClass(name = "OneofDescriptor", include = "Enumerable")
public class RubyOneofDescriptor extends RubyObject {

    public static void createRubyOneofDescriptor(Ruby runtime) {
        RubyModule protobuf = runtime.getClassFromPath("Google::Protobuf");
        RubyClass cRubyOneofDescriptor = protobuf.defineClassUnder("OneofDescriptor", runtime.getObject(), new ObjectAllocator() {
            @Override
            public IRubyObject allocate(Ruby ruby, RubyClass rubyClass) {
                return new RubyOneofDescriptor(ruby, rubyClass);
            }
        });
        cRubyOneofDescriptor.defineAnnotatedMethods(RubyOneofDescriptor.class);
        cRubyOneofDescriptor.includeModule(runtime.getEnumerable());
    }

    public RubyOneofDescriptor(Ruby ruby, RubyClass rubyClass) {
        super(ruby, rubyClass);
        fields = new ArrayList<RubyFieldDescriptor>();
    }

    /*
     * call-seq:
     *     OneofDescriptor.name => name
     *
     * Returns the name of this oneof.
     */
    @JRubyMethod(name = "name")
    public IRubyObject getName(ThreadContext context) {
        return name;
    }

    /*
     * call-seq:
     *     OneofDescriptor.each(&block) => nil
     *
     * Iterates through fields in this oneof, yielding to the block on each one.
     */
    @JRubyMethod
    public IRubyObject each(ThreadContext context, Block block) {
        for (RubyFieldDescriptor field : fields) {
            block.yieldSpecific(context, field);
        }
        return context.nil;
    }

    protected Collection<RubyFieldDescriptor> getFields() {
        return fields;
    }

    protected OneofDescriptor getDescriptor() {
        return descriptor;
    }

    protected void setDescriptor(ThreadContext context, OneofDescriptor descriptor, Map<FieldDescriptor, RubyFieldDescriptor> fieldCache) {
        this.descriptor = descriptor;
        this.name = context.runtime.newString(descriptor.getName());

        for (FieldDescriptor fd : descriptor.getFields()) {
            fields.add(fieldCache.get(fd));
        }
    }

    private IRubyObject name;
    private List<RubyFieldDescriptor> fields;
    private OneofDescriptor descriptor;
}
