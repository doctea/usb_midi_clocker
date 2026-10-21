# Debug notes

Kinda got this working in conjunction with TeensyDebug and Platformio in VSCode to allow breakpoint debugging with GDB etc..

.. I saw 'kind of working' because it worked up to a point, but was a bit flakey, sometimes breakpoints didn't break, and basically did not allow me to figure out anything that I didn't already know, as when the code crashed it would seem to start the debugger too late?  IDK, YMMV

## Open observations (2026-10-19)

- **Intermittent stuck notes on Neutron:** reported with Prog.Bass and a
  TuringMachine output both routed to Neutron. BeatStep -> Neutron did not show
  the symptom despite higher message frequency. It is not currently reproducible.
  Cause and relationship to recent routing changes are unknown; interaction with
  seqlib's independent quantisation is a hypothesis, not a diagnosis. The latest
  supplied endpoint listing shows `Prog.Bass` as source 2, `TuringMachine Ch1` as
  source 24, and `S3 : Neutron : ch 1` as target 2; earlier discussion mentioned
  TuringMachine 2, so verify the actual connected role rather than assuming one.
  Follow the [note capture procedure](docs/midi-note-routing.md#capturing-an-intermittent-stuck-note)
  when it recurs, capturing all sources to the target before cleanup.
- **Blank matrix target rows:** Bass Proxy, Arp Chords, and Arp Trigger occupy
  rows but appear black/empty rather than showing their labels. Selecting them
  still shows their names in messages. Found a 30-colour palette declared with
  50 entries, leaving targets 30-49 black. Fixed the palette size so target
  colours cycle through populated entries; host tests cover all 50 target IDs.
  Hardware confirmation pending.
- **Playback-only UI pauses:** brief, regularly recurring UI freezes while
  playback is active; not reported while stopped. Frequency, clock/tempo
  correlation, affected controls, and whether musical output also stalls remain
  unmeasured. Found and removed four unconditional `Serial_flush()` calls in
  `do_tick()` outside compiled-out debug logging; these could block the clock
  interrupt on serial traffic even with debug logging off. Host tests check
  that normal playback ticks still dispatch clocks/behaviours without flushing.
  This removes a concrete blocking hazard, but whether it fully explains the
  observed freezes requires hardware confirmation. No connection to the new
  note diagnostics has been established.

## RAM1 Stack Reserve

The stack guard remains at 8 KB minimum, with a warning below 16 KB. The
[custom linker script](linker/imxrt1062_t41_alt.ld) keeps read-only C++
construction vtables (`.rodata._ZTC*`) and VTTs (`.rodata._ZTT*`) in flash
instead of copying them into RAM1. These support virtual-base construction and
destruction; normal runtime vtables remain in RAM1. This requires no PSRAM.
Rebuild to measure the resulting reserve; an existing ELF retains its old layout.

## USB Host Diagnostics

The Debug > USB Debug page shows free host pools, pending MIDI output, transmit
drops, and queue-storage failures. With detailed tracing enabled it also shows
the latest detected host error and previous phase. Dump status prints the
available details to Serial; the console command is `usb_debug`. Clear error
clears the retained error, not the controller's halted state, MIDI transmit-drop
counters, previous phase, or SD log. The Behaviours/USB page is unchanged.

Detailed tracing is controlled by `USBHOST_T36_ENABLE_DIAGNOSTICS` (default
`0`). Define it as `1` in the consuming firmware build to enable retained error
and foreground-phase records, controller/queue-head/hub snapshots, detailed
MIDI counters, and SD reset snapshots. Apply the define to the whole build so
the application and USBHost_t36 compile with the same setting. It is independent
of `USBHOST_PRINT_DEBUG` and `USBHOST_T36_QTD_CERR`. With tracing off, `usb_debug`
still reports the controller fault latch, pool health, MIDI queue/drop counts,
and always-on MIDI transfer errors; detailed record APIs are inert and phase
markers, retained records, snapshots, and reset-log writes compile out. Pool
corruption/double-free checks and controller/teardown safety behavior remain on.

Pipe teardown fails closed if schedule removal or controller-stop waits time
out: it latches a host error, disables controller interrupts/schedules, and
retains potentially DMA-visible descriptors. This exceptional state requires a
reboot; it deliberately does not return uncertain memory to the pools.

The `CERR=3` retry setting remains separate and unchanged. Host regressions
cover both diagnostics settings and `CERR=0/3`, but do not verify hardware DMA
quiescence or prove the physical USB fault is universally resolved. After the
next firmware upload, repeatedly reset the previously problematic peripheral
and exercise hub unplug/replug while checking MIDI/APC output and the keyboard.
Collect `usb_debug` output before replugging if a failure recurs; a fault latch
means the controller intentionally stopped and should be rebooted.

When detailed tracing is enabled, one fixed-size record captures pool
exhaustion, failed control queuing, fatal enumeration transfers/descriptors,
halted driver endpoint transfers, and EHCI system errors. Optional
enumeration-string descriptor stalls are not counted.
`Endpoint transfer halted` includes bulk/interrupt failures and driver-owned
control requests, including hub controls that use the shared enumeration
callback. Detail is the raw failing qTD token for transfer errors, received
byte count for invalid descriptors, or USBSTS for system errors. A rejected
hub port-control submission records `Control queue failed`, with setup word1
in detail. Address/hub/port are useful before VID/PID have been discovered.
Zero VID/PID alone does not identify a fault: pool/controller errors may have
no device context. Use the error count and reason; when no event exists the
serial dump now says so instead of printing a default zero device identity.

`countFree()` detects unaligned free-list links and walks that exceed the number
of contributed nodes (including cycles). With detailed tracing enabled it also
records the fault. An affected count is `UINT32_MAX`, displayed as CORRUPT
rather than usable capacity. The error names the affected pool; detail is the
suspect link address. IRQ state is preserved, and pool push/pop operations use
short IRQ-preserving critical sections.
This catches the observed pipe link `0x1` without dereferencing it, but does
not repair a damaged list or validate every aligned pointer. The original
corrupting writer still needs to be identified if the problem recurs.

Transfer returns now make a bounded, IRQ-protected scan before changing the
free list. A descriptor already on the list is not inserted twice; when detailed
tracing is enabled, the retained error is `Transfer double free`, with the
caller's return PC in `detail` rather than a descriptor address. Resolve it
against the ELF from that exact build.
The scan also rejects an already unaligned or overlong list without changing it.
It adds no allocation or ownership registry, does not repair existing corruption,
and does not establish that a duplicate return caused earlier failures.

The captured duplicate-return PC `0x600637CB` resolved to the final qTD walk in
`delete_Pipe()` in the matching firmware. A regression reproduced a related
queueing defect: a real error-halted qTD was mistaken for the dummy tail because
both have the Halted bit set. Each pipe now tracks its actual dummy in a formerly
unused word, without changing the ARM pipe size. Queueing uses that pointer and
assigns every qTD its pipe owner; error recovery resets to the actual dummy.
After removing a pipe from the hardware schedule, teardown frees its software
followup entries and the dummy, not potentially stale hardware qTD links.
Host checks cover callback requeueing and stale-head teardown while preserving
another pipe. Physical peripheral reset/unplug/reconnect recovery still needs
hardware testing; these fixes do not guarantee every unresponsive device emits
a hub disconnect event.

A separate regression reproduced a hub port-control submission failure leaving
`sending_control_transfer` latched with no future callback. Port helpers now
retain the pending bit and release the busy flag when queueing fails. The
existing `USBHost::Task()` hook retries pending work, preserving IRQ state and
the original request priority. This does not recover an accepted request that
never completes, or a stopped hub interrupt poll.

The currently reported problematic peripheral is `2e8a:10c1`, not the earlier
`1337:117E` attribution. Its reset can stop MIDI output and downstream hotplug
while the already-connected keyboard keeps working; reconnecting the hub
restores operation. The queue-rejection fix is not yet established as the cause
of this hardware failure. Rebuild/upload, Clear error, reproduce, and collect
the full `usb_debug` output before unplugging the hub. No recorded error still
does not rule out a silent active-transfer stall.

With detailed tracing enabled on Teensy 4, the error record uses uncleared
DMAMEM and is cache-flushed only when an error is captured or cleared. A second
small record tracks the foreground
phase around USB polling, MIDI reads/reconnects, behaviour reads/loops, and
serial I/O, clock/menu tick updates, gates, tap tempo, viewer commands, and
UI/CV/APC work. Other loop work covers the remaining gaps, not a specific call.
Each phase change flushes one RAM2 cache line. It captures the
last entered phase even if a call never returns an error; it is not a stack trace
or proof that the foreground call itself caused the hang (an ISR may have stalled).
The previous-boot phase is copied before loop markers overwrite it, and shown on
USB Debug and in `usb_debug`. There is no heap allocation or per-message trace.

The clock/menu block now distinguishes CV clock handling, clock tick polling,
and menu tick callbacks. Existing phase numbers and the older Clock/menu tick
update label are preserved. The narrower labels still name foreground work,
not an interrupt that may have interrupted it.

These records are intended to survive watchdog/software resets, not power loss.
The phase timestamp is the prior retained foreground uptime. An error timestamp
is the uptime when that error was detected; the error can survive several resets
without another detection, so the two timestamps need not share a boot.
The error count counts detections, including repeated checks of one damaged list,
not necessarily separate failures. Before a fresh reproduction, use Clear error
on Debug > USB Debug; this does not delete the SD log or previous phase.
On normal startup after SD
initialization, an available previous snapshot is appended to `usb_debug.log`.
A detected watchdog reset with no retained phase is logged explicitly too.
The log contains the phase and any retained USB error, not post-reboot pool/queue
values masquerading as pre-reset values. No SD writes are done in loop markers,
USB interrupts, or watchdog callbacks; the eight-second watchdog is unchanged.
Without a usable SD card, the copied previous phase remains available in the UI.

Retention test (requires `USBHOST_T36_ENABLE_DIAGNOSTICS=1`; both commands reboot
and lose unsaved changes):

1. Rebuild/upload, then run `usb_debug_test reset` in the serial console.
2. After reboot, run `usb_debug`; the previous phase should be `Retention test`.
3. Run `usb_debug_test watchdog`; it stops feeding without disabling interrupts
  and should reboot within eight seconds. Check for `Retention test` again.
4. Run `usb_debug_log` to dump the appended SD snapshots; use
  `usb_debug_clearlog` to delete that file without clearing RAM diagnostics.
5. Reproduce the hub freeze, then inspect `usb_debug` / `usb_debug_log` after
  reboot. Record the phase, watchdog status, and any USB error details.

If the deliberate tests lose the marker, retention is not working on that reset
path; SD archiving after boot cannot recover it. Verify retention on actual
hardware before relying on the freeze's saved phase. A cold boot or firmware
upload may invalidate retained data; perform these tests on the same firmware.

## Enabling Dual Serial connections with MIDI 

Add this to /mnt/c/Users/_YOUR_USERNAME_HERE_/.platformio/packages/framework-arduinoteensy/cores/teensy4/usb_desc.h around line 587:

~~~
#elif defined(USB_MIDI16_DUAL_SERIAL)

  #define VENDOR_ID             0x16C0
  #define PRODUCT_ID            0x0489
  #define BCD_DEVICE            0x0413
  #define MANUFACTURER_NAME     {'T','e','e','n','s','y','d','u','i','n','o'}
  #define MANUFACTURER_NAME_LEN 11
  #define PRODUCT_NAME          {'T','e','e','n','s','y',' ','M','I','D','I','x','1','6',' ','D','u','a','l',' ','S','e','r','i','a','l'}
  #define PRODUCT_NAME_LEN      26
  #define EP0_SIZE              64
  #define NUM_ENDPOINTS         7
  #define NUM_INTERFACE         5

  #define CDC_IAD_DESCRIPTOR    1
  #define CDC_STATUS_INTERFACE  0
  #define CDC_DATA_INTERFACE    1       // Serial
  #define CDC_ACM_ENDPOINT      2
  #define CDC_RX_ENDPOINT       3
  #define CDC_TX_ENDPOINT       3
  #define CDC_ACM_SIZE          16
  #define CDC_RX_SIZE_480       512
  #define CDC_TX_SIZE_480       512
  #define CDC_RX_SIZE_12        64
  #define CDC_TX_SIZE_12        64

  #define CDC2_STATUS_INTERFACE 2       // SerialUSB1
  #define CDC2_DATA_INTERFACE   3
  #define CDC2_ACM_ENDPOINT     4
  #define CDC2_RX_ENDPOINT      5
  #define CDC2_TX_ENDPOINT      5

  #define MIDI_INTERFACE        4       // MIDI
  #define MIDI_NUM_CABLES       16
  #define MIDI_TX_ENDPOINT      6
  #define MIDI_TX_SIZE_12       64
  #define MIDI_TX_SIZE_480      512
  #define MIDI_RX_ENDPOINT      6
  #define MIDI_RX_SIZE_12       64
  #define MIDI_RX_SIZE_480      512

  #define ENDPOINT2_CONFIG      ENDPOINT_RECEIVE_UNUSED + ENDPOINT_TRANSMIT_INTERRUPT
  #define ENDPOINT3_CONFIG      ENDPOINT_RECEIVE_BULK + ENDPOINT_TRANSMIT_BULK
  #define ENDPOINT4_CONFIG      ENDPOINT_RECEIVE_UNUSED + ENDPOINT_TRANSMIT_INTERRUPT
  #define ENDPOINT5_CONFIG      ENDPOINT_RECEIVE_BULK + ENDPOINT_TRANSMIT_BULK
  #define ENDPOINT6_CONFIG	ENDPOINT_RECEIVE_BULK + ENDPOINT_TRANSMIT_BULK
~~~

## Troubleshooting

I don't know why it happens, but it has happened to me a couple of times when messing around with this stuff: 

If, when you try to compile, you get into a recursive loop of it trying to satisfy the Adafruit GFX library dependency, then try editing .pio/libdeps/debug/integrity.dat to remove the 'adafruit/Adafruit GFX Library' line.

This seemed to solve that problem for me the last time it happened to me.