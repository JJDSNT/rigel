# Rigel API Convergence Plan

## Refining the Existing Rigel Host Boundary for Bellatrix Integration

**Status:** Proposed API Boundary Refinement Baseline  
**Target:** Rigel API Version 1  
**Related specification:** `Bellatrix/docs/Rigel_integration.md`

---

# 1. Purpose

This document defines the recommended refinement of the existing Rigel public API so that the boundary between Rigel and its hosts is explicit, minimal, host-neutral, and suitable for long-term use by Bellatrix and the standalone harness.

The authority chain remains:

~~~text
Bellatrix.md
        │
        ▼
Rigel_integration.md
        │
        ▼
Rigel API Convergence Plan
        │
        ▼
Rigel public API
        │
        ▼
librigel implementation
~~~

The current Rigel implementation already contains the major architectural properties required by the Bellatrix/Rigel design.

The objective is therefore **not to redesign Rigel**.

The objective is:

> Preserve the existing Rigel execution and hardware boundary. Refine only those public interfaces where the current API either exposes implementation details, mixes host and hardware concerns, or requires the host to understand Rigel-owned semantics.

The primary areas requiring attention are:

* public API organization;
* separation of hardware configuration from host services;
* opaque host context semantics;
* explicit guest-physical memory semantics;
* canonical M68K-visible MMIO dispatch;
* MMIO width, alignment, ordering, side-effect, and result semantics;
* removal of direct host access to Rigel internals;
* explicit lifecycle and reset semantics;
* explicit execution/concurrency contract without constraining host topology;
* preservation of Rigel's authoritative chipset timeline;
* preservation of Rigel IPL ownership;
* preservation of DMA and Chip RAM ownership rules;
* formalization of video output as host-consumable classic chipset output;
* separation of integration APIs from diagnostic and advanced APIs;
* preservation of existing observable behavior during API refinement.

The existing chipset implementation, timing model, interrupt model, DMA model, bus model, video renderer, and harness should be preserved wherever they already satisfy the architectural contract.

---

# 2. Classification Model

Every proposed API change should first be classified into one of four categories:

~~~text
PRESERVE

    The current implementation and host boundary are already
    architecturally correct.

FORMALIZE

    The current behavior is already correct, but its public
    contract is implicit or insufficiently documented.

CHANGE

    The current public API requires host/chipset responsibility
    leakage, mixes unrelated concerns, or exposes an unsuitable
    host-facing abstraction.

INTERNALIZE

    The functionality may remain useful internally, for tests,
    inspection, or tooling, but should not remain part of the
    normal host integration contract.
~~~

This distinction is fundamental.

The existence of a topic in this document does not imply that the current Rigel implementation is deficient in that area.

---

# 3. Guiding Principles

The convergence work should follow these rules:

> Preserve working Rigel semantics.

> Refine the API boundary rather than rewriting the chipset.

> Do not treat already-correct host/Rigel ownership as a migration task.

> A host-side boundary violation does not automatically justify expanding the Rigel API.

> The Rigel API defines operation semantics and ordering requirements. The host decides where and how those operations execute.

> Rigel must support host freedom without becoming aware of host execution topology.

> Bellatrix must not interpret classic hardware semantics that belong to Rigel.

> Rigel must not acquire Bellatrix, Emu68, Raspberry Pi, AROS, VC4, scheduler, or core-specific knowledge.

---

# 4. Existing Rigel Architecture

The current Rigel implementation already contains the major classic hardware domains expected from the compatibility component.

Conceptually:

~~~text
Rigel
│
├── public API
│
├── chipset composition
│
│   ├── Agnus
│   ├── Denise
│   ├── Paula
│   ├── CIA
│   └── related devices
│
├── hardware domains
│
│   ├── beam
│   ├── DMA
│   ├── Copper
│   ├── Blitter
│   ├── interrupts
│   ├── audio
│   ├── disk
│   ├── serial
│   └── input
│
├── deterministic timing
├── bus observation
├── classic video generation
└── harness / tests
~~~

This organization should remain.

Hardware domains represent ownership of chipset state and behavior.

They must not be confused with host execution threads, ARM cores, queues, schedulers, or Bellatrix runtime topology.

---

# 5. What Should Be Preserved

The following are architectural properties that should be treated primarily as **PRESERVE**.

## 5.1 Host-independent chipset implementation

Rigel should remain independent from:

* Bellatrix;
* Raspberry Pi hardware;
* Emu68 internals;
* VC4;
* BCM interrupt controllers;
* USB;
* Bluetooth;
* AROS;
* host scheduler topology;
* host core numbering;
* host synchronization primitives.

No Bellatrix-specific dependency should enter the Rigel core.

---

## 5.2 Deterministic execution model

Given identical defined:

* configuration;
* guest memory;
* MMIO accesses;
* input;
* execution progress;

Rigel should produce identical defined hardware state transitions and outputs.

Host wall-clock timing remains outside chipset correctness.

---

## 5.3 Existing temporal model

Rigel already owns and advances the authoritative classic chipset timeline.

Conceptually:

~~~text
Rigel
  │
  ▼
next deadline
  │
  ▼
Host executes CPU according to host policy
  │
  ▼
execution progress
  │
  ▼
Rigel step/advance
  │
  ▼
new deadline
~~~

This temporal ownership should be preserved.

The current stepping/deadline model should not be replaced merely to match a new naming convention.

---

## 5.4 Interrupt ownership

Rigel should continue to own:

~~~text
INTREQ
INTENA
classic interrupt sources
classic priority resolution
Rigel IPL
~~~

Bellatrix consumes the resulting IPL.

Bellatrix must not reconstruct Rigel IPL from internal interrupt registers.

---

## 5.5 DMA ownership

Rigel should continue to own:

* Agnus DMA semantics;
* Paula DMA semantics;
* Copper and Blitter memory behavior;
* chipset-generated address interpretation;
* classic Chip RAM visibility rules.

Bellatrix provides memory services but must not reproduce classic chipset DMA rules.

---

## 5.6 Host-provided memory

Rigel should continue to operate on memory supplied by the host.

Rigel must not become the allocator or owner of Bellatrix guest physical memory.

The existing callback-based memory model remains valid.

---

## 5.7 Host-controlled execution topology

Execution placement belongs entirely to the host.

Bellatrix may execute Rigel:

~~~text
on the same host core as Emu68

on another ARM core

on another host thread

through a queue

through a rendezvous mechanism

through another serialized execution context
~~~

without changing Rigel semantics.

Rigel must not encode or depend on any specific execution topology.

---

## 5.8 Standalone harness

The standalone harness remains a first-class consumer of the same production `librigel` implementation used by Bellatrix.

The relationship remains:

~~~text
Bellatrix ──┐
            │
            ├──► public Rigel API ──► librigel
            │
Harness ────┘
~~~

---

## 5.9 Existing classic video generation

Rigel should continue to own classic Amiga video semantics, including the existing Denise raster pipeline and planar-to-chunky conversion.

