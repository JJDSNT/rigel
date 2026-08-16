---
id: ISSUE-0003
title: "enable_trace is a config field nothing reads"
status: open
priority: low
type: bug
owner: unassigned
created_at: 2026-08-15
updated_at: 2026-08-15
tags:
  - config
  - api
  - tracing
related_files:
  - include/rigel/rigel_config.h
  - src/debug/trace.c
---

## What

`rigel_config_t.enable_trace` is declared in the public header and read
nowhere:

```sh
$ grep -rn enable_trace include/ src/
include/rigel/rigel_config.h:89:    bool              enable_trace;
```

A host sets it and nothing happens. Every other field in the struct is
consumed somewhere.

## What to decide

Either wire it to whatever tracing it was meant to gate — `src/debug/trace.c`
exists and has its own enable path — or remove it from the public config.
Leaving a documented switch that does nothing is the worst of the three.

Removing it is an ABI break for anything already compiled against the header,
which for now is the harness and nothing else.

## Notes

Found by counting consumers of every config field, which is also how
[[ISSUE-0002]] and the `clock_hz` defect turned up. See [[ISSUE-0004]].
