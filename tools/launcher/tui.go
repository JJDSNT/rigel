// rigel/tools/launcher/tui.go
package main

import (
	"fmt"
	"os"
	"strconv"
	"strings"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

/* ------------------------------------------------------------------------- */
/* Result                                                                    */
/* ------------------------------------------------------------------------- */

type launchResult struct {
	kickstart string
	adf       string
	iso       string
	hdf       string
	cpu       string
	video     string // "pal" | "ntsc"
	chipset   string // "ocs" | "ecs"
	chipKB    int
	slowKB    int
	fastMB    int

	cycleExact bool
	headless   bool
	trace      bool
	frames     int

	// Filesystem handler for an ISO. Without it the CD is visible but has
	// nothing to mount it, so it travels with the ISO selection.
	odfs string

	cancelled bool
}

// harnessArgs builds the rigel-harness command line for this selection.
func (r launchResult) harnessArgs() []string {
	args := []string{r.kickstart}

	if r.adf != "" {
		args = append(args, "--adf", r.adf)
	}
	if r.hdf != "" {
		args = append(args, "--hdf", r.hdf)
	}
	if r.iso != "" {
		args = append(args, "--iso", r.iso)
	}
	if r.odfs != "" {
		args = append(args, "--odfs", r.odfs)
	}
	args = append(args, "--cpu", r.cpu)
	if r.video == "ntsc" {
		args = append(args, "--ntsc")
	} else {
		args = append(args, "--pal")
	}
	if r.chipset == "ecs" {
		args = append(args, "--ecs")
	}
	args = append(args, "--chip", strconv.Itoa(r.chipKB))
	if r.slowKB > 0 {
		args = append(args, "--slow", strconv.Itoa(r.slowKB))
	}
	if r.fastMB > 0 {
		args = append(args, "--fast", strconv.Itoa(r.fastMB))
	}
	if r.cycleExact {
		args = append(args, "--cycle-exact")
	}
	if r.trace {
		args = append(args, "--trace")
	}
	if r.headless {
		args = append(args, "--headless")
	}
	if r.frames > 0 {
		args = append(args, "--frames", strconv.Itoa(r.frames))
	}
	return args
}

/* ------------------------------------------------------------------------- */
/* Model                                                                     */
/* ------------------------------------------------------------------------- */

type activePane int

const (
	paneKickstart activePane = iota
	paneADF
	paneISO
	paneHDF
	paneCount
)

func (p activePane) title() string {
	switch p {
	case paneKickstart:
		return "Kickstart"
	case paneADF:
		return "Floppy (DF0)"
	case paneISO:
		return "CD-ROM"
	case paneHDF:
		return "Hard disk"
	}
	return ""
}

type model struct {
	roms []FileEntry
	adfs []FileEntry
	isos []FileEntry
	hdfs []FileEntry

	cursor [paneCount]int
	active activePane

	cpu        string
	video      string
	chipset    string
	chipKB     int
	slowKB     int
	fastMB     int
	cycleExact bool
	headless   bool
	trace      bool
	frames     int

	width     int
	height    int
	quitting  bool
	cancelled bool
}

var cpuChoices = []string{"68000", "68010", "68ec020", "68020", "68030", "68040"}

// Chip RAM sizes Agnus can actually address, in KB.
var chipChoices = []int{512, 1024, 2048}

// Slow ("ranger") RAM at 0xC00000; 0 means none.
var slowChoices = []int{0, 512}

// Zorro II Fast RAM in MB; 0 means none. Only these sizes have autoconfig
// codes, so the harness rejects anything else.
var fastChoices = []int{0, 1, 2, 4, 8}

// 0 means "run until the window is closed".
var frameChoices = []int{0, 60, 300, 600, 1200}

func cycleString(list []string, cur string) string {
	for i, v := range list {
		if v == cur {
			return list[(i+1)%len(list)]
		}
	}
	return list[0]
}

func cycleInt(list []int, cur int) int {
	for i, v := range list {
		if v == cur {
			return list[(i+1)%len(list)]
		}
	}
	return list[0]
}

// odfsPath resolves the ISO filesystem handler, but only when an ISO is
// actually selected — passing --odfs with no CD just makes the harness warn.
// ODFS overrides the built location; scripts/build-odfs.sh puts it there.
func odfsPath(iso string) string {
	if iso == "" {
		return ""
	}
	if env := os.Getenv("ODFS"); env != "" {
		return env
	}
	const built = "external/ODFileSystem/build/amiga/ODFileSystem"
	if _, err := os.Stat(built); err == nil {
		return built
	}
	return ""
}

func defaultROMIndex(roms []FileEntry) int {
	// Skip the "[No Kickstart]" placeholder when a real ROM exists.
	if len(roms) > 1 {
		return 1
	}
	return 0
}

func runLauncher(roms, adfs, isos, hdfs []FileEntry) (launchResult, error) {
	m := model{
		roms:    roms,
		adfs:    adfs,
		isos:    isos,
		hdfs:    hdfs,
		active:  paneKickstart,
		cpu:     "68000",
		video:   "pal",
		chipset: "ocs",
		chipKB:  512,
		slowKB:  0,
		fastMB:  0,
		frames:  0,
	}
	m.cursor[paneKickstart] = defaultROMIndex(roms)

	p := tea.NewProgram(m, tea.WithAltScreen())
	finalModel, err := p.Run()
	if err != nil {
		return launchResult{}, err
	}

	fm := finalModel.(model)
	if fm.cancelled {
		return launchResult{cancelled: true}, nil
	}

	pick := func(list []FileEntry, idx int) string {
		if idx < 0 || idx >= len(list) || list[idx].None {
			return ""
		}
		return list[idx].Path
	}

	kickstart := pick(fm.roms, fm.cursor[paneKickstart])
	if kickstart == "" {
		return launchResult{cancelled: true}, nil
	}

	return launchResult{
		kickstart:  kickstart,
		adf:        pick(fm.adfs, fm.cursor[paneADF]),
		iso:        pick(fm.isos, fm.cursor[paneISO]),
		hdf:        pick(fm.hdfs, fm.cursor[paneHDF]),
		odfs:       odfsPath(pick(fm.isos, fm.cursor[paneISO])),
		cpu:        fm.cpu,
		video:      fm.video,
		chipset:    fm.chipset,
		chipKB:     fm.chipKB,
		slowKB:     fm.slowKB,
		fastMB:     fm.fastMB,
		cycleExact: fm.cycleExact,
		headless:   fm.headless,
		trace:      fm.trace,
		frames:     fm.frames,
	}, nil
}

/* ------------------------------------------------------------------------- */
/* Bubbletea                                                                 */
/* ------------------------------------------------------------------------- */

func (m model) Init() tea.Cmd { return nil }

func (m model) list(p activePane) []FileEntry {
	switch p {
	case paneKickstart:
		return m.roms
	case paneADF:
		return m.adfs
	case paneISO:
		return m.isos
	case paneHDF:
		return m.hdfs
	}
	return nil
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.WindowSizeMsg:
		m.width = msg.Width
		m.height = msg.Height
		return m, nil

	case tea.KeyMsg:
		switch msg.String() {
		case "ctrl+c", "q", "esc":
			m.cancelled = true
			m.quitting = true
			return m, tea.Quit

		case "enter":
			m.quitting = true
			return m, tea.Quit

		case "tab", "right", "l":
			m.active = (m.active + 1) % paneCount
			return m, nil

		case "shift+tab", "left", "h":
			m.active = (m.active + paneCount - 1) % paneCount
			return m, nil

		case "up", "k":
			if m.cursor[m.active] > 0 {
				m.cursor[m.active]--
			}
			return m, nil

		case "down", "j":
			if m.cursor[m.active] < len(m.list(m.active))-1 {
				m.cursor[m.active]++
			}
			return m, nil

		case "c":
			m.cpu = cycleString(cpuChoices, m.cpu)
		case "v":
			m.video = cycleString([]string{"pal", "ntsc"}, m.video)
		case "s":
			m.chipset = cycleString([]string{"ocs", "ecs"}, m.chipset)
		case "m":
			m.chipKB = cycleInt(chipChoices, m.chipKB)
		case "w":
			m.slowKB = cycleInt(slowChoices, m.slowKB)
		case "r":
			m.fastMB = cycleInt(fastChoices, m.fastMB)
		case "x":
			m.cycleExact = !m.cycleExact
		case "d":
			m.headless = !m.headless
		case "t":
			m.trace = !m.trace
		case "f":
			m.frames = cycleInt(frameChoices, m.frames)
		}
		return m, nil
	}

	return m, nil
}

