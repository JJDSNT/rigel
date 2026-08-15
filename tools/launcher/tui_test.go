// rigel/tools/launcher/tui_test.go
package main

import (
	"strings"
	"testing"

	tea "github.com/charmbracelet/bubbletea"
)

func testModel() model {
	roms := []FileEntry{
		{Name: "[No Kickstart]", None: true},
		{Name: "KS13.rom", Path: "/images/roms/KS13.rom"},
		{Name: "KS31.rom", Path: "/images/roms/KS31.rom"},
	}
	adfs := []FileEntry{
		{Name: "[No disk]", None: true},
		{Name: "wb13.adf", Path: "/images/disks/wb13.adf"},
	}
	isos := []FileEntry{{Name: "[No CD-ROM]", None: true}}
	hdfs := []FileEntry{{Name: "[No hard disk]", None: true}}

	m := model{
		roms: roms, adfs: adfs, isos: isos, hdfs: hdfs,
		active: paneKickstart,
		cpu:    "68000", video: "pal", chipset: "ocs",
		chipKB: 512, slowKB: 0,
		width: 100, height: 40,
	}
	m.cursor[paneKickstart] = defaultROMIndex(roms)
	return m
}

func press(m model, key string) model {
	next, _ := m.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune(key)})
	return next.(model)
}

func TestRenderPanelDoesNotPanic(t *testing.T) {
	m := testModel()
	out := m.renderPanel()
	if !strings.Contains(out, "Rigel Harness") {
		t.Fatalf("panel is missing its header:\n%s", out)
	}
	if !strings.Contains(out, "KS13.rom") {
		t.Fatalf("panel is missing the ROM list:\n%s", out)
	}
}

// Every pane must render, including the ones with only a placeholder entry.
func TestRenderEveryPane(t *testing.T) {
	for p := paneKickstart; p < paneCount; p++ {
		m := testModel()
		m.active = p
		if out := m.renderPanel(); out == "" {
			t.Fatalf("pane %v rendered empty", p)
		}
	}
}

func TestOptionKeysCycle(t *testing.T) {
	m := testModel()

	if m = press(m, "c"); m.cpu != "68010" {
		t.Fatalf("cpu after 'c' = %q, want 68010", m.cpu)
	}
	if m = press(m, "v"); m.video != "ntsc" {
		t.Fatalf("video after 'v' = %q, want ntsc", m.video)
	}
	if m = press(m, "s"); m.chipset != "ecs" {
		t.Fatalf("chipset after 's' = %q, want ecs", m.chipset)
	}
	if m = press(m, "m"); m.chipKB != 1024 {
		t.Fatalf("chipKB after 'm' = %d, want 1024", m.chipKB)
	}
	if m = press(m, "w"); m.slowKB != 512 {
		t.Fatalf("slowKB after 'w' = %d, want 512", m.slowKB)
	}
	if m = press(m, "x"); !m.cycleExact {
		t.Fatal("cycleExact should be on after 'x'")
	}
	if m = press(m, "x"); m.cycleExact {
		t.Fatal("cycleExact should toggle back off")
	}
}

// Cycling a choice list all the way round must return to where it started.
func TestCycleWrapsAround(t *testing.T) {
	m := testModel()
	start := m.cpu
	for range cpuChoices {
		m = press(m, "c")
	}
	if m.cpu != start {
		t.Fatalf("cpu after a full cycle = %q, want %q", m.cpu, start)
	}
}

func TestPaneNavigationWraps(t *testing.T) {
	m := testModel()
	for i := 0; i < int(paneCount); i++ {
		next, _ := m.Update(tea.KeyMsg{Type: tea.KeyTab})
		m = next.(model)
	}
	if m.active != paneKickstart {
		t.Fatalf("active pane after a full loop = %v, want paneKickstart", m.active)
	}
}

func TestCursorStaysInRange(t *testing.T) {
	m := testModel()
	// Far past the end, then far past the start.
	for i := 0; i < 50; i++ {
		m = press(m, "j")
	}
	if got := m.cursor[paneKickstart]; got != len(m.roms)-1 {
		t.Fatalf("cursor = %d, want %d", got, len(m.roms)-1)
	}
	for i := 0; i < 50; i++ {
		m = press(m, "k")
	}
	if got := m.cursor[paneKickstart]; got != 0 {
		t.Fatalf("cursor = %d, want 0", got)
	}
}

func TestHarnessArgs(t *testing.T) {
	r := launchResult{
		kickstart: "/images/roms/KS13.rom",
		adf:       "/images/disks/wb13.adf",
		cpu:       "68000",
		video:     "pal",
		chipset:   "ocs",
		chipKB:    512,
	}
	got := strings.Join(r.harnessArgs(), " ")
	want := "/images/roms/KS13.rom --adf /images/disks/wb13.adf --cpu 68000 --pal --chip 512"
	if got != want {
		t.Fatalf("harnessArgs()\n got: %s\nwant: %s", got, want)
	}
}

func TestHarnessArgsFullOptions(t *testing.T) {
	r := launchResult{
		kickstart:  "/ks.rom",
		cpu:        "68030",
		video:      "ntsc",
		chipset:    "ecs",
		chipKB:     2048,
		slowKB:     512,
		cycleExact: true,
		headless:   true,
		trace:      true,
		frames:     600,
	}
	got := strings.Join(r.harnessArgs(), " ")
	want := "/ks.rom --cpu 68030 --ntsc --ecs --chip 2048 --slow 512 " +
		"--cycle-exact --trace --headless --frames 600"
	if got != want {
		t.Fatalf("harnessArgs()\n got: %s\nwant: %s", got, want)
	}
}

