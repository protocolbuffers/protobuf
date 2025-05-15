package main

import (
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"slices"
	"time"

	"google3/base/go/flag"
	"google3/base/go/google"
	"google3/third_party/upb/benchmarks/run_android_benchmarks/benchmarks"
	"google3/third_party/upb/benchmarks/run_android_benchmarks/environment"
	"google3/third_party/upb/benchmarks/run_android_benchmarks/output"
	"google3/third_party/upb/benchmarks/run_android_benchmarks/parsebenchy"
)

var (
	benchy    = "/google/data/ro/users/ja/jacobsa/benchy"
	reference = flag.String("reference", "srcfs", "The reference to use for the benchmark.")
)

type RunResult struct {
	Device string
	Output string
	Err    error
}

func runBenchmark(invocation output.Invocation, ch chan RunResult) {
	// To allow multiple Blaze invocations to run in parallel, and to maximize utilization of the
	// Blaze cache, we use an output directory that is unique to each device.
	args := []string{"--blaze_output_base_dir=" + invocation.BlazeOutputBaseDir}
	args = append(args, benchmarks.GetArgs(invocation.Params)...)

	cmd := exec.Command(benchy, args...)

	cmd.Env = os.Environ()
	if invocation.AndroidSerial != "" {
		cmd.Env = append(cmd.Env, "ANDROID_SERIAL="+invocation.AndroidSerial)
	}

	outputBytes, execErr := cmd.CombinedOutput() // Capture both stdout and stderr
	output := string(outputBytes)

	if writeErr := ioutil.WriteFile(invocation.OutputFileName, outputBytes, 0644); writeErr != nil {
		ch <- RunResult{
			Device: invocation.Params.CPU.Device,
			Output: "",
			Err:    fmt.Errorf("could not write command output to %s: %v", invocation.OutputFileName, writeErr),
		}
		return
	}

	if execErr != nil {
		ch <- RunResult{
			Device: invocation.Params.CPU.Device,
			Output: "",
			Err:    fmt.Errorf("benchmark failed: %v, see output in %s", execErr, invocation.OutputFileName),
		}
		return
	}

	ch <- RunResult{
		Device: invocation.Params.CPU.Device,
		Output: output,
		Err:    nil,
	}
}

func percentDiff(new, old float64) string {
	ret := fmt.Sprintf("%.2f%%", (new-old)/old*100)
	if ret[0] != '-' {
		ret = "+" + ret
	}
	return ret
}

func runBenchmarks(env *environment.Environment) error {
	if err := os.MkdirAll(env.OutputDir, 0755); err != nil {
		return fmt.Errorf("Failed to create output directory %s: %v", env.OutputDir, err)
	}

	fmt.Printf("Outputs will be saved in: %s\n\n", env.OutputDir)

	var groups []*output.InvocationGroup
	execQueues := make(map[string][]*output.Invocation)

	for _, fasttable := range benchmarks.FasttableOptions {
		for _, target := range benchmarks.Targets {
			group := output.InvocationGroup{
				Target: &target,
			}
			for _, cpu := range benchmarks.CPUs {
				serial := ""
				if cpu.Platform == "android" {
					foundSerial, ok := env.AndroidDevices[cpu.Device]
					if !ok {
						fmt.Printf("WARNING: Skipping benchmarks for %s because no matching device found\n", cpu.Name)
						continue
					}
					serial = foundSerial
				}

				if !slices.Contains(target.Platforms, cpu.Platform) {
					continue
				}

				params := benchmarks.InvocationParams{
					Reference: *reference,
					Benchmark: target,
					CPU:       cpu,
					Fasttable: fasttable,
				}
				invocation := output.Invocation{
					Params:             params,
					Reference:          *reference,
					AndroidSerial:      serial,
					BlazeOutputBaseDir: filepath.Join(env.BlazeOutputBaseDir, cpu.Device),
					OutputFileName:     filepath.Join(env.OutputDir, benchmarks.GetOutputFileName(params)),
				}
				for _, benchmark := range target.Benchmarks {
					line := output.ResultLine{
						Invocation: &invocation,
						Benchmark:  &benchmark,
					}
					output.SetBenchmarkHeader(&line)
					invocation.ResultLines = append(invocation.ResultLines, &line)
				}
				group.Invocations = append(group.Invocations, &invocation)
				execQueues[cpu.Device] = append(execQueues[cpu.Device], &invocation)
			}
			groups = append(groups, &group)
		}
	}

	ch := make(chan RunResult)
	for _, queue := range execQueues {
		invocation := queue[0]
		now := time.Now()
		invocation.StartTime = &now
		go runBenchmark(*invocation, ch)
	}

	scrollBack := 0

	for len(execQueues) > 0 {
		select {
		case result := <-ch:
			if result.Err != nil {
				return result.Err
			}

			queue := execQueues[result.Device]
			invocation := queue[0]
			now := time.Now()
			invocation.EndTime = &now
			metrics, err := parsebenchy.ParseBenchyOutput(result.Output)
			if err != nil {
				return err
			}
			for _, line := range invocation.ResultLines {
				instructions := (*metrics)["INSTRUCTIONS"][line.Benchmark.Name]
				cycles := (*metrics)["CPU-CYCLES"][line.Benchmark.Name]
				instructionsNew, err := parsebenchy.ParseBenchyValue(instructions.New)
				if err != nil {
					return err
				}
				cyclesNew, err := parsebenchy.ParseBenchyValue(cycles.New)
				if err != nil {
					return err
				}
				instructionsOld, err := parsebenchy.ParseBenchyValue(instructions.Old)
				if err != nil {
					return err
				}
				cyclesOld, err := parsebenchy.ParseBenchyValue(cycles.Old)
				if err != nil {
					return err
				}
				ipcNew := instructionsNew / cyclesNew
				ipcOld := instructionsOld / cyclesOld
				resultValues := output.ResultValues{
					CPU: output.ResultValue{
						New:   (*metrics)["cpu"][line.Benchmark.Name].New,
						Delta: (*metrics)["cpu"][line.Benchmark.Name].Delta,
					},
					Instructions: output.ResultValue{
						New:   instructions.New,
						Delta: instructions.Delta,
					},
					IPC: output.ResultValue{
						New:   fmt.Sprintf("%.1f", ipcNew),
						Delta: percentDiff(ipcNew, ipcOld),
					},
					CacheMisses: output.ResultValue{
						New:   (*metrics)["CACHE-MISSES"][line.Benchmark.Name].New,
						Delta: (*metrics)["CACHE-MISSES"][line.Benchmark.Name].Delta,
					},
					BranchMisses: output.ResultValue{
						New:   (*metrics)["BRANCH-MISSES"][line.Benchmark.Name].New,
						Delta: (*metrics)["BRANCH-MISSES"][line.Benchmark.Name].Delta,
					},
				}
				lineText := output.FormatResultLine(resultValues)
				line.Value = &lineText
			}
			queue = queue[1:]
			if len(queue) == 0 {
				delete(execQueues, result.Device)
			} else {
				invocation := queue[0]
				now := time.Now()
				invocation.StartTime = &now
				go runBenchmark(*invocation, ch)
				execQueues[result.Device] = queue
			}
		case <-time.After(200 * time.Millisecond):
			scrollBack = output.PrintOutput(groups, scrollBack)
		}
	}

	// Render final result.
	output.PrintOutput(groups, scrollBack)

	return nil
}

func main() {
	google.Init()

	env, err := environment.GetEnvironment()
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}

	err = runBenchmarks(env)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
