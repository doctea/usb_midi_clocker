# MIDI note routing and quantisation

This document describes note ownership after quantisation became a per-connection policy.

## Connections and pause

The matrix stores up to 128 configured source-target connections in
`MIDIConnectionStore`, with a sorted linked list per source. MIDI events visit
only that source's configured connections, in ascending target order as before.
Disallowed feedback pairs are kept separately and survive a routing reset.
If connection capacity is exhausted, existing connections remain unchanged;
the matrix popup reports the failure and the serial log identifies the pair.

In the matrix's target selection, press **[>]** to open the connection popup.
**Paused: yes/no** suspends or resumes an existing connection. A paused route
remains connected for counts, display, and project saving, but carries no Note
On, Note Off, CC, or pitch-bend events. Pausing immediately releases its recorded
notes, including dropped-note ledger entries. Other sources holding the same
target pitch retain the existing wrapper refcount protection.

Resume does not replay previously held keys: a new Note On is required.
Connection policy changes while paused do not start notes. Channel, quantise,
and pause controls are disabled until a connection exists; disconnected pairs
no longer have policy storage. Disconnecting removes the policy; reconnecting
creates the usual defaults, including quantisation off for drum endpoints.

Each endpoint and connection records its latest event timestamp and whether it
has received an event. Paused routes do not update connection/target activity,
but input activity is still recorded on their source. Held-note counts are
queried from the existing ledger rather than maintained in a second counter.

Project routing is saved as:

```text
midi_matrix~<source>~<target>~connection=version=2;channel=0;quantise=0;note_map=0;paused=1
```

Version 1 connection lines load as unpaused; legacy `targets=a;b;...` lines still
load using normal connection defaults. Malformed known policy fields or
unsupported versions are logged and skipped without replacing an existing
connection. Unknown fields are ignored for extension. Older firmware does not
understand pause, so it cannot preserve paused state when loading these files.

## Two note paths

There are two distinct paths through `DeviceBehaviourUltimateBase`:

1. **Matrix-routed notes** originate at a registered matrix source and travel through a source-target connection. `MIDIMatrixManager` owns their dynamic quantisation and Note On/Off pairing.
2. **Behaviour-local notes** are sent directly by a behaviour, for example by an internal sequencer or chord generator. `DeviceBehaviourUltimateBase` owns their global quantisation and Note On/Off pairing.

These paths intentionally use different trackers. Removing either tracker would leave its path unable to release the pitch that was actually started after harmony changed.

## Matrix-routed Note On

```mermaid
sequenceDiagram
    participant Source as Source behaviour/looper
    participant Matrix as MIDIMatrixManager
    participant Ledger as MIDIConnectionNoteState
    participant Wrapper as MIDIOutputWrapper
    participant Target as Target device/behaviour

    Source->>Matrix: processNoteOn(source, pitch, velocity, channel)
    Matrix->>Matrix: apply_connection_policy_note(source, target, ...)
    Note right of Matrix: fixed channel and per-connection<br/>OFF/SCALE/CHORD/INHERIT quantisation
    Matrix->>Ledger: record input and resolved output
    Matrix->>Wrapper: sendNoteOn(resolved pitch/channel)
    Wrapper->>Target: physical or behaviour-backed send
```

Each occupied ledger entry represents one logical held Note On for one connection. It stores:

- source and target IDs;
- original input pitch/channel;
- resolved connection output pitch/channel;
- original velocity;
- whether the policy emitted or dropped the note.

A dropped note is still recorded because a harmony or policy change can make it valid while its key remains held.

## Matrix-routed Note Off

```mermaid
sequenceDiagram
    participant Source as Source behaviour/looper
    participant Matrix as MIDIMatrixManager
    participant Ledger as MIDIConnectionNoteState
    participant Wrapper as MIDIOutputWrapper
    participant Target as Target device/behaviour

    Source->>Matrix: processNoteOff(source, original pitch/channel)
    Matrix->>Ledger: take matching held entry
    Ledger-->>Matrix: previously resolved pitch/channel
    Matrix->>Wrapper: sendNoteOff(recorded pitch/channel)
    Wrapper->>Target: release note
```

The connection policy is **not recalculated on ordinary Note Off**. Recalculating against current harmony was the source of stuck notes: the release could target a different pitch from the earlier Note On.

## Harmony or connection-policy change

`MIDIMatrixManager::migrate_connection_notes()` recalculates each affected held entry from its original input:

1. Compare the current recorded output with the output under the new harmony/policy.
2. Stop every changed old output.
3. Start every changed new valid output using the original velocity.
4. Update the ledger entry.

