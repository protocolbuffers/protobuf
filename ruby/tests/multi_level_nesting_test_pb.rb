#
# Provide tests for having messages nested 3 levels deep
#

require 'google/protobuf'

Google::Protobuf::DescriptorPool.generated_pool.build do
  add_file("function_call.proto", :syntax => :proto3) do
    add_message "Function" do
      optional :name, :string, 1
      repeated :parameters, :message, 2, "Function.Parameter"
      optional :return_type, :string, 3
    end
    add_message "Function.Parameter" do
      optional :name, :string, 1
      optional :value, :message, 2, "Function.Parameter.Value"
    end
    add_message "Function.Parameter.Value" do
      oneof :type do
        optional :string, :string, 1
        optional :integer, :int64, 2
      end
    end
  end
end
