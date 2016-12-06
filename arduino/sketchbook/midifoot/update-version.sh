#!/bin/bash
git pull
MIDIFOOT_VERSION="$(grep "#define MIDIFOOT_VERSION" midifoot.ino | cut -d '"' -f2)"
echo "$MIDIFOOT_VERSION" > midifoot-latest.txt
cp /C/Users/ramalhais/AppData/Local/Temp/arduino_build_358381/midifoot.ino.bin midifoot-$MIDIFOOT_VERSION.bin
git add --all
git commit -a -m "Update OTA version."