Stopping all changed notes before starting replacements keeps `MIDIOutputWrapper` pitch refcounts correct when several inputs collapse onto the same quantised pitch.

## Behaviour-backed targets

`MIDIOutputWrapper_Behaviour` calls `sendRoutedNoteOn()` and `sendRoutedNoteOff()` rather than ordinary behaviour sends.

The scoped routed-note dispatch depth exists so virtual behaviour logic still runs (for example polyphonic voice selection or arpeggiator trigger channels), while the base behaviour knows that connection quantisation and pairing have already been handled by the matrix. It is call-stack state, not held-note state: an RAII guard increments it before each synchronous routed delivery and restores it when that call returns. It does not wait for Note Off. In that context the base class:

- does not apply global harmony quantisation again;
- does not add the note to the behaviour-local `NoteTracker`;
- still applies target-local note limits and `TUNING_OFFSET`;
- sends through `actualSendNoteOn()` / `actualSendNoteOff()`.

The scope is restored after each routed call so later direct behaviour sends use the local path normally. Nested routed calls increase the depth and unwind correctly.

### Target-local transform changes

The matrix ledger records the value sent to the target wrapper. It does not observe the final pitch produced inside a behaviour target, including target note limits, `TUNING_OFFSET`, polyphonic voice allocation, and CV per-channel limits. Routed Note Off normally recomputes those target-local transforms symmetrically.

Behaviour note-limit setters and modulated effective-limit parameters explicitly migrate held notes. Before changing a limit, the manager snapshots each held route's resolved target pitch and channel without emitting anything. Afterwards it compares the new result and only releases or starts outputs whose resolved result changed. Notes valid at the same pitch and channel before and after are untouched. `TRANSPOSE` moves notes whose output changes into range, while `IGNORE` stops notes that are now out of range. The ledger entries remain occupied so the eventual source Note Off is still paired and consumed.

Connection-policy migration and target-transform migration use the same two-pass migration engine: all changed old outputs are released before any replacement outputs start. Connection-policy migration resolves from the original input and commits new connection pitch/channel values to the ledger. Target-transform migration compares a pre-change snapshot with the behaviour's current resolved output, emits directly at that target boundary, and leaves the connection-level ledger mapping unchanged.

Direct behaviour notes use the same comparison rule through the behaviour `NoteTracker`: unchanged mappings remain sounding, changed valid mappings are moved, and newly invalid mappings are stopped. Other mutable target-local transforms must use the same begin/end notification or provide their own migration.

## Behaviour-local notes

```mermaid
sequenceDiagram
    participant Generator as Internal generator
    participant Behaviour as DeviceBehaviourUltimateBase
    participant Tracker as NoteTracker
    participant Device as Actual target/device

    Generator->>Behaviour: sendNoteOn(original pitch)
    Behaviour->>Behaviour: do_quant(global harmony)
    Behaviour->>Behaviour: apply note limits and tuning
    Behaviour->>Tracker: held_note_on(original, transformed)
    Behaviour->>Device: actualSendNoteOn(transformed)

    Generator->>Behaviour: sendNoteOff(original pitch)
    Behaviour->>Tracker: get_transposed_note_for(original)
    Behaviour->>Tracker: held_note_off(original)
    Behaviour->>Device: actualSendNoteOff(recorded transformed pitch)
```

The behaviour `NoteTracker` remains necessary for this direct path. On a harmony change, `DeviceBehaviourUltimateBase::requantise_all_notes()` migrates these locally owned notes. Specialized behaviours such as the progression generator may override that method to regenerate semantic musical state instead.

## Ownership summary

| Concern | Owner |
|---|---|
| Per-connection quantise mode, fixed channel, and pause | `MIDIMatrixManager::connections` (`MIDIConnectionStore`) |
| Routed input to resolved connection pitch/channel | `MIDIConnectionNoteState` |
| Routed output pitch refcounts shared by sources | `MIDIOutputWrapper::playing_notes` |
| Direct behaviour input to globally quantised pitch | `DeviceBehaviourUltimateBase::note_tracker` |
| Target-local limits, tuning, voice allocation | Target behaviour/output implementation |
| Harmony-change notification | `Conductor` |
| Routed harmony migration | `MIDIMatrixManager::migrate_connection_notes()` |
| Direct/specialized harmony migration | `DeviceBehaviourUltimateBase::requantise_all_notes()` and overrides |

## Panic behavior

Normal route cleanup uses recorded ledger outputs. Forced panic deliberately ignores known state: the manager discards relevant ledger entries without emitting their individual releases, then performs one raw Note Off sweep across every MIDI pitch on each relevant channel. This preserves the purpose of `force` without duplicating tracked Note Off events.
