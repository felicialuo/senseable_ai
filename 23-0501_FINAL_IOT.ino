struct Prediction {
  int pred1;
  int pred2;
  int pred3;
};

#include <stdlib.h>
#include <string.h>
// For bluetooth communication
#include <ArduinoBLE.h>
#include <string>

bool startReceiving = false;
String receivedMessage = "";

const char* deviceServiceUuid = "19b10000-e8f2-537e-4f6c-d104768a1214";
const char* deviceServiceCharacteristicUuid = "19b10001-e8f2-537e-4f6c-d104768a1214";

BLEService messageService(deviceServiceUuid); 
BLEByteCharacteristic messageCharacteristic(deviceServiceCharacteristicUuid, BLERead | BLEWrite);


// Reset switch setup
int switchPin = 3;

// Servo setup
#include <Servo.h>

int servoPin = 2;
Servo servo;
// bool updatePosition = true;
String SERVO_POS = "neutral";


// Proximity sensor setup
#include <Wire.h>
#include "Adafruit_VCNL4010.h"
Adafruit_VCNL4010 vcnl;


// LED ring setup
#include <Adafruit_NeoPixel.h>
// IMPORTANT: Set pixel COUNT, PIN and TYPE
#define PIXEL_PIN 8
#define PIXEL_COUNT 16
#define PIXEL_TYPE NEO_GRB + NEO_KHZ800

#define LED_PIN    7
#define LED_COUNT  23

Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


// Thermal printer setup
#include "Adafruit_Thermal.h"
#include "adalogo.h"
#include "adaqrcode.h"
// #define BAUDRATE  19200
Adafruit_Thermal printer(&Serial1);      // Or Serial2, Serial3, etc.

// Variables init
unsigned long prox_previousMillis = 0;
const unsigned long interval = 1000;
String DWGPAD_STATUS = "empty";
bool pred_true = false;
String pred_result = "";
int play_run = 1;
int pass_run = 2;

char labels[4][3][20] = {
  {
    "apple",
    "banana",
    "pineapple"
  },
  {
    "strawberry",
    "cherry",
    "raspberry"
  },
  {
    "pineapple",
    "tomato",
    "dragonfruit"
  },
  {
    "orange",
    "mango",
    "watermelon"
  }
};

void getLabels(char* label1, char* label2, char* label3) {
  randomSeed(analogRead(0));
  int block = random(4);     // randomly select a block (0 or 1)
  strcpy(label1, labels[block][0]);   
  strcpy(label2, labels[block][1]);
  strcpy(label3, labels[block][2]);
}



// -----------------------------------------------------------------------