The current video renderer should not be rewritten merely because Bellatrix presentation changes.

---

# 6. Behavioral Preservation

Before modifying the public boundary, establish a behavioral baseline.

Representative scenarios should capture:

* MMIO accesses;
* MMIO ordering and side effects;
* timing;
* deadlines;
* IPL transitions;
* DMA reads and writes;
* reset behavior;
* video frame output;
* audio output;
* bus behavior where relevant.

Conceptually:

~~~text
Current Rigel
     │
     ▼
Capture behavioral baseline
     │
     ├── MMIO traces
     ├── timing/deadline traces
     ├── IPL traces
     ├── DMA effects
     ├── frame hashes/state
     ├── audio state
     └── selected bus state
     │
     ▼
Refine API
     │
     ▼
Replay equivalent scenarios
     │
     ▼
Compare behavior
~~~

Differences must be classified as either:

~~~text
intentional semantic correction
~~~

or:

~~~text
regression caused by API refinement
~~~

---

# 7. Primary Refactoring Target: The Public Boundary

The primary transformation should occur around Rigel rather than inside its hardware implementation.

The public surface should be divided conceptually into:

~~~text
Rigel API
   │
   ├── Host Integration API
   │
   │      lifecycle
   │      MMIO
   │      progress / stepping
   │      deadlines
   │      IPL
   │      guest memory
   │      video output
   │      audio output
   │      input
   │
   └── Advanced / Inspection API
          bus inspection
          beam inspection
          snapshots
          diagnostics
          testing controls
          internal-state observation
~~~

Bellatrix should depend only on the Host Integration API unless a concrete architectural requirement justifies otherwise.

---

# 8. Opaque Rigel Instance

The primary Rigel object should be opaque to hosts.

Conceptually:

~~~c
struct rigel;
~~~

Bellatrix must not directly access:

~~~text
rigel->chipset.*
rigel->agnus.*
rigel->denise.*
rigel->paula.*
rigel->cia.*
~~~

or equivalent internal structures.

Internal state includes:

* Agnus;
* Denise;
* Paula;
* CIA;
* Copper;
* Blitter;
* beam state;
* DMA scheduler;
* interrupt state;
* floppy state;
* internal queues and caches.

If Bellatrix currently accesses such structures:

~~~text
host actually needs the information?
        │
       / \
     no   yes
     │     │
 remove   existing public API?
            │
           / \
         yes  no
         │     │
       use it  define a narrow API
~~~

Debug-only access should use inspection APIs rather than the production host contract.

---

# 9. Separate Hardware Configuration from Host Services

This is a real API cleanup.

The public API should separate:

~~~text
what classic hardware Rigel models
~~~

from:

~~~text
what services the host provides
~~~

Conceptually:

~~~c
struct rigel_config {
    enum rigel_chipset chipset;
    enum rigel_video_standard video_standard;
    uint32_t chip_ram_size;
    ...
};

struct rigel_host_ops {
    uint8_t  (*mem_read8)(
        void *ctx,
        uint32_t guest_physical_address);

    uint16_t (*mem_read16)(
        void *ctx,
        uint32_t guest_physical_address);

    uint32_t (*mem_read32)(
        void *ctx,
        uint32_t guest_physical_address);

    void (*mem_write8)(
        void *ctx,
        uint32_t guest_physical_address,
        uint8_t value);

    void (*mem_write16)(
        void *ctx,
        uint32_t guest_physical_address,
        uint16_t value);

    void (*mem_write32)(
        void *ctx,
        uint32_t guest_physical_address,
        uint32_t value);

    void (*log)(
        void *ctx,
        int level,
        const char *message);
};
~~~

Creation could conceptually become:

~~~c
struct rigel *
rigel_create(
    const struct rigel_config *config,
    const struct rigel_host_ops *host_ops,
    void *host_context);
~~~

The exact signatures are not normative.

The architectural distinction is:

~~~text
rigel_config
      │
      └── modeled hardware

rigel_host_ops
      │
      └── services supplied by host

host_context
      │
      └── opaque association with this host instance
~~~

---

# 10. Host Context Is a First-Class Boundary Concept

The host context should be explicitly defined as:

> An opaque value supplied by the host and returned unchanged to host operations. Rigel must never interpret its contents.

Conceptually:

~~~text
Rigel instance
      │
      ├── chipset configuration
      ├── chipset state
      ├── host operations
      └── opaque host context
~~~

For Bellatrix, the host context may represent:

~~~text
BellatrixMachine *
~~~

or another integration object.

For the harness, it may represent:

~~~text
HarnessContext *
~~~

For another host it may represent something entirely different.

Rigel must not know whether the context contains:

* guest memory state;
* Emu68 state;
* queues;
* locks;
* video state;
* JIT metadata;
* cross-core mailboxes;
* native-device state.

This property is what allows the same API to support different host architectures without changing Rigel.

---

# 11. Chip RAM Configuration and Ownership

Fields such as:

~~~c
uint32_t chip_ram_size;
~~~

describe chipset-visible memory topology.

They do not imply that Rigel allocates memory.

The ownership remains:

~~~text
Bellatrix
    │
    ├── allocates guest memory
    ├── maps guest memory
    └── provides backing
            │
            ▼
      guest physical memory
            │
            ▼
          Rigel
            │
            └── applies classic
                Chip RAM semantics
~~~

Rigel owns:

* chipset-visible address rules;
* address masking;
* pointer interpretation;
* DMA accessibility;
* classic Chip RAM behavior.

The host owns:

* allocation;
* mapping;
* backing;
* host pointers;
* MMU policy;
* host-side coherency.

---

# 12. Memory Address Semantics

Memory callback addresses must have one explicit meaning:

> Addresses passed to host memory callbacks are guest physical addresses.

Therefore:

~~~c
mem_read16(
    host_context,
    guest_physical_address);
~~~

must not ambiguously mean:

* Chip RAM array offset;
* raw chipset-generated address;
* M68K MMIO address;
* host pointer.

The translation is:

~~~text
chipset register / DMA engine
        │
        ▼
chipset-generated address
        │
        ▼
Rigel classic address rules
        │
        ▼
guest physical address
        │
        ▼
host memory operation
        │
        ▼
Bellatrix memory backend
~~~

---

# 13. Host Memory Is Also the Coherency Boundary

Host memory callbacks are also the correct boundary for host-specific side effects required by writes.

For example:

~~~text
Rigel DMA write
      │
      ▼
host mem_write
      │
      ▼
Bellatrix
      │
      ├── update guest RAM
      └── perform host-specific coherency work
          if required
~~~

This may include Emu68 executable-code invalidation or another host-specific operation.

Rigel must not know:

* whether the host uses JIT translation;
* where translated code is stored;
* whether memory is executable;
* how cache or translation invalidation works.

---

# 14. Memory Failure Semantics

Before API Version 1, determine whether host memory operations are:

~~~text
infallible by contract
~~~

or:

