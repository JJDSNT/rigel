---
id: ISSUE-0002
title: "pixel_format and framebuffer.format are two sources of truth and can disagree"
status: open
priority: medium
type: bug
owner: unassigned
created_at: 2026-08-15
updated_at: 2026-08-15
tags:
  - config
  - denise
  - video
  - api
related_files:
  - include/rigel/rigel_config.h
  - src/core/rigel_denise_api.c
  - src/chipset/denise/output/framebuffer.c
---

## What

`rigel_config_t` carries the pixel format twice: once as `pixel_format`, once
as `framebuffer.format`. They feed different outputs and nothing reconciles
them, so a host that sets one and reads the other is silently misled.

```c
rigel_config_t c = {0};
c.framebuffer.pixels = fb;
c.framebuffer.width  = 640;
c.framebuffer.height = 256;
c.framebuffer.pitch  = 640 * 2;
c.framebuffer.format = RIGEL_PIXEL_RGB565;   /* what the host asked for */

rigel_frame_t f;
rigel_get_frame(rigel_create(&c), &f);
/* f.format == RIGEL_PIXEL_RGBA8888 */
```

Denise writes RGB565 into the host's buffer, as asked. `rigel_get_frame`
reports RGBA8888, because it reads `config.pixel_format`, which was never set
and defaults to 0.

A host that trusts `frame.format` reads RGB565 data as RGBA8888.

## Why it is not simply a fix

The two fields are not redundant. They describe two independent outputs:

- `framebuffer.format` — the format Denise writes into the host's own buffer
  (`src/chipset/denise/output/framebuffer.c`)
- `pixel_format` — the format of the internal buffer `rigel_get_frame` hands
  back a pointer to (`src/core/rigel_denise_api.c`)

A host could legitimately want RGB565 going to a display buffer while pulling
RGBA8888 frames for screenshots. Making one inherit from the other would break
that.

Nor can "unset" be detected: `RIGEL_PIXEL_RGBA8888` is 0, so a zeroed config
and a deliberate choice of RGBA8888 are indistinguishable.

## Options

1. **Document it** and leave the behaviour. Cheapest, and keeps both outputs
   independent, but the footgun stays loaded — setting `framebuffer.format` is
   the natural gesture and leaves the other at a default that disagrees.
2. **Inherit when a target is configured** and `pixel_format` is 0. Fixes the
   common case; removes the ability to run the two paths at different formats.
3. **Renumber the enum** so 0 means "unset", then inherit only when genuinely
   unset. Correct, and an ABI break.

Needs a decision about whether the two-independent-formats case is real before
any of them.

## Notes

Rigel's own harness sets both, which is why this went unnoticed — by luck
rather than by knowing. See [[ISSUE-0004]] for the class of defect this
belongs to.