func badge(on bool) string {
	if on {
		return onBadgeStyle.Render("ON")
	}
	return offBadgeStyle.Render("OFF")
}

// renderList draws one media pane. The active pane opens up to `visible`
// entries; the others collapse to their current selection, so all four fit in
// the column no matter how many images the directory holds.
func (m model) renderList(p activePane, visible int) string {
	list := m.list(p)
	cursor := m.cursor[p]

	head := p.title()
	if p == m.active {
		head += "  ◂"
	}

	var b strings.Builder
	b.WriteString(sectionTitleStyle.Render(head))
	b.WriteString("\n")

	if len(list) == 0 {
		b.WriteString(mutedStyle.Render("  (none found)"))
		return b.String()
	}

	clip := func(name string) string {
		if len(name) > colWidth-6 {
			return name[:colWidth-9] + "..."
		}
		return name
	}

	if p != m.active {
		// Collapsed: just show what is selected, and how much is hidden.
		name := clip(list[cursor].Name)
		line := "   " + itemStyle.Render(name)
		if len(list) > 1 {
			line += mutedStyle.Render(fmt.Sprintf("  (%d/%d)", cursor+1, len(list)))
		}
		b.WriteString(line)
		return b.String()
	}

	start := cursor - visible/2
	if start < 0 {
		start = 0
	}
	if start+visible > len(list) {
		start = len(list) - visible
	}
	if start < 0 {
		start = 0
	}
	end := start + visible
	if end > len(list) {
		end = len(list)
	}

	if start > 0 {
		b.WriteString(mutedStyle.Render(fmt.Sprintf("   ↑ %d more", start)) + "\n")
	}
	for i := start; i < end; i++ {
		name := clip(list[i].Name)
		if i == cursor {
			b.WriteString("  " + selectedItemStyle.Render(name) + "\n")
		} else {
			b.WriteString("   " + itemStyle.Render(name) + "\n")
		}
	}
	if end < len(list) {
		b.WriteString(mutedStyle.Render(fmt.Sprintf("   ↓ %d more", len(list)-end)))
	}

	return strings.TrimRight(b.String(), "\n")
}