~~~text
capable of explicit host integration failure
~~~

This must remain distinct from classic hardware-visible invalid access behavior.

The contract must document:

* whether callbacks can fail;
* which component validates addresses;
* behavior outside configured Chip RAM;
* open-bus or ignored-write behavior where historically applicable;
* distinction between classic hardware behavior and host implementation failure.

No unusual but valid hardware behavior should accidentally become a host API error.

---

# 15. Preserve Callback-Based Memory

The callback memory model should remain the baseline integration path.

~~~text
Rigel
  │
  ▼
host memory API
  │
  ▼
Bellatrix guest memory
~~~

Advantages include:

* explicit ownership;
* instrumentation;
* harness compatibility;
* JIT-aware host handling;
* host independence;
* easy testing.

Direct memory windows may be added later as an optimization.

---

# 16. Direct Memory Windows as Optional Optimization

A future optimization may expose validated direct memory windows.

Conceptually:

~~~c
struct rigel_memory_window {
    uint32_t guest_base;
    size_t size;
    void *host_ptr;
};
~~~

This must remain an optimization of the same memory contract.

It must not cause Rigel to become aware of:

* Bellatrix allocation policy;
* ARM page tables;
* Emu68 translation metadata;
* Raspberry Pi physical layout.

---

# 17. Canonical MMIO Boundary

This is one of the primary real API changes.

The current host-facing model should evolve from subsystem-specific entry points that require the host to decode Amiga hardware semantics toward a canonical M68K-visible transaction boundary.

The target is:

~~~text
Bellatrix / execution engine
            │
            ▼
M68K-visible MMIO transaction
            │
            ├── address
            ├── width
            ├── direction
            └── value
            │
            ▼
           Rigel
~~~

For example:

~~~text
WRITE

M68K address = 0x00DFF096
width        = 16 bits
value        = 0x8200
~~~

Bellatrix should not need to transform this into:

~~~text
custom register
offset 0x096
~~~

before entering Rigel.

That decode belongs inside the Rigel compatibility domain.

---

# 18. Provider Selection Remains Outside Rigel

Canonical MMIO does not make Rigel a global M68K address dispatcher.

The host still decides which provider owns a region.

~~~text
M68K address
     │
     ▼
Bellatrix / Emu68 dispatcher
     │
     ├── native provider
     │
     ├── Rigel provider
     │
     └── unmapped
~~~

Bellatrix may know:

~~~text
this address belongs to a Rigel-owned compatibility region
~~~

but should not need to know:

~~~text
this address is DMACON

this address is a CIA register

this register has this chipset meaning
~~~

The responsibility boundary is:

~~~text
Bellatrix
    │
    │ select provider
    ▼
Rigel
    │
    │ interpret classic hardware meaning
    ▼
chipset component
~~~

---

# 19. Preserve Internal Region-Specific MMIO

The existing internal APIs may remain useful.

Conceptually:

~~~text
canonical Rigel MMIO
        │
        ▼
internal MMIO router
        │
        ├── custom
        ├── CIAA
        └── CIAB
~~~

The change is the external host boundary, not necessarily the internal implementation.

---

# 20. M68K-Logical MMIO Values

The canonical MMIO boundary must use M68K-visible logical values.

For example:

~~~text
address = 0x00DFF096
width   = 16
value   = 0x8200
~~~

means:

~~~text
M68K-visible value = 0x8200
~~~

regardless of ARM host endianness.

Host-native byte representation must not leak into Rigel register semantics.

---

# 21. MMIO Width and Alignment

Before freezing the canonical MMIO API, define:

* supported widths;
* alignment requirements;
* misaligned behavior;
* unusual historical accesses;
* unsupported-width behavior;
* wider-access decomposition;
* ordering of decomposed accesses;
* side effects;
* timing visibility.

The following concepts remain distinct:

~~~text
CPU transaction width
        │
        ▼
classic bus semantics
        │
        ▼
register implementation width
~~~

A 32-bit access must not automatically be assumed equivalent to two 16-bit accesses unless the Rigel contract defines it that way.

---

# 22. MMIO Transactions Are Observable Hardware Operations

MMIO must be treated as hardware transactions, not ordinary memory.

The host or execution engine must not freely:

* cache;
* eliminate;
* duplicate;
* combine;
* split;
* reorder;

Rigel MMIO accesses unless the contract explicitly permits the transformation.

For example:

~~~text
read A
read A
~~~

must remain two transactions if classic semantics make them observable.

Similarly:

~~~text
write A
write B
~~~

must preserve ordering.

This is especially important for translated execution engines such as Emu68.

> MMIO optimization must preserve Rigel-defined observable transaction semantics.

---

# 23. MMIO Result and Failure Semantics

The canonical MMIO contract must define how to represent:

* successful transactions;
* unsupported widths;
* misaligned accesses;
* address holes within a Rigel-owned compatibility domain;
* classic hardware-visible exceptional behavior;
* genuine host integration failures.

The key distinction is:

~~~text
guest-visible classic behavior
            │
            │ distinct from
            ▼
host integration failure
~~~

A valid classic hardware result must not be reported as an API failure simply because it is unusual from a host perspective.

---

# 24. Address Namespace Separation

The public contract must distinguish:

~~~text
M68K MMIO address
        │
        ▼
CPU-visible hardware address


chipset-generated address
        │
        ▼
address generated by DMA logic


guest physical address
        │
        ▼
address supplied to host memory API


host pointer
        │
        ▼
host representation
~~~

Recommended terminology should preserve this distinction explicitly.

---

# 25. Preserve the Existing Temporal Model

The existing timing model should be treated primarily as **PRESERVE / FORMALIZE**, not as a redesign target.

Conceptually:

~~~text
Host execution progress
        │
        ▼
Rigel step/advance
        │
        ▼
authoritative chipset timeline
        │
        ├── beam
        ├── DMA
        ├── Copper
        ├── Blitter
        ├── Paula
        └── CIA
~~~

The host may maintain CPU execution accounting, budgets, or scheduling state.

Those values are not a second chipset clock.

---

# 26. Temporal API Formalization

Before API Version 1, document:

* canonical progress unit;
* current-time representation;
* deadline representation;
* overflow behavior;
* overshoot behavior;
* event ordering.

The existing stepping API should be preserved if it already expresses the required semantics cleanly.

A rename from:

~~~text
step
~~~

to:

~~~text
advance
~~~

or:

~~~text
advance_to
~~~

is not itself an architectural objective.

The first question must be:

> Does the current temporal API correctly express the existing temporal contract?

If yes, preserve it.

---

# 27. Overshoot

Rigel should continue to tolerate deterministic deadline overshoot.

Conceptually:

~~~text
deadline = 100
CPU reaches = 112

Rigel:
    process event at 100
    continue through remaining 12
~~~

Overshoot tolerance remains a correctness property, not a scheduling strategy.

---

# 28. Preserve Returned Host-Visible Results

The existing model in which stepping returns host-visible state changes should be preserved.

