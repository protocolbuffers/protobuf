TESTS=(message_test message_factory_test descriptor_test proto_builder_test)
for test in ${TESTS[@]}; do
	python -m unittest -v google.protobuf.internal.${test}
done
