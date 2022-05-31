

void setup_pc_usb();
void read_usb_from_computer();

void pc_usb_1_setOutputWrapper(MIDIOutputWrapper *wrapper);
void pc_usb_2_setOutputWrapper(MIDIOutputWrapper *wrapper);

extern MIDIOutputWrapper *pc_usb_1_output;
extern MIDIOutputWrapper *pc_usb_2_output;
