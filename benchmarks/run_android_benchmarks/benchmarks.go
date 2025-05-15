// Package benchmarks defines the list of benchmarks to run.
//
// At some point we may wish to make this some mix of configuration and command-line flags.
package benchmarks

import (
	"fmt"
)

// A Benchmark is a specific benchmark within a Target.
type Benchmark struct {
	ShortName string
	Name      string
}

// A Target is a specific benchmark target.
type Target struct {
	Name       string   // User-friendly name, e.g., "templates", "FileDesc"
	Label      string   // The specific benchmark target string, e.g., "//path/to:target"
	Platforms  []string // The platforms to run this benchmark on, e.g., "android", "perflab"
	FixedArgs  []string // Arguments that are always present for this benchmark
	Benchmarks []Benchmark
}

var (
	// Targets is a list of benchmark targets to run.
	Targets = []Target{
		{
			Name:      "FileDesc",
			Label:     "//third_party/upb/benchmarks:benchmark",
			Platforms: []string{"android", "perflab"},
			FixedArgs: []string{"--benchmark_filter=Upb_FileDesc"},
			Benchmarks: []Benchmark{
				{ShortName: "Alias", Name: "BM_Parse_Upb_FileDesc<UseArena, Alias>"},
				{ShortName: "Copy", Name: "BM_Parse_Upb_FileDesc<UseArena, Copy>"},
			},
		},
		{
			Name:      "templates",
			Platforms: []string{"android"},
			Label:     "//multiplatform/elements/compiler/experimental/eml2wasm/tests:overflow_button_eko_wasm_diff_test_benchmark",
			FixedArgs: []string{"--benchmark_filter=BM_MakeElement_WAMR/OVERFLOW_BUTTON_FULL_THEME_WITH_CAPS"},
			Benchmarks: []Benchmark{
				{ShortName: "Button", Name: "BM_MakeElement_WAMR/OVERFLOW_BUTTON_FULL_THEME_WITH_CAPS"},
			},
		},
	}
)

// A CPU is a specific CPU configuration.
type CPU struct {
	Name     string // User-friendly label, e.g., "Pixel9a_BigCore"
	Platform string // The platform name, e.g., "android", "perflab"
	Device   string // The device name, e.g., "Pixel_9a".  As seen in `adb devices -l` (model:X)
	Flag     string // Additional command-line flag, if necessary, e.g., "--cpu_affinity=80"
}

var (
	// CPUs is a list of CPU configurations to run benchmarks on.
	CPUs = []CPU{
		{
			Name:     "Pixel9a_BigCore",
			Platform: "android",
			Device:   "Pixel_9a",
			Flag:     "--cpu_affinity=80",
		},
		{
			Name:     "Pixel9a_MiddleCore",
			Platform: "android",
			Device:   "Pixel_9a",
			Flag:     "--cpu_affinity=70",
		},
		{
			Name:     "Pixel9a_LittleCore",
			Platform: "android",
			Device:   "Pixel_9a",
			Flag:     "--cpu_affinity=01",
		},
		{
			Name:     "Mokey_BigCore",
			Platform: "android",
			Device:   "mokey",
			Flag:     "--cpu_affinity=80",
		},
		{
			Name:     "Mokey_LittleCore",
			Platform: "android",
			Device:   "mokey",
			Flag:     "--cpu_affinity=01",
		},
		{
			Name:     "Perflab",
			Platform: "perflab",
			Device:   "Perflab",
			Flag:     "",
		},
	}
)

// A FasttableOption controls whether fasttable is enabled.
type FasttableOption struct {
	Name string
	Flag string
}

var (
	// FasttableOptions is a list of FasttableOptions to run benchmarks with.
	FasttableOptions = []FasttableOption{
		// TODO: Re-enable fasttable once fasttable can build/run on Android.
		{
			Name: "Fasttable",
			Flag: "--extra_blaze_arg=--//third_party/upb:fasttable_enabled=True",
		},
		{
			Name: "NoFasttable",
			Flag: "",
		},
	}
)

// InvocationParams stores the parameters for a single benchmark invocation.
type InvocationParams struct {
	Reference string
	Benchmark Target
	CPU       CPU
	Fasttable FasttableOption
}

var (
	commonArgs = []string{
		"--compilation_mode=opt",
		"--extra_blaze_arg=--dynamic_mode=off",
		"--copt=-gmlt",
		"--benchtime=0.3s",
		"--perf_counters=CPU-CYCLES,INSTRUCTIONS,CACHE-MISSES,BRANCH-MISSES",
	}

	platformArgs = map[string][]string{
		"android": []string{
			"--adb",
			"--config=release",
			"--extra_blaze_arg=--config=android_arm64-v8a",
			"--extra_blaze_arg=--embed_changelist=none",
			"--extra_blaze_arg=--features=-fully_strip",
			"--extra_blaze_arg=--android_ndk_min_sdk_version=26",
			"--copt=-fno-omit-frame-pointer",
			"--copt=-DGOOGLE_COMMANDLINEFLAGS_FULL_API=1",
		},

		"perflab": []string{
			"--perflab",
		},
	}
)

// GetArgs returns the arguments for a single benchmark invocation.
func GetArgs(params InvocationParams) []string {
	args := []string{fmt.Sprintf("--reference=%s", params.Reference)}

	args = append(args, commonArgs...)
	args = append(args, platformArgs[params.CPU.Platform]...)

	if params.CPU.Flag != "" {
		args = append(args, params.CPU.Flag)
	}

	if params.Fasttable.Flag != "" {
		args = append(args, params.Fasttable.Flag)
	}

	args = append(args, params.Benchmark.FixedArgs...)
	args = append(args, params.Benchmark.Label)

	return args
}

// GetOutputFileName returns the name of the output filename for a single benchmark invocation.
func GetOutputFileName(p InvocationParams) string {
	return fmt.Sprintf("%s-%s-%s.txt", p.Benchmark.Name, p.CPU.Name, p.Fasttable.Name)
}
