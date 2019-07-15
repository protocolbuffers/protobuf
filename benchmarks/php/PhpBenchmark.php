<?php

namespace Google\Protobuf\Benchmark;
ini_set('memory_limit', '4096M');

const NAME = "PhpBenchmark.php";

function _require_all($dir, &$prefix) {
    // require all php files
    foreach (glob("$dir/*") as $path) {
        if (preg_match('/\.php$/', $path) &&
            substr($path, -strlen(NAME)) != NAME) {
                require_once(substr($path, strlen($prefix) + 1));
            } elseif (is_dir($path)) {
                _require_all($path, $prefix);
            }
    }
}
// include all file
foreach (explode(PATH_SEPARATOR, get_include_path()) as $one_include_path) {
    _require_all($one_include_path, $one_include_path);
}

use Benchmarks\BenchmarkDataset;

class BenchmarkMethod
{
    // $args[0]: dataset
    // $args[1]: message class
    static function parse(&$args) {
        $payloads = $args[0]->getPayload();
        for ($i = $payloads->count() - 1; $i >= 0; $i--) {
            (new $args[1]())->mergeFromString($payloads->offsetGet($i));
        }
    }

    // $args: array of message
    static function serialize(&$args) {
        foreach ($args as &$temp_message) {
            $temp_message->serializeToString();
        }
    }
}

class Benchmark
{
    private $benchmark_name;
    private $args;
    private $benchmark_time;
    private $total_bytes;
    private $coefficient;

    public function __construct($benchmark_name, $args, $total_bytes,
        $benchmark_time = 5.0) {
            $this->args = $args;
            $this->benchmark_name = $benchmark_name;
            $this->benchmark_time = $benchmark_time;
            $this->total_bytes = $total_bytes;
            $this->coefficient = pow (10, 0) / pow(2, 20);
    }

    public function runBenchmark() {
        $t = $this->runBenchmarkWithTimes(1);
        $times = ceil($this->benchmark_time / $t);
        return $this->total_bytes * $times /
        ($times == 1 ? $t : $this->runBenchmarkWithTimes($times)) *
        $this->coefficient;
    }

    private function runBenchmarkWithTimes($times) {
        $st = microtime(true);
        for ($i = 0; $i < $times; $i++) {
            call_user_func_array($this->benchmark_name, array(&$this->args));
        }
        $en = microtime(true);
        return $en - $st;
    }
}

function getMessageName(&$dataset) {
    switch ($dataset->getMessageName()) {
        case "benchmarks.proto3.GoogleMessage1":
            return "\Benchmarks\Proto3\GoogleMessage1";
        case "benchmarks.proto2.GoogleMessage1":
            return "\Benchmarks\Proto2\GoogleMessage1";
        case "benchmarks.proto2.GoogleMessage2":
            return "\Benchmarks\Proto2\GoogleMessage2";
        case "benchmarks.google_message3.GoogleMessage3":
            return "\Benchmarks\Google_message3\GoogleMessage3";
        case "benchmarks.google_message4.GoogleMessage4":
            return "\Benchmarks\Google_message4\GoogleMessage4";
        default:
            exit("Message " . $dataset->getMessageName() . " not found !");
    }
}

function runBenchmark($file, $behavior_prefix) {
    $datafile = fopen($file, "r") or die("Unable to open file " . $file);
    $bytes = fread($datafile, filesize($file));
    $dataset = new BenchmarkDataset(NULL);
    $dataset->mergeFromString($bytes);
    $message_name = getMessageName($dataset);
    $message_list = array();
    $total_bytes = 0;
    $payloads = $dataset->getPayload();
    for ($i = $payloads->count() - 1; $i >= 0; $i--) {
        $new_message = new $message_name();
        $new_message->mergeFromString($payloads->offsetGet($i));
        array_push($message_list, $new_message);
        $total_bytes += strlen($payloads->offsetGet($i));
    }

    $parse_benchmark = new Benchmark(
        "\Google\Protobuf\Benchmark\BenchmarkMethod::parse",
        array($dataset, $message_name), $total_bytes);
    $serialize_benchmark = new Benchmark(
        "\Google\Protobuf\Benchmark\BenchmarkMethod::serialize",
        $message_list, $total_bytes);

    return array(
        "filename" => $file,
        "benchmarks" => array(
            $behavior_prefix . "_parse" => $parse_benchmark->runBenchmark(),
            $behavior_prefix . "_serailize" => $serialize_benchmark->runBenchmark()
        ),
        "message_name" => $dataset->getMessageName()
    );
}

// main
$json_output = false;
$results = array();
$behavior_prefix = "";

foreach ($argv as $index => $arg) {
    if ($index == 0) {
        continue;
    }
    if ($arg == "--json") {
        $json_output = true;
    } else if (strpos($arg, "--behavior_prefix") == 0) {
        $behavior_prefix = str_replace("--behavior_prefix=", "", $arg);
    }
}

foreach ($argv as $index => $arg) {
    if ($index == 0) {
        continue;
    }
    if (substr($arg, 0, 2) == "--") {
        continue;
    } else {
        array_push($results, runBenchmark($arg, $behavior_prefix));
    }
}

if ($json_output) {
    print json_encode($results);
} else {
    print "PHP protobuf benchmark result:\n\n";
    foreach ($results as $result) {
        printf("result for test data file: %s\n", $result["filename"]);
        foreach ($result["benchmarks"] as $benchmark => $throughput) {
            printf("   Throughput for benchmark %s: %.2f MB/s\n",
                $benchmark, $throughput);
        }
    }
}

?>