Conceptually:

~~~text
Host
  │
  ▼
Rigel step
  │
  ▼
step result
  │
  ├── IRQ changed
  ├── frame ready
  ├── output available
  └── other defined host-visible results
~~~

This avoids arbitrary reentrancy and keeps the host in control.

Internal events should remain internal unless they cross the host boundary for a defined reason.

---

# 29. Avoid Generic Event Escape Callbacks

Do not introduce a generic:

~~~c
signal_event(ctx, event);
~~~

merely for convenience.

Prefer:

~~~text
Rigel operation
      │
      ▼
explicit return result
      │
      ▼
host reacts
~~~

or narrowly defined output mechanisms.

---

# 30. Preserve Rigel IPL Boundary

The normal Bellatrix interrupt integration should continue to consume only Rigel's resolved compatibility-domain IPL.

Conceptually:

~~~c
unsigned
rigel_get_ipl(
    const struct rigel *rigel);
~~~

Bellatrix performs:

~~~text
native_ipl ─────┐
                │
                ▼
          IPL arbitration
                │
                ▼
              M68K
                ▲
                │
rigel_ipl ──────┘
~~~

This is primarily a **PRESERVE** item.

---

# 31. INTREQ and INTENA Are Inspection State

Detailed interrupt state may remain available for:

* debugging;
* harness validation;
* diagnostics;
* state inspection.

Conceptually:

~~~text
Host integration:
    rigel_get_ipl()

Inspection:
    rigel_get_intreq()
    rigel_get_intena()
~~~

Bellatrix must not reconstruct IPL from those values.

---

# 32. Bus Observation

Rigel's existing bus-observation capabilities should remain available as advanced functionality.

Possible capabilities include:

~~~text
get bus state

get next bus change

determine whether CPU can access Chip RAM

determine resume time
~~~

These may later support more accurate CPU/chipset contention.

---

# 33. Bus Contention Is an Optional Host Integration Level

Fine-grained bus integration should not be mandatory for the initial Bellatrix adapter unless required for correctness.

Conceptually:

~~~text
basic integration

MMIO
step/progress
deadline
IPL
DMA
memory coherence

        │
        ▼

advanced integration

Chip RAM contention
bus ownership
CPU stalls
resume timing
~~~

Rigel remains authoritative for classic bus-slot semantics.

Bellatrix must not duplicate Agnus scheduling logic.

---

# 34. Internal Wiring Should Not Be Host API

Any function whose only purpose is wiring internal Rigel components should be reviewed for internalization.

The preferred lifecycle is:

~~~text
rigel_create()
      │
      ├── allocate internal state
      ├── configure hardware
      ├── wire components
      └── return valid opaque instance
~~~

The host should not need to construct internal relationships.

---

# 35. Lifecycle and Reset

Lifecycle semantics should be **FORMALIZED** before API Version 1.

At minimum:

~~~text
create

cold reset

warm reset

destroy
~~~

must have documented semantics.

Determine:

* state immediately after create;
* whether create implies cold reset;
* whether an explicit reset is required before first use;
* cold-reset semantics;
* warm-reset semantics;
* timeline behavior across reset;
* interrupt behavior;
* DMA behavior;
* output state;
* preservation of configuration;
* preservation of host operations;
* preservation of host context;
* interaction with guest memory.

Bellatrix selects a reset class.

Rigel owns the classic hardware behavior of that reset class.

Bellatrix must not reproduce chipset-specific reset rules.

---

# 36. Advanced Development Controls

Controls such as cycle-exact toggles may remain useful for:

* diagnostics;
* testing;
* A/B validation;
* development.

They should not automatically become core host integration requirements.

---

# 37. Snapshots

Snapshot functionality should remain outside the minimal host contract.

Snapshots may remain part of:

~~~text
advanced
tooling
inspection
test
~~~

APIs.

---

# 38. Video Output Boundary

Video requires an explicit host boundary, but the existing Rigel renderer should be preserved.

Rigel already owns:

* bitplane interpretation;
* palette behavior;
* dual playfield;
* HAM;
* EHB;
* sprite composition;
* raster effects;
* planar-to-chunky conversion;
* classic Denise output semantics.

The target boundary is:

~~~text
Chip RAM
   │
   ▼
Agnus / Denise
   │
   ▼
classic video generation
   │
   ▼
chunky host-consumable output
   │
   ▼
Bellatrix video adapter
   │
   ▼
native graphics / presentation
~~~

Rigel must not become an RTG device.

Rigel must not require P96.

Rigel must not know VC4, AROS graphics internals, HDMI, or physical framebuffer details.

---

# 39. Canonical Video Representation

The existing chunky frame output should become the primary host-facing video abstraction.

The canonical general-purpose format should remain conceptually:

~~~text
RGBA8888
~~~

with existing optimized output formats such as:

~~~text
RGB565
~~~

remaining available where useful.

A frame should carry host-independent metadata such as:

~~~text
pixels
width
height
pitch
pixel format
frame counter
full-redraw state
dirty-line information
~~~

The host consumes the frame.

Rigel does not decide where or how it is presented.

---

# 40. Video Is Not Guest Memory

Classic video output is derived hardware output, not guest physical memory.

The relationship is:

~~~text
Chip RAM
   │
   │ DMA
   ▼
Agnus / Denise
   │
   ▼
pixels
   │
   ▼
host presentation
~~~

Therefore the video-output interface must remain distinct from the guest-memory API.

---

# 41. Host Owns Presentation

Rigel owns the meaning and production of classic video pixels.

The host owns:

* storage used for presentation;
* composition;
* scaling;
* aspect correction;
* synchronization with physical display;
* GPU upload;
* framebuffer presentation;
* fullscreen/window decisions;
* native graphics integration.

Conceptually:

~~~text
Rigel
  │
  ▼
RGBA frame
  │
  ▼
Bellatrix
  │
  ├── native surface
  ├── GPU texture
  ├── framebuffer
  ├── compositor
  └── headless test buffer
~~~

---

# 42. Preserve Pull-Based Video as the Baseline

The simplest and cleanest initial API should preserve a pull-style model.

Conceptually:

~~~text
Rigel step
   │
   ▼
FRAME_READY
   │
   ▼
rigel_get_frame()
   │
   ▼
host presents frame
~~~

This allows Rigel to keep its existing internal renderer and avoids tying chipset generation to any particular host presentation resource.

---

# 43. Host-Provided Video Targets Are Optional Optimization

If the existing implementation supports host-provided video buffers or zero-copy paths, those may remain as optional performance facilities.

They should not live inside classic hardware configuration.

Conceptually:

~~~text
rigel_config
    = modeled hardware

video target
    = optional host output resource
~~~

A possible future relationship is:

~~~text
Rigel
  │
  ├── internal frame + get_frame()
  │
  └── optional validated host video target
~~~

Both represent the same classic video semantics.

---

# 44. Scanline Output

Existing scanline-oriented video inspection/output should be preserved.

