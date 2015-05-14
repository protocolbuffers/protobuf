// ProtoBench.java's cpp version
// Copyright enjolras1205@gmail.com
// Feel free to use, reuse and abuse the code in this file.

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <google_speed.pb.h>
#include <google_size.pb.h>

using namespace google::protobuf::io;
using namespace std;

#define RETURN_IF_FAILED(x) do { int ret = (x); if (ret != 0) return ret; } while(0)
#define GO_ERROR_IF_FAILED(x) do { int ret = (x); if (ret != 0) goto error; } while(0)

const long MIN_SAMPLE_TIME_US = 2 * 1000000;
const long TARGET_TIME_US = 30 * 1000000;

typedef function<int(void)> Func;

// calc usec gap of start to now
int time_action(int iterations, const Func &func, long &gap) {
	struct timeval start;
	struct timeval end;
	gettimeofday(&start, 0);
	for (int i = 0; i < iterations; ++i) {
		if (!func()) {
			cerr << "serialize/deserialize error" << endl;
			return -1;
		}
	}
	gettimeofday(&end, 0);
	gap = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;
	return 0;
}

int benchmark(const string &test_type, size_t data_size, const Func &func) {
	// Run it progressively more times until we've got a reasonable sample
	int iterations = 1;
	long elapsed = 0;
	RETURN_IF_FAILED(time_action(iterations, func, elapsed));
	while (elapsed < MIN_SAMPLE_TIME_US) {
		iterations *= 2;
		RETURN_IF_FAILED(time_action(iterations, func, elapsed));
	}
	// Upscale the sample to the target time. Do this in floating point arithmetic
	// to avoid overflow issues.
	iterations = (int) ((TARGET_TIME_US / (double) elapsed) * iterations);
	RETURN_IF_FAILED(time_action(iterations, func, elapsed));

	cout<< test_type << iterations << " iterations in "
			<< elapsed / 1000000.0 << "s; "
			<< (static_cast<double>(iterations) * data_size) / (elapsed / 1000000.0 * 1024 * 1024)
			<< "MB/s" <<endl;

	return 0;
}

template<typename MSG>
int run_test(const string &file_path, const string &msg_type) {
	ifstream file_stream(file_path);

	if (!file_stream) {
		cerr << "can't read from file " << file_path << endl;
		return -1;
	}

	int fd = open("/dev/null", O_WRONLY);
	if (fd < 0) {
		cerr << "can't open dev null for write" << endl;
		return -1;
	}

	file_stream.seekg(0, std::ios::end);
	size_t data_size = file_stream.tellg();

	// str
	string str;
	str.reserve(data_size);
	file_stream.seekg(0, std::ios::beg);
	str.assign((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());

	// byte array
	char *arr = new char[data_size];

	// memory out stream
	ostringstream o_str_stream;
	OstreamOutputStream o_mem_stream(&o_str_stream);

	// dev null out stream
	FileOutputStream null_stream(fd);
	// reuse dev null out stream
	CodedOutputStream reuse_null_stream(&null_stream);

	// memory in stream
	istringstream i_str_stream;
	IstreamInputStream i_mem_stream(&i_str_stream);

	MSG msg;

	GO_ERROR_IF_FAILED(!msg.ParseFromString(str));

	GO_ERROR_IF_FAILED(benchmark(msg_type + "Serialize to byte string ", data_size,
		[&msg, &str]() -> int { return msg.SerializeToString(&str); }));
	GO_ERROR_IF_FAILED(benchmark(msg_type + "Serialize to byte array ", data_size,
		[&msg, arr, data_size]() -> int { return msg.SerializeToArray(arr, data_size); }));
	GO_ERROR_IF_FAILED(benchmark(msg_type + "Serialize to memory stream ", data_size,
		[&msg, &o_mem_stream, &o_str_stream]() -> int
		{ o_str_stream.seekp(std::ios::beg); return msg.SerializeToZeroCopyStream(&o_mem_stream); }
	));
	GO_ERROR_IF_FAILED(benchmark(msg_type + "Serialize to /dev/null with FileOutputStream ", data_size,
		[&msg, &null_stream]() -> int { return msg.SerializeToZeroCopyStream(&null_stream); }));
	GO_ERROR_IF_FAILED(benchmark(msg_type + "Serialize to /dev/null reusing FileOutputStream ", data_size,
		[&msg, &reuse_null_stream]() -> int { return msg.SerializeToCodedStream(&reuse_null_stream); }));

	GO_ERROR_IF_FAILED(benchmark(msg_type + "Deserialize from byte string ", data_size,
		[&msg, &str]() -> int { return msg.ParseFromString(str); }));
	GO_ERROR_IF_FAILED(benchmark(msg_type + "Deserialize from byte array ", data_size,
		[&msg, arr, data_size]() -> int { return msg.ParseFromArray(arr, data_size); }));
	GO_ERROR_IF_FAILED(benchmark(msg_type + "Deserialize from memory stream ", data_size,
		[&msg, &i_mem_stream, &i_str_stream, &str]() -> int {
			i_str_stream.str(str);
			i_str_stream.clear();
			return msg.ParseFromZeroCopyStream(&i_mem_stream);
		}));

	close(fd);
	delete []arr;
	return 0;

error:
	cerr << "error" << endl;
	close(fd);
	delete []arr;
	return -1;
}

int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <message_dat_file1> <message_dat_file2>" << endl;
        return -1;
    }

    int ret = 0;

    ret |= run_test<benchmarks::SpeedMessage1>(argv[1], "speed1 ");
    ret |= run_test<benchmarks::SizeMessage1>(argv[1], "size1 ");

    ret |= run_test<benchmarks::SpeedMessage2>(argv[2], "speed2 ");
    ret |= run_test<benchmarks::SizeMessage2>(argv[2], "size2 ");

    return ret;
}
