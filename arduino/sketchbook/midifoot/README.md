# midifoot

## Software Setup

1. Install <a href="https://www.arduino.cc/en/Main/Software">Arduino IDE</a>
1. Get <a href="http://playground.arduino.cc/Main/MIDILibrary">MIDILibrary</a> from <a href="https://github.com/FortySevenEffects/arduino_midi_library/releases/latest">here</a>
1. From the Arduino IDE menu: Sketch -> Include Library -> Add .ZIP Library, and select the downloaded ZIP file
1. Install the USB serial drivers for your board (in my case i needed <a href="http://www.arduined.eu/files/windows8/CH341SER.zip">CHG340 drivers for Windows 10</a>
1. Select the COM port in Arduino IDE menu: Tools -> Port -> COM?
1. Download and open <a href="https://raw.githubusercontent.com/ramalhais/code/master/arduino/sketchbook/midifoot/midifoot.ino">midifoot.ino</a> in Arduino IDE
1. Compile/Upload to Arduino board with Ctrl+U or menu: Sketch -> Upload (make sure the TX pin is disconnected)

## Hardware Setup

### Arduino

I'm using an <a href="https://www.arduino.cc/en/Main/ArduinoBoardNano">Arduino Nano Board</a>:
<img src="https://www.arduino.cc/en/uploads/Main/ArduinoNanoFront_3_lg.jpg">

Right now i'm powering it with a phone charger, but plan to power it from the MIDI port on my Randall RM100 guitar amplifier.

Any MIDI In port should put out power on pins 1 and 3.

Randall RM100 sends phantom power on ports 6 and 7. It also has some DIP switches inside the amp that enables phantom power on pins 1 and/or 3:
<img src="http://i42.photobucket.com/albums/e311/Soulinsane/RM100F-Board.jpg" />

Check the <a href="http://mtsforum.grailtone.com/viewtopic.php?t=5606">RM100/RM4 Midi Guide</a> at the <a href="http://mtsforum.grailtone.com/">MTS Forum</a>

### Foot switches

I'm using normal guitar amplifier footswitch switches (ON/OFF).
This may allow me to use a normal guitar amplifier 2 switches footswitch, but still haven't tested it.

Inputs for the switches are:
- Switch 1: Arduino Digital Pin 2
- Switch 2: Arduino Digital Pin 3
- Switch 3: Arduino Digital Pin 4
- Switch 4: Arduino Digital Pin 5

We can add more switches by changing the code
from:

    int buttonPin[] = {2, 3, 4, 5};
to:

    int buttonPin[] = {2, 3, 4, 5, 6, 7};

If you want to use momentary switches, you may have to add some debouncing code to ignore multiple detections.

An improvement would be to use the <a href="http://playground.arduino.cc/Main/PinChangeInt">PinChangeInt library</a> to trigger MIDI on interrupts.

### MIDI

1. MIDI jack pin 5 -> 220 Ohm resistor -> Arduino digital pin 1 (Serial TX)
1. MIDI jack pin 2 -> Arduino pin GND
1. MIDI jack pin 4 -> 220 Ohm resistor -> Arduino pin +5V

Note:

1. MIDI pin 2 is the center bottom
1. MIDI pin 5 is left of pin 2
1. MIDI pin 4 is right of pin 2
1. Use arduino.cc <a href="https://www.arduino.cc/en/Tutorial/Midi">MIDI Tutorial</a>

<img src="https://www.arduino.cc/en/uploads/Tutorial/MIDI_schem.png" />

## Hacked up result

It's working!

<img src="https://github.com/ramalhais/code/raw/master/arduino/sketchbook/midifoot/midifoot-hack.jpg" />
