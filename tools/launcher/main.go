// rigel/tools/launcher/main.go
package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

const usage = `usage: rigel-launcher [media-dir] [options]

  media-dir       root holding the images (default: ./images)
                  If it has roms/ and disks/ subdirectories those are used;
                  otherwise everything is scanned from media-dir itself.

  -o FILE         write the selection to FILE as shell assignments
  -exec PATH      run PATH (rigel-harness) with the selected options
  -h, --help      this text

With neither -o nor -exec the selection goes to stdout, ready for eval:

  eval "$(rigel-launcher images)"
  rigel-harness $RIGEL_HARNESS_ARGS
`

type paths struct {
	roms  string
	disks string
}

// The legacy layout keeps ROMs and disks in sibling directories. A flat
// directory holding everything is also fine, and is what a fresh checkout of
// rigel looks like.
func resolvePaths(root string) paths {
	romsDir := filepath.Join(root, "roms")
	disksDir := filepath.Join(root, "disks")

	romsInfo, romsErr := os.Stat(romsDir)
	disksInfo, disksErr := os.Stat(disksDir)

	if romsErr == nil && romsInfo.IsDir() && disksErr == nil && disksInfo.IsDir() {
		return paths{roms: romsDir, disks: disksDir}
	}
	return paths{roms: root, disks: root}
}

func shellQuote(s string) string {
	if s == "" {
		return ""
	}
	return "'" + strings.ReplaceAll(s, "'", `'\''`) + "'"
}

func main() {
	root := "images"
	outputFile := ""
	execPath := ""

	args := os.Args[1:]
	for i := 0; i < len(args); i++ {
		switch args[i] {
		case "-h", "--help":
			fmt.Print(usage)
			return
		case "-o":
			if i+1 >= len(args) {
				fmt.Fprintln(os.Stderr, "rigel-launcher: -o needs a value")
				os.Exit(2)
			}
			i++
			outputFile = args[i]
		case "-exec":
			if i+1 >= len(args) {
				fmt.Fprintln(os.Stderr, "rigel-launcher: -exec needs a value")
				os.Exit(2)
			}
			i++
			execPath = args[i]
		default:
			if strings.HasPrefix(args[i], "-") {
				fmt.Fprintf(os.Stderr, "rigel-launcher: unknown option %s\n", args[i])
				os.Exit(2)
			}
			root = args[i]
		}
	}

	p := resolvePaths(root)

	roms, err := scanROMs(p.roms)
	if err != nil {
		fmt.Fprintf(os.Stderr, "rigel-launcher: cannot scan %s: %v\n", p.roms, err)
		os.Exit(1)
	}
	// scanROMs always prepends the "[No Kickstart]" entry, so a directory with
	// no ROM at all still yields one element.
	if len(roms) <= 1 {
		fmt.Fprintf(os.Stderr, "rigel-launcher: no ROM images found in %s\n", p.roms)
		os.Exit(1)
	}

	adfs, _ := scanADFs(p.disks)
	isos, _ := scanISOs(p.disks)
	hdfs, _ := scanHDFs(p.disks)

	result, err := runLauncher(roms, adfs, isos, hdfs)
	if err != nil {
		fmt.Fprintf(os.Stderr, "rigel-launcher: %v\n", err)
		os.Exit(1)
	}
	if result.cancelled {
		os.Exit(130)
	}

	harnessArgs := result.harnessArgs()

	if execPath != "" {
		cmd := exec.Command(execPath, harnessArgs...)
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		cmd.Stdin = os.Stdin
		if err := cmd.Run(); err != nil {
			if ee, ok := err.(*exec.ExitError); ok {
				os.Exit(ee.ExitCode())
			}
			fmt.Fprintf(os.Stderr, "rigel-launcher: %v\n", err)
			os.Exit(1)
		}
		return
	}

	quoted := make([]string, 0, len(harnessArgs))
	for _, a := range harnessArgs {
		quoted = append(quoted, shellQuote(a))
	}

	var b strings.Builder
	fmt.Fprintf(&b, "KICKSTART=%s\n", shellQuote(result.kickstart))
	fmt.Fprintf(&b, "ADF=%s\n", shellQuote(result.adf))
	fmt.Fprintf(&b, "ISO=%s\n", shellQuote(result.iso))
	fmt.Fprintf(&b, "HDF=%s\n", shellQuote(result.hdf))
	fmt.Fprintf(&b, "HARNESS_CPU=%s\n", result.cpu)
	fmt.Fprintf(&b, "RIGEL_HARNESS_ARGS=%s\n", shellQuote(strings.Join(quoted, " ")))

	if outputFile != "" {
		if err := os.WriteFile(outputFile, []byte(b.String()), 0o644); err != nil {
			fmt.Fprintf(os.Stderr, "rigel-launcher: cannot write %s: %v\n", outputFile, err)
			os.Exit(1)
		}
		return
	}

	fmt.Print(b.String())
}
