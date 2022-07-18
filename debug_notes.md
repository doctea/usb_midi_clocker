# Debug notes

Kinda got this working in conjunction with TeensyDebug and Platformio in VSCode to allow breakpoint debugging with GDB etc..

.. I saw 'kind of working' because it worked up to a point, but was a bit flakey, sometimes breakpoints didn't break, and basically did not allow me to figure out anything that I didn't already know, as when the code crashed it would seem to start the debugger too late?  IDK, YMMV

## Enabling Dual Serial connections with MIDI 

Note that this is **UNTESTED**, but its something like this!

Add this to /mnt/c/Users/_YOUR_USERNAME_HERE_/.platformio/packages/framework-arduinoteensy/cores/teensy4/usb_desc.h around line 587:

~~~
#elif defined(USB_MIDI_16_DUAL_SERIAL)

  #define VENDOR_ID             0x16C0
  #define PRODUCT_ID            0x0489
  #define BCD_DEVICE            0x0414
  #define MANUFACTURER_NAME     {'T','e','e','n','s','y','d','u','i','n','o'}
  #define MANUFACTURER_NAME_LEN 11
  #define PRODUCT_NAME          {'T','e','e','n','s','y',' ','M','I','D','I','x','1','6',' ','D','u','a','l',' ','S','e','r','i','a','l'}
  #define PRODUCT_NAME_LEN      26
  #define EP0_SIZE              64
  #define NUM_ENDPOINTS         6
  #define NUM_INTERFACE         4
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
  #define ENDPOINT6_CONFIG      ENDPOINT_RECEIVE_BULK + ENDPOINT_TRANSMIT_BULK
~~~