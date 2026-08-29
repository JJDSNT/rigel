---
id: ISSUE-0007
title: "Paula audio is only available pre-mixed: no per-channel state or output"
status: open
priority: medium
type: enhancement
owner: unassigned
created_at: 2026-08-29
updated_at: 2026-08-29
tags:
  - audio
  - paula
  - api
  - host-integration
related_files:
  - include/rigel/rigel_audio.h
  - src/chipset/paula/
---

## What the API gives today

```c
typedef struct rigel_audio_sample {
    rigel_i16 left;
    rigel_i16 right;
} rigel_audio_sample_t;

rigel_audio_sample_t rigel_get_audio_sample(const RigelContext *ctx);
```

One mixed stereo pair, with Paula's channel-to-side assignment already applied.
That is the right default and it is enough to make a machine audible: a host
can take it at the `RIGEL_EVENT_HBLANK` tick (~15.6 kHz PAL) and hand it
straight to its own audio stack. Bellatrix's plan is exactly that -- feed it to
an AHI unit and let the existing HDMI driver play it.

## What a host cannot do with it

Everything that needs the four voices separately:

- **Per-voice volume and mute while debugging.** With a pre-mixed pair, a wrong
  period or a stuck DMA pointer on one channel can only be found by ear.
- **Resampling quality.** Mixing happens at whatever rate the host polls. A
  host that wants 48 kHz output is upsampling an already-mixed 15.6 kHz signal;
  with the voices separate it could resample each one from its own period,
  which is what a modern Amiga emulator does.
- **Handing the voices to a mixer that wants channels.** AHI on the Bellatrix
  side is a mixing API with its own channels and its own panning. Giving it one
  pre-mixed pair wastes it; giving it four voices lets it do the job it exists
  for.

## What the host actually needs is state, not samples

An earlier draft of this issue asked for the four voices' rendered samples.
That was the wrong ask, and the reason is worth recording, because it makes the
request smaller rather than larger.

A host with its own mixer does not want Paula's rendered output at all. It wants
Paula's *parameters*, so its mixer can play the guest's own sample data at the
host's rate and at the pitch the period register asks for. This is what
NallePuh (`https://github.com/khval/NallePuh`, Paula emulation for AmigaOS 4.1)
does: it hands AHI a dynamic sound spanning the address space, points it at
whatever `AUDxLCH/LCL` holds, and translates the registers:

```c
struct AHISampleInfo si = { AHIST_M8S, 0, 0xffffffff };
AHI_LoadSound( 0, AHIST_DYNAMICSAMPLE, &si, ctrl );
AHI_SetFreq( channel, chip_freq / period, ctrl, AHISF_IMM );
AHI_SetVol ( channel, volume << 10, pan,   ctrl, AHISF_IMM );
```

No emulator-rendered sample ever crosses that boundary, and the sound stays
correct while the emulated chipset runs slowly -- which matters here, since
ISSUE-0006 has this chipset ~3.8x short of realtime on the host's target
hardware.

## The shape of the request

Something alongside the existing call, not replacing it -- the mixed pair stays
the simple path, and the harness keeps using it:

```c
typedef struct rigel_audio_channel {
    rigel_u32 location;   /* AUDxLC as the DMA engine currently holds it */
    rigel_u16 length;     /* AUDxLEN, in words */
    rigel_u16 period;     /* AUDxPER */
    rigel_u8  volume;     /* AUDxVOL, 0..64 */
    bool      dma_on;     /* DMACON bit for this channel */
    bool      right;      /* side this voice is panned to */
} rigel_audio_channel_t;

void rigel_get_audio_channels(const RigelContext *ctx,
                              rigel_audio_channel_t out[4]);
```

Field set is a suggestion, not a specification. What matters is that a host can
see, per voice, **where the sample data is, how long it is, how fast to play it,
how loud, and whether it is running** -- enough to hand the voice to its own
mixer without Rigel rendering anything.

Rigel stays authoritative for everything the guest observes: the audio
interrupts, the DMA pointer's progress through the sample, and the register
semantics. Only the audible rendering moves to the host.

## The rendered per-voice output is still wanted, for a different reason

Parameter hand-off cannot represent everything a program does to Paula: a period
or volume rewritten many times per frame, a one-shot to loop switch at a
sample-exact moment, anything that makes the waveform depend on chipset timing
rather than on a set of register values. An interception layer with no chipset
underneath simply loses those -- but Rigel models them, so a host driving Rigel
should be able to fall back to the rendered waveform for the voice that needs
it, while the other voices stay on the cheap path.

That makes a per-voice rendered sample a second, independent ask rather than a
weaker version of the first:

- **channel state** unblocks the common case and is what an integration needs
  first;
- **per-voice rendered output** covers the programs the common case cannot
  serve, and is also what makes per-voice debugging and per-voice resampling
  possible.

A host that has both can choose per voice, at run time, from Rigel's own state,
without either choice changing what the guest observes.

## Why the host wants this more than "later"

Bellatrix has since written the host side up as its own `AI_context/issues/
ISSUE-0071.md`, and it concludes that four independent channels is the
*preferred* architecture rather than a refinement. The reason is AHI: on the
host side AHI is itself a mixer with channels, volumes and stereo positions, so
handing it one pre-mixed pair discards information it is built to carry, and
per-channel identity has to survive from Paula all the way to it. The mixed pair
stays the simple path for hosts that do not have a mixer of their own -- and for
the harness.

## Priority

Lower than ISSUE-0006, but no longer waiting on it. An earlier draft said a host
could not make usable sound until the chipset ran near realtime; that is true
only if Rigel renders the audio. With per-channel state the host's own mixer
plays at the host's rate, so audio stops being gated on chipset speed -- which
makes this one of the few pieces of a host integration that can be finished
before the loop in ISSUE-0006 is fixed.
