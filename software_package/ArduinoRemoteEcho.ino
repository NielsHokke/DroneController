int inByte = 0;         // incoming serial byte

void setup() {
  Serial.begin(115200);
  while (!Serial);
}

void loop() {
  if (Serial.available() > 0){
    inByte = Serial.read();
    Serial.write(inByte);
  }
}

