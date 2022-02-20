/*
BEATSTEP:   Initialised device vendorid: 7285   productid: 518
BAMBLEWEENY:  Initialised device vendorid: 10374    productid: 32779
AKAI APCMINI: Initialised device vendorid: 2536   productid: 40
*/

MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *midi_beatstep;
MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *midi_apcmini;
MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *midi_bamble;

bool beatstep_started = false, apcmini_started = false, bamble_started = false;

void handleNoteOn(byte inChannel, byte inNumber, byte inVelocity);
void handleNoteOff(byte inChannel, byte inNumber, byte inVelocity);

void beatstep_control_change (byte inChannel, byte inNumber, byte inValue) {
  if (inNumber==54 && inValue==127) {
    Serial.println("BEATSTEP START PRESSED!");
    beatstep_started = false;
  } else {
    Serial.print("Beatstep");
    //handleNoteOn(inChannel, inNumber, inValue);
  }
}

void apcmini_control_change (byte inChannel, byte inNumber, byte inValue) {
  Serial.print("APCMINI CC ch");
  Serial.print(inChannel);
  Serial.print("\tnum ");
  Serial.print(inNumber);
  Serial.print("\tvalue: ");
  Serial.println(inValue);

  if (inNumber==51) {
    bpm_current = map(inValue, 0, 127, 60, 140);
    Serial.print("set bpm to ");
    Serial.println(bpm_current);
    ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
  }  
}

void handle_beatstep_start() {
  midi_beatstep->sendStart();
  beatstep_started = true;
}

void disconnect_callbacks(MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> &interface) {
  interface.disconnectCallbackFromType((uint8_t)0xB0);
}

void onInit(int midi_number, USBH_MIDI *device, const MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *interface) {
  Serial.print("onInit: Initialised device vid:"); //, midi_number);
  Serial.print(device->idVendor());
  Serial.print("\tpid:");
  Serial.print(device->idProduct());
  Serial.println();

  if (device->idVendor()==7285 && device->idProduct()==518) {
    Serial.print("FOUND BEATSTEP AS midi_number ");
    beatstep_started = false;
    midi_beatstep = interface;
    if (midi_apcmini  == interface)     midi_apcmini  = NULL;
    if (midi_bamble   == interface)     midi_bamble   = NULL;

    midi_beatstep->setHandleControlChange(beatstep_control_change);
    midi_beatstep->setHandleStart(handle_beatstep_start);    
  } else if (device->idVendor()==10374 && device->idProduct()==32779) {
    bamble_started = false;
    Serial.print("FOUND BAMBLE AS midi_number ");
    midi_bamble = interface;
    if (midi_apcmini  == interface)    midi_apcmini = NULL;
    if (midi_beatstep == interface)   midi_beatstep = NULL;
  } else if (device->idVendor()==2536 && device->idProduct()==40) {
    apcmini_started = false;
    Serial.print("FOUND APCMINI AS midi_number ");
    midi_apcmini = interface;
    midi_apcmini->setHandleControlChange(apcmini_control_change);
    if (midi_beatstep == interface)   {
      disconnect_callbacks(*midi_beatstep);
      midi_beatstep = NULL;
    }
    if (midi_bamble   == interface)   {
      disconnect_callbacks(*midi_bamble);
      midi_bamble = NULL;
    }
  } else {
    Serial.print("FOUND unidentified device!");
    if (midi_beatstep == interface)   midi_beatstep = NULL;
    if (midi_bamble   == interface)   midi_bamble   = NULL;
    if (midi_apcmini  == interface)   midi_apcmini  = NULL;
  }
  Serial.print(midi_number);
  Serial.println("!");

  //MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *target;
  
  /*switch(midi_number) {
    case 1: target = Midi1; break;
    case 2: target = Midi2; break;
    case 3: target = Midi3; break;
  }*/
  //device->sendStart();
  /*Midi1.sendStart();
  Midi2.sendStart();
  Midi3.sendStart();*/
  //target->sendStart();

  //MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *test = device; 
}

void onInit1(USBH_MIDI *device) {
  onInit(1, device, &Midi1);
}
void onInit2(USBH_MIDI *device) {
  onInit(2, device, &Midi2);
}
void onInit3(USBH_MIDI *device) {
  onInit(3, device, &Midi3);
}

void start_clocks_if_stopped() {
  if (!beatstep_started && midi_beatstep) {
    Serial.println("BEATSTEP not started -- starting!");
    midi_beatstep->sendStart();
    beatstep_started = true;
  }
  if (!bamble_started && midi_bamble) {
    midi_bamble->sendStart();
    bamble_started = true;
  }
  if (!apcmini_started && midi_apcmini) {
    beat_counter = 8;
    apcmini_started = true;
  }
}
