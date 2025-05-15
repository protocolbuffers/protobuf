package environment

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"
)

type DeviceMap map[string]string

type Environment struct {
	CLNumber           string
	OutputDir          string
	BlazeOutputBaseDir string
	AndroidDevices     DeviceMap // Maps device name to serial for ANDROID_SERIAL
}

// Gets the current CL number by running "/google/data/ro/teams/fig/bin/vcstool pending-change-number"
// and getting the output.
func getCurrentCL() (string, error) {
	cmd := exec.Command("/google/data/ro/teams/fig/bin/vcstool", "pending-change-number")
	output, err := cmd.Output()

	if err != nil {
		return "", fmt.Errorf("failed to get current CL number: %v", err)
	}

	// Remove trailing whitespace.
	cl := strings.TrimSpace(string(output))

	if cl == "-1" {
		return "", fmt.Errorf("client should have a single pending CL")
	}

	return cl, nil
}

func getOutputDir(homeDir string, currentCL string) string {
	timestampDirStr := time.Now().Format("2006-01-02_15-04-05")
	baseOutputDir := filepath.Join(homeDir, "upb_android_benchmarks")
	clDir := filepath.Join(baseOutputDir, currentCL)
	return filepath.Join(clDir, timestampDirStr)
}

func getAndroidDevices() (DeviceMap, error) {
	cmd := exec.Command("adb", "devices", "-l")
	output, err := cmd.Output()

	if err != nil {
		return nil, fmt.Errorf("failed to get Android devices from adb: %v", err)
	}

	// Sample output:
	//   List of devices attached
	//   localhost:38889        device product:mokey model:mokey device:mokey transport_id:7
	//   localhost:39933        device product:tegu model:Pixel_9a device:tegu transport_id:8
	//
	// Parse this into a DeviceMap.
	outputStr := string(output)
	lines := strings.Split(outputStr, "\n")

	// Discard first line (header) and last line (empty).
	lines = lines[1 : len(lines)-2]

	devices := make(DeviceMap)
	for _, line := range lines {
		parts := strings.Fields(line)
		serial := parts[0]
		attrs := make(map[string]string)
		for _, attr := range parts[2:] {
			k, v, found := strings.Cut(attr, ":")
			if !found {
				return nil, fmt.Errorf("invalid Android device line: %s", line)
			}
			attrs[k] = v
		}
		devices[attrs["model"]] = serial
	}

	return devices, nil
}

func GetEnvironment() (*Environment, error) {
	currentCL, err := getCurrentCL()
	if err != nil {
		return nil, fmt.Errorf("failed to get current CL number: %w", err)
	}

	homeDir, err := os.UserHomeDir()
	if err != nil {
		return nil, fmt.Errorf("failed to get user home directory: %w", err)
	}

	devices, err := getAndroidDevices()
	if err != nil {
		return nil, fmt.Errorf("failed to get Android devices: %v", err)
	}

	cl, err := getCurrentCL()
	if err != nil {
		return nil, fmt.Errorf("failed to get current CL number: %v", err)
	}

	return &Environment{
		CLNumber:           cl,
		OutputDir:          getOutputDir(homeDir, currentCL),
		BlazeOutputBaseDir: filepath.Join(homeDir, "blaze_output"),
		AndroidDevices:     devices,
	}, nil
}
