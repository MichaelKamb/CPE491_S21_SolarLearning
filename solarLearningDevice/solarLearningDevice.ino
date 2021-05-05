#include <Servo.h>
#include <WiFiNINA.h>
#include "secrets.h"
#include "ThingSpeak.h"

// Website variables
char ssid[] = SECRET_SSID;   // network SSID 
char pass[] = SECRET_PASS;   // network password
int keyIndex = 0;
WiFiClient  client;
unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;
int delayLoops = 64;

// Solar tracking variables
Servo servo270;  // create servo object to control a servo
Servo servo360;
bool facingSun = true;
int pos270 = 90;    // variable to store the servo position
int pLeft = 0;
int pRight = 0;
int pTop = 0;
int pBottom = 0;
int leftAvg = 0;
int rightAvg = 0;
int topAvg = 0;
int bottomAvg = 0;
int tempLeft = 0;
int tempRight = 0;
int tempTop = 0;
int tempBottom = 0;
int jitter = 10;
int xaccuracy = 5;
int yaccuracy = 5;
float voltage = 0.0;
float R1 = 78000.0; // 78kohm
float R2 = 20000.0; // 20kohm
float reading = 0.0;

void setup() {
  // website
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv != "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }
  ThingSpeak.begin(client);  //Initialize ThingSpeak
  
  // solar tracking
  servo360.attach(9);
  servo270.attach(10);  // attaches the servo on pin 9 to the servo object
  servo270.write(pos270);
  delay(2000); // give time for it to get to atrating position
  Serial.begin(9600);

  // Connect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }
}

void loop() {
  // solar tracking
  reading = 0;
  leftAvg = 0;
  rightAvg = 0;
  topAvg = 0;
  bottomAvg = 0;
  for(int i=0; i<delayLoops; i++) {
    reading += analogRead(A4);
    facingSun = true;
    
    tempLeft = 0;
    tempRight = 0;
    tempTop = 0;
    tempBottom = 0;
    for (int i=0; i<jitter; i++) {
      tempLeft += analogRead(A0);
      tempRight += analogRead(A1);
      tempTop += analogRead(A2);
      tempBottom += analogRead(A3);
    }
    pLeft = tempLeft / jitter;
    pRight = tempRight / jitter;
    pTop = tempTop / jitter;
    pBottom = tempBottom / jitter;

    leftAvg += pLeft;
    rightAvg += pRight;
    topAvg += pTop;
    bottomAvg += pBottom;

    Serial.println(pLeft);
    Serial.println(pRight);
    Serial.println(pTop);
    Serial.println(pBottom);
    Serial.println();
    
    if ((pLeft - pRight) > xaccuracy) {
      servo360.write(0);
      delay(40);
      servo360.write(90);
    }
    else if ((pRight - pLeft) > xaccuracy) {
      servo360.write(180);
      delay(40);
      servo360.write(90);
    }
    
    if ((pTop - pBottom) > yaccuracy) {
      if (pos270 < 180) {
        pos270 -= 1;
      }
      facingSun = false;
      servo270.write(pos270);
    }
    else if ((pBottom - pTop) > yaccuracy) {
      if (pos270 > 0) {
        pos270 += 1;
      }
      facingSun = false;
      servo270.write(pos270);
    }

    
    
    delay(250);
  }

  // voltage sensor reading
  reading = reading / delayLoops;
  voltage = (reading * 5)/1024;
  voltage = voltage / (R2/(R2+R1));

  leftAvg = leftAvg/delayLoops;
  rightAvg = rightAvg/delayLoops;
  topAvg = topAvg/delayLoops;
  bottomAvg = bottomAvg/delayLoops;
  
  // Write to ThingSpeak
  ThingSpeak.setField(1, voltage);
  ThingSpeak.setField(2, leftAvg);
  ThingSpeak.setField(3, rightAvg);
  ThingSpeak.setField(4, topAvg);
  ThingSpeak.setField(5, bottomAvg);
  ThingSpeak.setField(6, facingSun);
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
}
