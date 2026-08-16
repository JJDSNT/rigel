---
id: ISSUE-0004
title: "No test exercises the config surface, and three defects hid there"
status: open
priority: medium
type: test-coverage
owner: unassigned
created_at: 2026-08-15
updated_at: 2026-08-15
tags:
  - config
  - testing
  - api
related_files:
  - include/rigel/rigel_config.h
  - tests/test_timing.c
---

## What

Three defects of the same shape surfaced within an hour of each other, all in
`rigel_config_t`, none caught by the 27-test suite:

| Field | Defect |
| --- | --- |
| `clock_hz` | The default ignored `video_std`, so an NTSC context reported the PAL rate alongside NTSC geometry. Fixed. |
| `pixel_format` | Disagrees with `framebuffer.format`; see [[ISSUE-0002]]. |
| `enable_trace` | Read by nothing; see [[ISSUE-0003]]. |

The common cause is that no test configures a field to a non-default value and
checks it had an effect.

`clock_hz` is the instructive one: it *did* have a test. `test_timing.c`
asserted the PAL rate and NTSC geometry in the same breath, so the test
enshrined the inconsistency rather than catching it. A test written from the
implementation rather than from the intent will do that.

## What would fix the class

A configuration test that, for every field in `rigel_config_t`, sets it to
something other than its default and asserts an observable consequence:

- `clock_hz`, `video_std` — the rate reported, and that a frame's cycles
  convert to a frame's time
- `chip_ram_size`, `chipset_model` — the effective Chip RAM limit
- `pixel_format`, `framebuffer.*` — what `rigel_get_frame` reports and what
  lands in a host buffer
- `rtc_model`, `rtc_time` — the RTC registers after creation
- `serial.tx_instant` — a SERDAT write queued without baud pacing
- `cycle_exact` — a blit costing differently
- `enable_trace` — currently nothing, which is the point

It should fail when a field is added without wiring, which is what would have
caught all three.

## Notes

The harness found none of these, because it sets most fields explicitly and
consistently. They were found by reading `rigel_config_t` and counting
consumers of each field — worth repeating whenever the struct grows.
