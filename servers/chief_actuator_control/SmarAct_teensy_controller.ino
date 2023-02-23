/*
Program to run the Smaract stage closed-loop
Jonah Hansen 2021
*/

int frequency;
int num_steps;
int direction;

String sdata = ""; // Initialised to nothing.

int PIN_CLK = 2;
int PIN_DIR = 3;
int PIN_EN = 4;
int PIN_ONOFF = 5;

void setup() {
  Serial.begin(9600); // USB is always 12 Mbit/sec
  Serial.setTimeout(10); // Timeout of 10ms between read commands
  // Turn ON
  pinMode(PIN_ONOFF, OUTPUT); // ON/OFF pin
  pinMode(PIN_EN, OUTPUT); // EN pin
  pinMode(PIN_DIR, OUTPUT); // DIR pin
  pinMode(PIN_CLK, OUTPUT); // CLK pin

  digitalWrite(PIN_ONOFF, HIGH); // ON/OFF pin high
}

void loop() {
  int messageReceived = 0;
  byte ch;

  while (Serial.available()) {

    ch = Serial.read();

    sdata += (char)ch;

    if (ch == '\r') { // Command recevied and ready.
      messageReceived = 1;
      sdata.trim();

      // Process command in sdata.

      int commaIndex = sdata.indexOf(',');
      //  Search for the next comma just after the first
      int secondCommaIndex = sdata.indexOf(',', commaIndex + 1);

      String firstInput = sdata.substring(0, commaIndex);
      String secondInput = sdata.substring(commaIndex + 1, secondCommaIndex);
      String thirdInput = sdata.substring(secondCommaIndex + 1); // To the end of the string

      num_steps = firstInput.toInt();
      direction = secondInput.toInt();
      frequency = thirdInput.toInt();

      sdata = ""; // Clear the string ready for the next command.
    }
  }

  if (messageReceived == 1) {

    Serial.print("Moving ");
    Serial.print(num_steps*20);
    Serial.println(" Nanometers");
    
    digitalWrite(PIN_EN, HIGH); // EN pin high

    if (direction == 1) {
      digitalWrite(PIN_DIR, HIGH); // DIR pin high
    } else if (direction == 0) {
      digitalWrite(PIN_DIR, LOW); // DIR pin low
    }

    for (int i=0; i<num_steps; i++) {
       digitalWrite(PIN_CLK, HIGH); // CLK pin high
       delayMicroseconds(500000.0/float(frequency)); 
       digitalWrite(PIN_CLK, LOW); // CLK pin high
       delayMicroseconds(500000.0/float(frequency)); 
    }

    digitalWrite(PIN_EN, LOW); // EN pin low

    Serial.println("Finished Movement");

    messageReceived = 0;
  }

}
