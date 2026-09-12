#ifndef MATRIX_MAPPER_TYPES__INCLUDED
#define MATRIX_MAPPER_TYPES__INCLUDED

typedef int source_id_t;
typedef int target_id_t;

typedef int8_t serial_midi_number_t;

struct routed_note_output_t {
	int8_t pitch;
	uint8_t channel;
	bool emitted;

	routed_note_output_t(int8_t pitch = -1, uint8_t channel = 0, bool emitted = false)
		: pitch(pitch), channel(channel), emitted(emitted) {}
};

class MIDIMatrixManager;
class DeviceBehaviourUltimateBase;

extern MIDIMatrixManager *midi_matrix_manager;

// Bracket a synchronous change to a behaviour-local pitch transform. Active
// matrix notes whose resolved output changes are migrated to the new output.
void begin_midi_matrix_target_transform_change(DeviceBehaviourUltimateBase *behaviour);
void end_midi_matrix_target_transform_change(DeviceBehaviourUltimateBase *behaviour);

#endif      