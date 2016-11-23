# midifoot

## Software Setup

1. Install <a href="https://www.arduino.cc/en/Main/Software">Arduino IDE</a>
1. Get <a href="http://playground.arduino.cc/Main/MIDILibrary">MIDILibrary</a> from <a href="https://github.com/FortySevenEffects/arduino_midi_library/releases/latest">here</a>
1. From the Arduino IDE menu: Sketch -> Include Library -> Add .ZIP Library, and select the downloaded ZIP file
1. Install the USB serial drivers for your board (in my case i needed <a href="http://www.arduined.eu/files/windows8/CH341SER.zip">CHG340 drivers for Windows 10</a>
1. Select the COM port in Arduino IDE menu: Tools -> Port -> COM?
1. Download and open midifoot.ino in Arduino IDE
1. Compile/Upload to Arduino board with Ctrl+U or menu: Sketch -> Upload (make sure the TX pin is disconnected)

## Hardware Setup

1. MIDI jack pin 5 -> 220 Ohm resistor -> Arduino digital pin 1 (Serial TX)
1. MIDI jack pin 2 -> Arduino pin GND
1. MIDI jack pin 4 -> 220 Ohm resistor -> Arduino pin +5V

Note:

1. MIDI pin 2 is the center bottom
1. MIDI pin 5 is left of pin 2
1. MIDI pin 4 is right of pin 2
1. Use arduino.cc <a href="https://www.arduino.cc/en/Tutorial/Midi">MIDI Tutorial</a>

<img src="https://www.arduino.cc/en/uploads/Tutorial/MIDI_schem.png">