It may serve:

* diagnostics;
* raster validation;
* low-latency presentation;
* streaming;
* advanced GPU integration;
* harness tests.

It need not be mandatory for normal Bellatrix presentation.

---

# 45. Native AROS Graphics and Rigel Video Are Separate Domains

Bellatrix may have a native graphics path independent from Rigel.

Conceptually:

~~~text
                 Bellatrix graphics
                        │
          ┌─────────────┴─────────────┐
          │                           │
   native AROS graphics        Rigel classic video
          │                           │
          └─────────────┬─────────────┘
                        │
                  host presentation
~~~

Rigel classic output is a source of video content.

It is not the native graphics architecture of Bellatrix.

---

# 46. Audio Output

The same ownership principle applies to Paula audio.

~~~text
Paula
  │
  ▼
Rigel
  │
  ▼
host-independent audio output
  │
  ▼
Bellatrix audio adapter
  │
  ▼
native audio presentation
~~~

Rigel must remain unaware of:

* HDMI audio;
* PWM;
* USB audio;
* Raspberry Pi-specific output.

---

# 47. Input

Input flows in the opposite direction.

~~~text
native input
    │
    ▼
Bellatrix
    │
    ▼
classic input adaptation
    │
    ▼
Rigel
    │
    ▼
classic hardware state
~~~

Rigel should receive classic hardware-facing input information.

It should not receive USB or Bluetooth transport objects.

---

# 48. Execution and Concurrency Contract

Rigel defines operation semantics.

The host defines execution topology.

The Version 1 contract should explicitly distinguish these responsibilities.

Rigel may require:

~~~text
ordered calls

non-concurrent entry into one instance

non-reentrant callbacks

serialized state transitions
~~~

Those requirements do not imply host thread or core affinity.

---

# 49. Reentrancy

Unless explicitly documented otherwise, callbacks into host services must not re-enter the same Rigel instance.

Conceptually:

~~~text
rigel_step()
      │
      ▼
host mem_read()
      │
      ╳
      └── must not synchronously call
          back into the same Rigel instance
~~~

This keeps execution deterministic and avoids unnecessary synchronization inside Rigel.

---

# 50. Host-Topology Neutrality

The API must not be described as single-thread-affine or single-core-affine unless a concrete implementation requirement proves such a restriction necessary.

The correct rule is:

> A Rigel instance is non-concurrent unless otherwise documented. The host is responsible for serialization.

The host may serialize calls while executing them:

~~~text
same core

different core

worker thread

queue consumer

remote execution context
~~~

Rigel remains unaware of:

~~~text
Core 0
Core 1
Core 2
Core 3

MPIDR

WFE
SEV

Bellatrix queues

Bellatrix scheduler

host locks
~~~

---

# 51. Cross-Core Execution Is a Host Policy, Not a Later Rigel Feature

Cross-core Rigel execution must not be classified as a future Rigel capability.

If the API is host-topology neutral, Bellatrix may already choose:

~~~text
Emu68 on one core
Rigel on another core
~~~

provided the host preserves:

* ordering;
* serialization;
* timing semantics;
* MMIO transaction identity;
* memory coherency;
* result publication.

Any queueing or rendezvous mechanism belongs to Bellatrix.

---

# 52. API Version Versus Binary ABI Stability

Rigel API Version 1 should define a stable public source-level contract.

Binary ABI stability should not be implied unless separately specified.

Recommended initial rule:

> Rigel API Version 1 defines a stable public source-level interface. Binary ABI stability is not guaranteed unless separately documented.

---

# 53. Recommended Public API Organization

A possible organization is:

~~~text
include/rigel/
│
├── rigel.h
├── rigel_types.h
├── rigel_config.h
├── rigel_host.h
├── rigel_memory.h
├── rigel_mmio.h
├── rigel_time.h
├── rigel_irq.h
├── rigel_video.h
├── rigel_audio.h
├── rigel_input.h
└── advanced/
    ├── rigel_bus.h
    ├── rigel_snapshot.h
    ├── rigel_debug.h
    └── rigel_inspect.h
~~~

The exact filesystem organization is not normative.

The important property is separation between normal host integration and advanced/internal functionality.

---

# 54. `rigel.h` Should Remain Deliberately Small

The umbrella header used by normal hosts should expose only what is required to host a Rigel instance.

Conceptually:

~~~c
struct rigel *
rigel_create(...);

void
rigel_destroy(...);

void
rigel_reset(...);

unsigned
rigel_get_ipl(...);
~~~

and access to the other stable host-facing capabilities:

~~~text
MMIO
memory contract
step/progress
deadlines
video output
audio output
input
~~~

without exposing chipset internals.

---

# 55. Bellatrix Adapter Responsibilities

The Bellatrix Rigel adapter should remain small.

~~~text
Bellatrix
   │
   └── Rigel adapter
          │
          ├── create/configure Rigel
          ├── register Rigel-owned address regions
          ├── forward canonical MMIO
          ├── provide guest-memory services
          ├── report execution progress
          ├── observe deadlines
          ├── obtain Rigel IPL
          ├── participate in host IPL arbitration
          ├── consume classic video output
          ├── consume classic audio output
          └── adapt native input
~~~

It must not contain:

~~~text
Copper semantics
Blitter semantics
Denise semantics
Paula semantics
CIA semantics
beam calculations
INTREQ semantics
INTENA semantics
Agnus DMA address rules
Amiga register decoding
~~~

---

# 56. Bellatrix Host Policy Remains Outside the Adapter Contract

Bellatrix may independently decide:

~~~text
where Rigel executes

how Rigel calls are transported

whether MMIO is synchronous

how cross-core rendezvous works

how native interrupts are delivered

how video frames are composed

how guest memory is mapped

how JIT coherency is implemented
~~~

These are host implementation decisions.

The Rigel API must support them without encoding them.

---

# 57. Recommended Migration Strategy

The migration should be incremental.

~~~text
Existing Rigel
      │
      ▼
behavioral baseline
      │
      ▼
API inventory
      │
      ▼
classify:
PRESERVE / FORMALIZE / CHANGE / INTERNALIZE
      │
      ▼
host boundary cleanup
      │
      ▼
memory contract formalization
      │
      ▼
canonical MMIO boundary
      │
      ▼
video boundary cleanup
      │
      ▼
encapsulation cleanup
      │
      ▼
candidate API
      │
      ├── harness
      └── Bellatrix
      │
      ▼
integration validation
      │
      ▼
API Version 1
~~~

---

# 58. Phase 0 — Behavioral Baseline

Capture representative deterministic behavior before API modifications.

Include:

* MMIO traces;
* timing;
* deadlines;
* IPL transitions;
* DMA behavior;
* reset;
* video frame state/hashes;
* audio state;
* bus behavior where applicable.

---

# 59. Phase 1 — API Inventory

Classify every exported symbol as:

~~~text
CORE HOST

ADVANCED HOST

INSPECTION

TEST