// "[No disk]" must not turn into a --adf flag pointing at nothing.
func TestPlaceholderEntriesAreNotPassedOn(t *testing.T) {
	m := testModel()
	m.cursor[paneADF] = 0 // the "[No disk]" placeholder
	args := strings.Join(m.previewArgs(), " ")
	if strings.Contains(args, "--adf") {
		t.Fatalf("placeholder leaked into args: %s", args)
	}
}

func TestQuitCancels(t *testing.T) {
	m := testModel()
	next, _ := m.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune("q")})
	if !next.(model).cancelled {
		t.Fatal("'q' should cancel")
	}
}

// The CD-ROM and Hard disk panes are first-class in the two-column layout, so
// a selection there has to reach the command line. It silently did not before.
func TestHarnessArgsCarriesHdfAndIso(t *testing.T) {
	r := launchResult{
		kickstart: "/ks.rom",
		hdf:       "/images/disks/wb20.hdf",
		iso:       "/images/disks/aros.iso",
		odfs:      "/build/ODFileSystem",
		cpu:       "68000",
		video:     "pal",
		chipset:   "ocs",
		chipKB:    512,
	}
	got := strings.Join(r.harnessArgs(), " ")
	want := "/ks.rom --hdf /images/disks/wb20.hdf --iso /images/disks/aros.iso " +
		"--odfs /build/ODFileSystem --cpu 68000 --pal --chip 512"
	if got != want {
		t.Fatalf("harnessArgs()\n got: %s\nwant: %s", got, want)
	}
}

// --odfs with no CD only makes the harness warn, so it must not be emitted.
func TestOdfsOnlyWithAnIso(t *testing.T) {
	if got := odfsPath(""); got != "" {
		t.Fatalf("odfsPath(\"\") = %q, want empty", got)
	}
	t.Setenv("ODFS", "/somewhere/ODFileSystem")
	if got := odfsPath("/disk.iso"); got != "/somewhere/ODFileSystem" {
		t.Fatalf("odfsPath with ODFS set = %q", got)
	}
	if got := odfsPath(""); got != "" {
		t.Fatalf("odfsPath(\"\") with ODFS set = %q, want empty", got)
	}
}

// Every pane feeds the preview now that all four are on screen at once.
func TestPreviewShowsEveryPane(t *testing.T) {
	m := testModel()
	m.isos = append(m.isos, FileEntry{Name: "cd.iso", Path: "/images/disks/cd.iso"})
	m.hdfs = append(m.hdfs, FileEntry{Name: "hd.hdf", Path: "/images/disks/hd.hdf"})
	m.cursor[paneADF] = 1
	m.cursor[paneISO] = 1
	m.cursor[paneHDF] = 1

	args := strings.Join(m.previewArgs(), " ")
	for _, want := range []string{"--adf", "wb13.adf", "--iso", "cd.iso", "--hdf", "hd.hdf"} {
		if !strings.Contains(args, want) {
			t.Fatalf("preview is missing %q: %s", want, args)
		}
	}
}

// Collapsed panes still have to render, and show which entry is selected.
func TestCollapsedPaneShowsSelection(t *testing.T) {
	m := testModel()
	m.active = paneKickstart
	m.cursor[paneADF] = 1

	out := m.renderList(paneADF, 8)
	if !strings.Contains(out, "wb13.adf") {
		t.Fatalf("collapsed pane does not show its selection:\n%s", out)
	}
	if !strings.Contains(out, "2/2") {
		t.Fatalf("collapsed pane does not show its position:\n%s", out)
	}
}

// Fast RAM is a Zorro II board, so only the sizes with autoconfig codes are
// offered; the harness rejects anything else.
func TestFastRamCycles(t *testing.T) {
	m := testModel()
	for _, want := range []int{1, 2, 4, 8, 0} {
		m = press(m, "r")
		if m.fastMB != want {
			t.Fatalf("fastMB = %d, want %d", m.fastMB, want)
		}
	}
}

func TestHarnessArgsCarriesFastRam(t *testing.T) {
	r := launchResult{
		kickstart: "/ks.rom", cpu: "68000", video: "pal",
		chipset: "ocs", chipKB: 512, fastMB: 8,
	}
	got := strings.Join(r.harnessArgs(), " ")
	if !strings.Contains(got, "--fast 8") {
		t.Fatalf("missing --fast: %s", got)
	}
	// Off must not emit the flag at all.
	r.fastMB = 0
	if strings.Contains(strings.Join(r.harnessArgs(), " "), "--fast") {
		t.Fatal("--fast emitted when Fast RAM is off")
	}
}

// The defaults follow the legacy launcher, except the CPU: 68000 rather than
// 68040, since that is the machine Rigel's chipset work targets.
func TestDefaults(t *testing.T) {
	m := testModel()
	for _, c := range []struct {
		name string
		got  any
		want any
	}{
		{"cpu", m.cpu, "68000"},
		{"video", m.video, "pal"},
		{"chipset", m.chipset, "ocs"},
		{"chipKB", m.chipKB, 512},
		{"slowKB", m.slowKB, 0},
		{"fastMB", m.fastMB, 0},
		{"cycleExact", m.cycleExact, false},
		{"headless", m.headless, false},
		{"trace", m.trace, false},
	} {
		if c.got != c.want {
			t.Errorf("default %s = %v, want %v", c.name, c.got, c.want)
		}
	}
}