func (m model) renderOptions() string {
	slow := "off"
	if m.slowKB > 0 {
		slow = fmt.Sprintf("%d KB", m.slowKB)
	}
	fast := "off"
	if m.fastMB > 0 {
		fast = fmt.Sprintf("%d MB", m.fastMB)
	}
	frames := "until closed"
	if m.frames > 0 {
		frames = strconv.Itoa(m.frames)
	}

	// One padded label width for every row, so the value column lines up
	// whether the value is styled text or an ON/OFF badge.
	row := func(key, label, value string) string {
		return fmt.Sprintf("(%s) %-12s %s", key, label, value)
	}
	// badge() styles with one column of internal padding, so its rows need one
	// less space to line up with the plain-text ones.
	badgeRow := func(key, label string, on bool) string {
		return fmt.Sprintf("(%s) %-11s %s", key, label, badge(on))
	}

	rows := []string{
		row("c", "CPU", itemStyle.Render(m.cpu)),
		row("v", "Video", itemStyle.Render(strings.ToUpper(m.video))),
		row("s", "Chipset", itemStyle.Render(strings.ToUpper(m.chipset))),
		row("m", "Chip RAM", itemStyle.Render(fmt.Sprintf("%d KB", m.chipKB))),
		row("w", "Slow RAM", itemStyle.Render(slow)),
		row("r", "Fast RAM", itemStyle.Render(fast)),
		badgeRow("x", "Cycle-exact", m.cycleExact),
		badgeRow("d", "Headless", m.headless),
		badgeRow("t", "Trace", m.trace),
		row("f", "Frames", itemStyle.Render(frames)),
	}

	var b strings.Builder
	b.WriteString(sectionTitleStyle.Render("Machine"))
	b.WriteString("\n")
	for _, r := range rows {
		b.WriteString("   " + r + "\n")
	}
	return strings.TrimRight(b.String(), "\n")
}