INTERNAL

DEPRECATED
~~~

Additionally classify each architectural area as:

~~~text
PRESERVE

FORMALIZE

CHANGE

INTERNALIZE
~~~

No public symbol should remain public merely because it historically was.

---

# 60. Phase 2 — Host Boundary Cleanup

Separate:

~~~text
rigel_config
    = modeled hardware

rigel_host_ops
    = host services

host_context
    = opaque host association
~~~

Remove host presentation resources from hardware configuration.

Remove direct host access to Rigel internal structures.

---

# 61. Phase 3 — Memory Contract

Preserve the callback-based model.

Formalize:

~~~text
chipset-generated address
        ↓
Rigel address interpretation
        ↓
guest physical address
        ↓
host memory API
~~~

Define failure semantics and host-side coherency responsibility.

---

# 62. Phase 4 — Canonical MMIO

Introduce the M68K-visible transaction boundary.

~~~text
Bellatrix / Harness
        │
        ▼
canonical MMIO
        │
        ▼
Rigel router
        │
        ├── custom
        ├── CIAA
        └── CIAB
~~~

Bellatrix selects the provider.

Rigel owns Amiga register interpretation.

Define:

* widths;
* alignment;
* decomposition;
* ordering;
* side effects;
* endianness;
* result semantics;
* failure semantics.

---

# 63. Phase 5 — Video Boundary

Preserve the existing Rigel renderer.

Formalize host-facing output around existing chunky video.

Baseline:

~~~text
FRAME_READY
     │
     ▼
rigel_get_frame()
     │
     ▼
host consumes RGBA8888/RGB565 output
~~~

Separate optional host-provided output targets from `rigel_config`.

Preserve scanline and dirty-region facilities where useful.

---

# 64. Phase 6 — Encapsulation Cleanup

Remove Bellatrix access to Rigel internals.

For every existing direct access:

~~~text
needed?
  │
 / \
no yes
│   │
remove
    │
    ├── existing API -> use it
    │
    └── missing API -> add narrow host or inspection API
~~~

---

# 65. Phase 7 — Contract Formalization

Formalize, without unnecessarily redesigning:

* current timing model;
* progress units;
* deadline semantics;
* overshoot;
* lifecycle;
* reset;
* IPL;
* concurrency;
* reentrancy;
* host-topology neutrality.

---

# 66. Phase 8 — Harness Migration

Move the standalone harness to the candidate host integration API.

The harness should validate:

~~~text
create

memory backend

canonical MMIO

step/deadlines

IPL

DMA

video frame output

reset

deterministic replay
~~~

---

# 67. Phase 9 — Bellatrix Migration

Bellatrix should consume the same candidate API.

Initial Bellatrix integration should cover:

~~~text
Lifecycle

MMIO

Memory

Progress

Deadline

IPL

Video output

DMA coherency
~~~

Cross-core placement remains a Bellatrix implementation choice and does not require a separate Rigel API phase.

---

# 68. Phase 10 — Integration Validation

Exercise both real consumers:

~~~text
              Candidate Rigel API
                     │
             ┌───────┴───────┐
             │               │
          Harness         Bellatrix
             │               │
             └───────┬───────┘
                     │
              integration tests
                     │
                     ▼
                API review
~~~

Resolve missing abstractions before freezing Version 1.

---

# 69. Phase 11 — API Version 1 Freeze

Review whether:

* Bellatrix still needs Rigel internals;
* the harness requires Bellatrix-specific behavior;
* MMIO requires host-side Amiga decoding;
* memory namespaces are explicit;
* host context remains opaque;
* video is independent from physical presentation;
* execution topology remains host-controlled;
* timing semantics remain deterministic;
* lifecycle/reset semantics are explicit;
* IPL remains Rigel-owned;
* advanced facilities remain separate.

Only then should:

~~~c
#define RIGEL_API_VERSION 1
~~~

represent a stable source-level contract.

---

# 70. Advanced Bus Integration

Fine-grained Chip RAM contention may be integrated later if required.

~~~text
CPU Chip RAM access
        │
        ▼
Rigel bus state
       / \
      /   \
available  busy
   │        │
access    resume later
~~~

This should use Rigel's existing bus model rather than duplicating Agnus logic in Bellatrix.

---

# 71. API Compatibility During Migration

Temporary compatibility wrappers may be used:

~~~text
old API
   │
   ▼
compatibility wrapper
   │
   ▼
candidate API
~~~

They should be removed before Version 1 is declared stable.

---

# 72. What Should Not Be Rewritten

Do not rewrite without concrete evidence of a semantic problem:

* Copper;
* Blitter;
* Denise;
* Paula;
* CIA;
* beam state;
* interrupt priority;
* DMA scheduler;
* deterministic event scheduler;
* bus ownership model;
* planar-to-chunky video renderer;
* frame generation;
* harness infrastructure;
* timing model.

For every change ask:

> Is this implementation actually violating the new boundary, or is the boundary already correct?

---

# 73. What Must Actually Change

## Required API work

1. Inventory and classify exported symbols.
2. Separate hardware configuration from host services.
3. Make `host_context` explicitly opaque.
4. Remove host presentation resources from classic hardware configuration.
5. Preserve callback memory while defining guest-physical semantics.
6. Define host memory failure semantics.
7. Define host-side coherency responsibility for DMA writes.
8. Introduce canonical M68K-visible MMIO.
9. Keep provider selection outside Rigel.
10. Move classic Amiga register decode behind the Rigel MMIO boundary.
11. Define MMIO width/alignment/ordering/side-effect/result semantics.
12. Remove Bellatrix direct access to Rigel internals.
13. Formalize classic video output as host-consumable chunky output.
14. Preserve existing frame and scanline mechanisms.
15. Separate optional zero-copy/video-target mechanisms from `rigel_config`.
16. Formalize lifecycle/reset semantics.
17. Formalize concurrency and reentrancy rules without imposing host core/thread affinity.
18. Separate advanced/debug/inspection APIs.
19. Establish explicit API Version 1 source-level compatibility rules.

## Primarily preserve/formalize

20. Existing deterministic execution model.
21. Existing stepping/deadline model.
22. Rigel authoritative chipset timeline.
23. Rigel IPL ownership.
24. INTREQ/INTENA ownership.
25. DMA ownership.
26. Callback-based host memory ownership model.
27. Existing bus model.
28. Existing Denise renderer.
29. Existing harness.
30. Host-controlled multicore execution topology.

## Optional later optimization

31. Validated direct memory windows.
32. Fine-grained CPU/Chip RAM contention.
33. Zero-copy video targets.
34. Zero-copy audio.
35. Specialized asynchronous notifications where proven necessary.

Cross-core Rigel execution is **not** listed as a future Rigel optimization.

It remains a host execution policy that the API must already permit.

---

# 74. What Bellatrix Must Not Force Rigel to Become

Bellatrix integration must not cause Rigel to become:

~~~text
a Raspberry Pi library

an Emu68 extension

