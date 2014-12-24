package com.google.protobuf.jruby;

import com.google.protobuf.DescriptorProtos;
import com.google.protobuf.Descriptors;
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

import java.util.*;

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
    }

    @JRubyMethod
    public IRubyObject initialize(ThreadContext context) {
        builder = DescriptorProtos.OneofDescriptorProto.newBuilder();
        fields = new ArrayList<RubyFieldDescriptor>();
        return this;
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
     *     OneofDescriptor.name = name
     *
     * Sets a new name for this oneof. The oneof must not have been added to a
     * message descriptor yet.
     */
    @JRubyMethod(name = "name=")
    public IRubyObject setName(ThreadContext context, IRubyObject name) {
        this.name = context.runtime.newString(name.asJavaString());
        this.builder.setName(name.asJavaString());
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     OneofDescriptor.add_field(field) => nil
     *
     * Adds a field to this oneof. The field may have been added to this oneof in
     * the past, or the message to which this oneof belongs (if any), but may not
     * have already been added to any other oneof or message. Otherwise, an
     * exception is raised.
     *
     * All fields added to the oneof via this method will be automatically added to
     * the message to which this oneof belongs, if it belongs to one currently, or
     * else will be added to any message to which the oneof is later added at the
     * time that it is added.
     */
    @JRubyMethod(name = "add_field")
    public IRubyObject addField(ThreadContext context, IRubyObject obj) {
        RubyFieldDescriptor fieldDescriptor = (RubyFieldDescriptor) obj;
        fieldDescriptor.setOneofName(this.name);
        fields.add(fieldDescriptor);
        return context.runtime.getNil();
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
        return context.runtime.getNil();
    }

    public DescriptorProtos.OneofDescriptorProto build(int index) {
        for (RubyFieldDescriptor field: fields) {
            field.setOneofIndex(index);
        }
        return this.builder.build();
    }

    protected Collection<RubyFieldDescriptor> getFields() {
        return fields;
    }

    protected Descriptors.OneofDescriptor getOneofDescriptor() {
        RubyFieldDescriptor fieldDescriptor = fields.get(0);
        return fieldDescriptor.getFieldDef().getContainingOneof();
    }

    private IRubyObject name;
    private DescriptorProtos.OneofDescriptorProto.Builder builder;
    private List<RubyFieldDescriptor> fields;
}