func (m model) renderPanel() string {
	var b strings.Builder

	b.WriteString(headerBlockStyle.Render(
		headerTitleStyle.Render("Rigel Harness") + "\n" +
			headerSubtitleStyle.Render("Musashi + Rigel chipset"),
	))
	b.WriteString("\n")

	// Left: every media pane, the active one expanded. Right: the machine.
	var left strings.Builder
	for p := paneKickstart; p < paneCount; p++ {
		left.WriteString(m.renderList(p, 8))
		left.WriteString("\n")
	}

	columns := lipgloss.JoinHorizontal(
		lipgloss.Top,
		columnStyle.Render(strings.TrimRight(left.String(), "\n")),
		columnGapStyle.Render(""),
		columnStyle.Render(m.renderOptions()),
	)
	b.WriteString(columns)
	b.WriteString("\n\n")

	// Preview of exactly what will run.
	preview := "rigel-harness " + strings.Join(m.previewArgs(), " ")
	if len(preview) > appWidth-8 {
		preview = preview[:appWidth-11] + "..."
	}
	b.WriteString(commandStyle.Render(preview))
	b.WriteString("\n")

	b.WriteString(helpStyle.Render(
		"tab/←→ pane · ↑↓ select · letter keys toggle · enter launch · q cancel",
	))

	return panelStyle.Render(b.String())
}

func (m model) previewArgs() []string {
	pick := func(list []FileEntry, idx int) string {
		if idx < 0 || idx >= len(list) || list[idx].None {
			return ""
		}
		return list[idx].Path
	}
	r := launchResult{
		kickstart:  pick(m.roms, m.cursor[paneKickstart]),
		adf:        pick(m.adfs, m.cursor[paneADF]),
		iso:        pick(m.isos, m.cursor[paneISO]),
		hdf:        pick(m.hdfs, m.cursor[paneHDF]),
		cpu:        m.cpu,
		video:      m.video,
		chipset:    m.chipset,
		chipKB:     m.chipKB,
		slowKB:     m.slowKB,
		fastMB:     m.fastMB,
		cycleExact: m.cycleExact,
		headless:   m.headless,
		trace:      m.trace,
		frames:     m.frames,
	}
	args := r.harnessArgs()
	// Show basenames only; full paths would blow the panel width.
	for i, a := range args {
		if idx := strings.LastIndex(a, "/"); idx >= 0 {
			args[i] = a[idx+1:]
		}
	}
	return args
}

func (m model) View() string {
	if m.quitting {
		return ""
	}
	if m.width == 0 || m.height == 0 {
		return "Loading Rigel launcher..."
	}

	return lipgloss.Place(
		m.width, m.height,
		lipgloss.Center, lipgloss.Center,
		m.renderPanel(),
	)
}