a VC4 driver

an RTG implementation

a P96 device

an AROS component

a Bellatrix scheduler component

a multicore runtime

a BCM interrupt adapter
~~~

Rigel remains:

> A host-independent implementation of classic Amiga hardware semantics.

---

# 75. Conformance Tests

## Behavioral preservation

Equivalent scenarios before and after API refinement produce equivalent defined hardware behavior unless differences are explicitly intentional.

## Independent build

`librigel` builds without:

~~~text
Bellatrix headers
Emu68 internal headers
Raspberry Pi headers
AROS headers
~~~

## Independent execution

The standalone harness creates and operates Rigel without Bellatrix.

## Host-context opacity

Rigel stores and returns host context without interpreting its contents.

## Memory ownership

Rigel performs chipset DMA through host-provided guest-memory operations without allocating Bellatrix guest physical memory.

## JIT independence

Host-specific JIT or executable-memory coherency can occur behind host memory operations without exposing Emu68 internals to Rigel.

## Canonical MMIO

A CPU-visible M68K MMIO transaction reaches the correct Rigel hardware implementation without Bellatrix decoding classic register semantics.

## Provider isolation

Bellatrix selects the provider.

Rigel interprets the compatibility-region address.

## Address-space separation

M68K MMIO, chipset-generated addresses, guest physical addresses, and host pointers remain distinct.

## Timing ownership

Rigel remains the sole authoritative classic chipset timeline.

## Host-topology neutrality

The same Rigel API works when the host executes the instance:

~~~text
same-core
cross-core
worker-thread
queued
serialized remote context
~~~

without Rigel knowing which topology is used.

## Interrupt ownership

Bellatrix consumes Rigel IPL without reconstructing it from INTREQ/INTENA.

## Video independence

Rigel produces classic chunky video output without knowing:

~~~text
RTG
P96
AROS gfx.hidd
VC4
HDMI
physical framebuffer
~~~

## Native graphics independence

Bellatrix native graphics can coexist with Rigel classic video without making Rigel part of the native graphics stack.

## Adapter isolation

Removing the Bellatrix Rigel adapter removes classic Amiga compatibility without structural changes to Bellatrix Core.

---

# 76. Review Checklist

Every Rigel API patch should answer:

1. Is this functionality already correct and merely undocumented?
2. Is this change PRESERVE, FORMALIZE, CHANGE, or INTERNALIZE?
3. Is this operation genuinely required by a host?
4. Is chipset implementation detail leaking into Bellatrix?
5. Is Bellatrix interpreting classic hardware that belongs to Rigel?
6. Is Rigel being given host-specific knowledge?
7. Is hardware configuration mixed with host services?
8. Is host presentation state mixed with classic hardware configuration?
9. Is `host_context` opaque?
10. Is an M68K MMIO address confused with a guest physical address?
11. Is a chipset-generated DMA address confused with a guest physical address?
12. Is a guest physical address confused with a host pointer?
13. Is Bellatrix reproducing chipset address-generation rules?
14. Is Bellatrix decoding Amiga registers that Rigel should decode?
15. Does Rigel remain provider-local rather than a global M68K dispatcher?
16. Are MMIO widths explicit?
17. Are alignment semantics explicit?
18. Are ordering and side effects explicit?
19. Can a translated execution engine accidentally optimize away observable MMIO?
20. Are MMIO guest-visible results distinct from host failures?
21. Are memory failures distinct from classic hardware behavior?
22. Does host memory remain the coherency boundary?
23. Is Rigel still authoritative for chipset time?
24. Is wall-clock time entering classic correctness?
25. Are reset semantics owned by Rigel?
26. Is Bellatrix reconstructing IPL?
27. Is CPU interrupt acceptance confused with interrupt-source acknowledgement?
28. Is an internal event exposed without a host-visible reason?
29. Is a generic callback being introduced unnecessarily?
30. Can host callbacks re-enter the same instance?
31. Does the API describe concurrency requirements rather than host placement?
32. Could Bellatrix move Rigel to another core without changing the API?
33. Is any ARM core number encoded in Rigel?
34. Is any Bellatrix queue or scheduler concept encoded in Rigel?
35. Does Rigel video output remain independent of RTG/P96?
36. Does Rigel video output remain independent of VC4 or native display implementation?
37. Is a framebuffer target incorrectly being treated as classic hardware configuration?
38. Can the harness consume the same video output?
39. Is an advanced/debug facility becoming core API accidentally?
40. Does Bellatrix access Rigel internals directly?
41. Can another non-Bellatrix host implement the interface cleanly?
42. Does the change preserve the behavioral baseline?
43. Have both harness and Bellatrix exercised the candidate API?
44. Is source API stability being distinguished from binary ABI stability?

If these questions cannot be answered cleanly, the API change should be reconsidered.

---

# 77. Target Architecture

~~~text
                           Bellatrix
                               │
                      Rigel Host Adapter
                               │
       ┌──────────────┬────────┼──────────┬──────────────┐
       │              │        │          │              │
      MMIO          Memory   Progress     IPL           Video
       │              │        │          │              │
       └──────────────┴────────┼──────────┴──────────────┘
                               │
                        public Rigel API
                               │
                               ▼
                           librigel
                               │
          ┌────────────────────┼────────────────────┐
          │                    │                    │
        Agnus                Denise               Paula
          │                    │                    │
     Copper/Blitter      raster/pixels          CIA/audio
          │                    │
         DMA                RGBA/RGB565
          │                    │
          ▼                    ▼
 chipset-generated         video output
      address                  │
          │                    ▼
          ▼             host presentation
  Rigel address rules
          │
          ▼
 guest physical address
          │
          ▼
 host memory operations
          │
          ▼
 Bellatrix guest memory
~~~

Provider selection remains outside Rigel:

~~~text
M68K address
     │
     ▼
Bellatrix / Emu68
address dispatcher
     │
     ├── native provider
     │
     ├── Rigel provider
     │       │
     │       ▼
     │   canonical Rigel MMIO
     │
     └── unmapped
~~~

Host execution topology also remains outside Rigel:

~~~text
                      Bellatrix host policy
                              │
             ┌────────────────┼────────────────┐
             │                │                │
          same-core       cross-core       worker/thread
             │                │                │
             └────────────────┼────────────────┘
                              │
                   serialized Rigel operations
                              │
                              ▼
                           librigel
~~~

---

# 78. Desired Bellatrix Dependency

The final Bellatrix integration should remain understandable from a small amount of code.

Conceptually:

~~~c
rigel = rigel_create(
    &config,
    &host_ops,
    host_context);

rigel_reset(
    rigel,
    RIGEL_RESET_COLD);
~~~

MMIO:

~~~text
M68K-visible transaction
        │
        ▼
Bellatrix provider selection
        │
        ▼
Rigel MMIO
        │
        ▼
classic hardware semantics
~~~

Memory:

~~~text
Rigel DMA
   │
   ▼
guest physical address
   │
   ▼
