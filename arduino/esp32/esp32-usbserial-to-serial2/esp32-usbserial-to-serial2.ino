void setup() {
  Serial.begin(115200);
  Serial.println("Starting Serial");
}

void loop() {
  if (Serial.available()) {
    int c = Serial.read();
  }
}