void setup()
{
  // Start a serial connection 
  Serial.begin( 9600 );
  // Thermal printer serial
  Serial1.begin(19200); // Use this instead if using hardware serial
  printer.begin();        // Init printer (same regardless of serial type)

  randomSeed(analogRead(0));

  // set up reset switch
  pinMode( switchPin , INPUT_PULLUP); // sets pin as input

  // set up the servo on D2
  servo.attach( servoPin );

  // test proximity connection
  if (!vcnl.begin()) {
      Serial.println("\t Proximity sensor not found :(");
      while (1);
  }
  Serial.println("\t Found VCNL4010 \n");

  // light up LED ring
  strip1.begin();
  strip1.show(); // Initialize all pixels to 'off'
  for(int i=0; i< strip1.numPixels(); i++) {
    strip1.setPixelColor(i, 255, 190, 100 );
     strip1.setBrightness(255);
		strip1.show();
  }

  strip2.begin();           // Initialize the LED strip
  strip2.show();            // Turn all LEDs off initially
  strip2.setBrightness(15);   // Set overall brightness to 50 (out of 255)

  LED_MACH_INIT(strip2.Color(255, 255, 255), 200);

  // Set up bluetooth
  if (!BLE.begin()) {
    Serial.println("- Starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  BLE.setLocalName("Arduino Nano 33 BLE (Peripheral)");
  BLE.setAdvertisedService(messageService);
  messageService.addCharacteristic(messageCharacteristic);
  BLE.addService(messageService);
  messageCharacteristic.writeValue(-1);
  BLE.advertise();

  Serial.println("\t Nano 33 BLE (Peripheral Device)\n");

  delay(5000);
}


// -----------------------------------------------------------------------

void loop()
{
  Serial.print("--------current switch state: ");
  Serial.println(digitalRead(switchPin));
  // only start the game when reset button is UNpressed
  while(digitalRead(switchPin) == LOW);
  Serial.println("--------button UNpressed, start game");

  char label1[20], label2[20], label3[20];
  getLabels(label1, label2, label3);
  Serial.println("Current pool of labels: ");
  Serial.println(label1);
  Serial.println(label2);
  Serial.println(label3);

  // thermal print greetings
  printer.setSize('M');
  printer.justify('L');
  printer.println("********************************");
  printer.println(F("Hi there! I am Josh!"));
  printer.print(F("I am so hungry and I really want\nsome "));
  printer.println(label3);
  printer.println(F("Could you help?"));
  printer.println(F("Please take a drawing pad:)\n\n"));

  // print greetings
  Serial.println("********************************");
  Serial.println("Hi there! I am Josh!");
  Serial.print("I am so hungry and I really want\nsome ");
  Serial.print(label3);
  Serial.println(". Could you help?\n");
  Serial.println("Please take a drawing pad:)\n");

  servo_rotate("neutral");
  delay(2000);

  play_run = 1;
  pass_run = random(2,5);
  Serial.print("--------pass run: ");
  Serial.println(pass_run);
  Serial.println(" ");
  
  while(pred_true == false){
    
    servo_rotate("waiting");

    printer.println(F("********************************"));
    printer.boldOn();
    printer.underlineOn();
    printer.justify('C');
    printer.print(F("Please draw a "));
    printer.println(label3);
    printer.println(F(" and feed to my tongue!"));
    printer.boldOff();
    printer.underlineOff();
    printer.justify('L');
    printer.println(F("********************************\n\n"));

    Serial.println(F("********************************"));
    Serial.print(F("Please draw a "));
    Serial.print(label3);
    Serial.println(F(" and feed to my tongue!"));
    Serial.println(F("********************************\n\n"));

    Serial.print("--------current switch state: ");
    Serial.println(digitalRead(switchPin));
    // exit the game if switch is pushed
    if (digitalRead(switchPin) == LOW) { // Switch is pushed
      pred_true = false; // Reset variables to their initial state
      DWGPAD_STATUS = "empty";
      Serial.println("------switch pushed, RESET");
      break;
    }

    // delay(5000);

    int wait_time = 0;
    while(DWGPAD_STATUS == "empty"){
      // check proxmity distance
      int proximityValue = vcnl.readProximity();
      int proximityValue_mapped = (map(proximityValue, 65535, 2156, 0, 100));
      Serial.print("\t Proximity: ");
      Serial.println(proximityValue_mapped);
      if (proximityValue_mapped <= 50) {
        DWGPAD_STATUS = "filled";

        printer.println(F("Thank you!"));
        Serial.println("Thank you!");

        delay(1000);
        break;
      }

      if (wait_time >= 10000) {
        printer.println(F("...I am waiting...\n\n"));
        Serial.println("...I am waiting...\n\n");
        wait_time = 0;
      }
      
      Serial.print("\t DWGPAD STATUS: ");
      Serial.println(DWGPAD_STATUS);
      delay(2500);
      wait_time += 2500;
    }

    servo_rotate("neutral");

    Serial.print("--------current switch state: ");
    Serial.println(digitalRead(switchPin));
    // exit the game if switch is pushed
    if (digitalRead(switchPin) == LOW) { // Switch is pushed
      pred_true = false; // Reset variables to their initial state
      DWGPAD_STATUS = "empty";
      Serial.println("------switch pushed, RESET");
      break;
    }

    printer.println(F("Let me see what you just gave me\n\n"));
    Serial.println("Let me see what you just gave me.");

    LED_PRCSSNG(strip2.Color(255, 255, 255), 200, 2500);

    printer.println(F("...I am thinking...\n\n"));
    Serial.println("...I am thinking...\n\n");

    LED_PRCSSNG(strip2.Color(255, 255, 255), 200, 2500);

    Serial.print("--------current switch state: ");
    Serial.println(digitalRead(switchPin));
    // exit the game if switch is pushed
    if (digitalRead(switchPin) == LOW) { // Switch is pushed
      pred_true = false; // Reset variables to their initial state
      DWGPAD_STATUS = "empty";
      Serial.println("------switch pressed, RESET");
      break;
    }

    Prediction p = get_pred(play_run);

    // print the prediction result
    pred_result = "I think this is ";
    pred_result += String(p.pred1) + "% a " + String(label1) + ", ";
    pred_result += String(p.pred2) + "% a " + String(label2) + ", ";
    pred_result += String(p.pred3) + "% a " + String(label3) + ".";

    printer.boldOn();
    printer.underlineOn();
    printer.println(pred_result);
    printer.boldOff();
    printer.underlineOff();
    printer.println(F(" "));
    Serial.println(pred_result);

    String description = " ";
    if (p.pred3 >= 50){
      pred_true = true;

      printer.print(F("Yay! You gave me a "));
      printer.print(label3);
      printer.println(F(" :)\n\n"));

      Serial.print("Yay! You gave me a ");
      Serial.print(label3);
      Serial.println(" :)\n\n");
      // Signal successful classification result
      servo_rotate("collecting");
      LED_SUCCESS(5000);

      printer.print(F("Thanks for the "));
      printer.print(label3);
      printer.println(F("!\nLove it!\n"));  
      
      Serial.print("Thanks for the ");
      Serial.print(label3);
      Serial.println("!\nLove it!\n");
      Serial.println("********************************\n");
      break;
    }
    else if (p.pred2 >= 50) {

      printer.print(F("I don't want "));
      printer.print(label2);
      printer.println(F("  :/ \n"));
      description = get_description();
      printer.println(description);
      printer.println(F("\n"));

      Serial.print("I don't want ");
      Serial.print(label2);
      Serial.println("  :/ \n");
      Serial.println(description);
      Serial.println("\n");

      // Signal false classification result
      LED_FALSE(6, 300);
      servo_rotate("waiting");

    }
    else if (p.pred1 >= 50) {

      printer.print(F("I don't want "));
      printer.print(label1);
      printer.println(F("  :/ \n"));
      description = get_description();
      printer.println(description);
      printer.println(F("\n"));

      Serial.print("I don't want ");
      Serial.print(label2);
      Serial.println("  :/ \n");
      Serial.println(description);
      Serial.println("\n");
      
      // Signal false classification result
      LED_FALSE(6, 300);
      servo_rotate("waiting");

    }
    else{

      printer.println(F("I have no idea what is this =.=\n"));
      description = get_description();
      printer.println(description);
      printer.println(F("\n"));
      
      Serial.println("I have no idea what is this =.=\n");
      Serial.println(description);
      Serial.println("\n");

      servo_rotate("waiting");
      // Signal false classification result
      LED_FALSE(6, 300);

    }

    Serial.print("--------current switch state: ");
    Serial.println(digitalRead(switchPin));
    // exit the game if switch is pushed
    if (digitalRead(switchPin) == LOW) { // Switch is pushed
      pred_true = false; // Reset variables to their initial state
      DWGPAD_STATUS = "empty";
      Serial.println("------switch pressed, RESET");
      break;
    }

    // detect if drawing pad is taken
    int wait_time_retry = 0;
    while(DWGPAD_STATUS == "filled"){
      // check proxmity distance
      int proximityValue = vcnl.readProximity();
      int proximityValue_mapped = (map(proximityValue, 65535, 2156, 0, 100));
      Serial.print("\t Proximity: ");
      Serial.println(proximityValue_mapped);
      if (proximityValue_mapped >= 50) {
        DWGPAD_STATUS = "empty";
        delay(1000);
        break;
      }
      if (wait_time >= 10000) {
        printer.println(F("...Please take the drawing pad\nback and try again!...\n\n"));
        Serial.println("...Please take the drawing pad back and try again!...\n\n");
        wait_time = 0;
      }
      Serial.print("\t DWGPAD STATUS: ");
      Serial.println(DWGPAD_STATUS);
      delay(2500);
      wait_time += 2500;
      
    }

    Serial.print("--------current switch state: ");
    Serial.println(digitalRead(switchPin));
    // exit the game if switch is pushed
    if (digitalRead(switchPin) == LOW) { // Switch is pushed
      pred_true = false; // Reset variables to their initial state
      DWGPAD_STATUS = "empty";
      Serial.println("------switch pressed, RESET");
      break;
    }

    play_run += 1;
    if (play_run > pass_run){
      play_run = 1;
    }

  }

  printer.println(F("********************************\n"));
  printer.print(F("How do you think I know if it\nwas a "));
  printer.print(label3);
  printer.println(F(" or not?"));
  printer.println(F("Who am I??\n"));
  printer.println(F("Share your thoughts with friends"));
  printer.println(F("Feel free to grab this\ntranscript with you!"));
  printer.println(F("\nBye! See you next time!\n"));
  printer.println("********************************");
  printer.println(F("\n\n"));


  // Detect if drawing pad is collected, if not, stay, o.w. proceed
  while(DWGPAD_STATUS == "filled"){
      // check proxmity distance
      int proximityValue = vcnl.readProximity();
      int proximityValue_mapped = (map(proximityValue, 65535, 2156, 0, 100));
      Serial.print("\t Proximity: ");
      Serial.println(proximityValue_mapped);
      if (proximityValue_mapped >= 50) {
        DWGPAD_STATUS = "empty";
        delay(1000);
        break;
      }

      Serial.println("Collect the drawing pad!");
      Serial.print("\t DWGPAD STATUS: ");
      Serial.println(DWGPAD_STATUS);
      delay(2000);
    }

  pred_true = false;
  servo_rotate("neutral");
  delay(5000);

}


// -----------------------------------------------------------------------

String get_description() {
  String descriptions[] = {
    "You know that fruits come in\nmany different colors, right?",
    "You know that fruits come in\nmany different shapes, right?",
    "You know that fruits come in\nmany different sizes, right?",
    "I like fruits in good-looking\nappearance!",
    "It is so hard to differentiate\nsimilar-looking fruits!",
    "I may need to taste it to tell\nwhat this really is.",
    "Who doesn't like fresh and juicy\nfruits?",
    "Do you cut the fruit when you\neat it?",
    "I love sweet and juicy fruits!",
    "Have you ever eaten fruits like\nwhat you drew?",
    "You cannot fool me, this is not\nwhat I want!",
    "You should eat more fruits like\nI do.",
    "I am pretty strict in fruit\nselection."
  };

  randomSeed(analogRead(0));
  int num_descriptions = sizeof(descriptions) / sizeof(descriptions[0]);
  int index = random(num_descriptions);
  return descriptions[index];
}


Prediction get_pred(int play_run){
  Prediction p;
  if (play_run < pass_run){
    p.pred3 = random(50); 
  }
  else{
    p.pred3 = random(50, 100);
  }
  p.pred2 = random(100 - p.pred3); 
  p.pred1 = 100 - p.pred3 - p.pred2; 
  return p; 
}

void servo_rotate(String SERVO_POS){
  if (SERVO_POS == "waiting"){
    servo.write( 160 );
    // Serial.print("\nSERVO_POS:");
    // Serial.println(SERVO_POS);
  }
  else if(SERVO_POS == "collecting"){
    servo.write( 35 );
    // Serial.print("\nSERVO_POS:");
    // Serial.println(SERVO_POS);
  }
  else if (SERVO_POS == "neutral"){
    servo.write( 90 );
    // Serial.print("\nSERVO_POS:");
    // Serial.println(SERVO_POS);
  }
  else{
    Serial.println("\nWrong Servo Position Command");
  }
}

// Function to perform LED machine initiation
void LED_MACH_INIT(uint32_t color, int wait) {
  unsigned long startTime = millis();
  int i = 0;
  
  while (i < strip2.numPixels()) {
    if ((millis() - startTime) >= wait) {
      strip2.setPixelColor(i, color);
      strip2.show();
      i++;
      startTime = millis();
    }
  }
  LED_CNSTON(strip2.Color(255, 255, 255));
}

// Function to signal successful classification result
void LED_SUCCESS(unsigned long duration) {
  unsigned long startTime = millis();
  int j = 0;
  while ((millis() - startTime) < duration) {
    for (int i = 0; i < strip2.numPixels(); i++) {
      strip2.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip2.show();
    j++;
  }

  // Turn off all LEDs after animation is complete
  for (int i = 0; i < strip2.numPixels(); i++) {
    strip2.setPixelColor(i, 0);
  }
  strip2.show();
  LED_CNSTON(strip2.Color(255, 255, 255));
}

// Function to signal classification in progress
void LED_PRCSSNG(uint32_t color, int wait, unsigned long duration) {
  unsigned long startTime = millis();
  int totalTime = 0;
  
  while (totalTime < duration) {
    for (int j = 0; j < 3; j++) { // Repeat 3 times for a full cycle
      for (int q = 0; q < 3; q++) {
        for (int i = q; i < strip2.numPixels(); i += 3) {
          strip2.setPixelColor(i, color);
        }
        strip2.show();
        delay(wait);
        for (int i = q; i < strip2.numPixels(); i += 3) {
          strip2.setPixelColor(i, 0);
        }
      }
    }
    totalTime = millis() - startTime;
  }
  strip2.clear(); // Turn off all LEDs
  strip2.show(); // Update the LED strip to turn off all LEDs
  LED_CNSTON(strip2.Color(255, 255, 255));
}

// Function to signal false classification result
void LED_FALSE(int times, int interval) {
  int i = 0;
  
  while (i < times) {
    strip2.fill(strip2.Color(255, 0, 0));   // Set all LEDs to white
    strip2.show();   // Turn on all LEDs
    unsigned long startTime = millis();
    while ((millis() - startTime) < interval) {}   // Wait for the specified interval
    strip2.clear();   // Turn off all LEDs
    strip2.show();   // Update the LED strip to turn off all LEDs
    startTime = millis();
    while ((millis() - startTime) < interval) {}   // Wait for the specified interval
    i++;
  }
  LED_CNSTON(strip2.Color(255, 255, 255));
}

void LED_CNSTON(uint32_t color) {
  for (int i = 0; i < strip2.numPixels(); i++) {
    strip2.setPixelColor(i, color);
  }
  strip2.show();
}

// Function to generate a color wheel value
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
   return strip2.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return strip2.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip2.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }

}