host_ops.mem_read/write
   │
   ▼
Bellatrix memory/coherency
~~~

Timing:

~~~text
Rigel deadline
     │
     ▼
host executes M68K
     │
     ▼
host reports progress
     │
     ▼
Rigel steps
~~~

Interrupt:

~~~c
rigel_ipl = rigel_get_ipl(rigel);

effective_ipl =
    bellatrix_arbitrate_ipl(
        native_ipl,
        rigel_ipl);
~~~

Video:

~~~text
Rigel
  │
  ▼
FRAME_READY
  │
  ▼
host obtains chunky frame
  │
  ▼
Bellatrix native presentation
~~~

The host may execute any of these operations through any serialized host topology.

---

# 79. Recommended Implementation Sequence

~~~text
0. Capture behavioral baseline
        │
1. Inventory exported API
        │
2. Classify:
   PRESERVE / FORMALIZE / CHANGE / INTERNALIZE
        │
3. Separate config / host_ops / host_context
        │
4. Remove host presentation state from hardware config
        │
5. Formalize guest-physical memory contract
        │
6. Formalize host memory failure/coherency behavior
        │
7. Introduce canonical M68K MMIO
        │
8. Move Amiga register decode behind Rigel boundary
        │
9. Define MMIO semantics
        │
10. Preserve existing internal MMIO helpers
        │
11. Formalize video frame output
        │
12. Preserve Denise renderer and scanline facilities
        │
13. Separate optional video target optimization
        │
14. Remove Bellatrix direct internal access
        │
15. Formalize timing without redesigning it unnecessarily
        │
16. Formalize lifecycle/reset
        │
17. Formalize IPL as preserved boundary
        │
18. Formalize non-concurrency/reentrancy
        │
19. Explicitly guarantee host-topology neutrality
        │
20. Separate advanced/debug APIs
        │
21. Move harness to candidate API
        │
22. Replay behavioral baseline
        │
23. Move Bellatrix to candidate API
        │
24. Validate same-core and host-selected cross-core use
        │
25. Validate MMIO/memory/timing/video/IRQ
        │
26. Review candidate API
        │
27. Freeze RIGEL_API_VERSION 1
        │
28. Optimize only after correctness
~~~

---

# 80. Definition of Done for Rigel API Version 1

Rigel API Version 1 should not be frozen until:

* the behavioral baseline exists;
* `librigel` builds independently;
* harness and Bellatrix use the same production API;
* Rigel state is opaque;
* `rigel_config` describes modeled hardware only;
* host services are separate;
* `host_context` is opaque and host-owned;
* guest-memory callback semantics are explicit;
* host memory failure semantics are explicit;
* host-side coherency responsibility is explicit;
* Chip RAM configuration does not imply allocation ownership;
* M68K MMIO reaches Rigel without Bellatrix decoding classic registers;
* provider selection remains outside Rigel;
* MMIO width/alignment/ordering/side-effect semantics are explicit;
* guest-visible MMIO behavior remains distinct from host failure;
* address namespaces are explicit;
* the existing temporal model remains authoritative;
* progress and deadline semantics are documented;
* overshoot remains deterministic;
* reset semantics are explicit;
* Rigel IPL remains sufficient for classic interrupt delivery;
* INTREQ/INTENA remain Rigel-owned;
* host execution topology is not encoded by Rigel;
* concurrent-entry rules are explicit;
* host callbacks have explicit reentrancy rules;
* cross-core Bellatrix execution requires no Rigel API change;
* existing Denise rendering remains intact;
* chunky video output is a stable host-facing abstraction;
* Rigel video output does not depend on RTG/P96;
* Rigel video output does not depend on VC4 or AROS native graphics;
* host presentation resources are not classic hardware configuration;
* advanced/debug APIs are separated;
* the behavioral baseline is replayed after convergence;
* any behavior changes are explicitly intentional;
* Version 1 states whether source or binary compatibility is guaranteed.

For the initial release:

> Stable public source-level API. Binary ABI stability is not implied unless separately documented.

---

# 81. Relationship to the Integration Specification

The authority hierarchy remains:

~~~text
Bellatrix.md
      │
      ▼
architecture
      │
      ▼
Rigel_integration.md
      │
      ▼
cross-boundary behavioral contract
      │
      ▼
Rigel API Convergence Plan
      │
      ▼
API refinement of existing implementation
      │
      ▼
public Rigel headers
      │
      ▼
librigel
~~~

This document does not redefine `Rigel_integration.md`.

Its purpose is to identify which parts of the current Rigel boundary should be:

~~~text
preserved

formalized

changed

internalized
~~~

in order to establish API Version 1.

---

# 82. Final Recommendation

The existing Rigel implementation should remain the foundation of `librigel`.

Its working:

* chipset implementation;
* deterministic execution model;
* temporal model;
* interrupt model;
* DMA behavior;
* bus infrastructure;
* Denise renderer;
* chunky video output;
* harness;

should remain intact wherever they already satisfy the required contract.

The primary work is not a chipset rewrite.

It is a host-boundary refinement concentrated in four areas:

~~~text
                  Rigel API v1 refinement

        ┌────────────────────────────────────┐
        │                                    │
        │  1. HOST BOUNDARY                  │
        │     config                         │
        │     host_ops                       │
        │     opaque host_context            │
        │                                    │
        │  2. MEMORY                         │
        │     guest physical semantics       │
        │     host coherency boundary        │
        │                                    │
        │  3. MMIO                           │
        │     canonical M68K transaction     │
        │     Rigel-owned Amiga decode       │
        │                                    │
        │  4. VIDEO                          │
        │     preserve Denise renderer       │
        │     chunky host output             │
        │     host-owned presentation        │
        │                                    │
        └────────────────────────────────────┘
~~~

The following should primarily be preserved:

~~~text
timing

deadlines

step model

DMA ownership

IPL ownership

determinism

bus model

harness

host-controlled multicore topology
~~~

The final boundary should satisfy this rule:

> Bellatrix may change its memory backend, execution topology, synchronization strategy, native graphics path, video presentation mechanism, or native-device implementation without requiring changes to Rigel, provided it continues to satisfy the Rigel host contract.

And conversely:

> Rigel may evolve its internal implementation of classic Amiga hardware without requiring Bellatrix to understand those implementation details.

The intended final relationship is:

~~~text
                      Host policy
                         │
        same-core / cross-core / thread / queue
                         │
                         ▼
                      Bellatrix
                         │
                   Rigel Adapter
                         │
       ┌─────────────────┼─────────────────┐
       │                 │                 │
      MMIO             Memory            Video
       │                 │                 │
       ├──────── Progress / IPL ───────────┤
       │                 │                 │
       └─────────────────┼─────────────────┘
                         │
                  public Rigel API
                         │
                         ▼
                      librigel
                         │
                classic Amiga hardware
~~~

Once this boundary has been validated by both Bellatrix and the standalone harness, it can be frozen as Rigel API Version 1.
