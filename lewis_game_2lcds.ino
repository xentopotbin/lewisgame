
 /*
   Play a 2x2 Lewis signaling game!
   This version reports success rate via an Adafruit SSD1306 display

   Function for reading & debouncing the buttons adapted from Adafruit:
   https://blog.adafruit.com/2009/10/20/example-code-for-multi-button-checker-with-debouncing/

   This version uses 2 I2C LCD displays to track the players' correct responses.
   One display for the signaler, one for the receiver.
   To prevent conflicts, they need to have different I2C addresses.

  */

  #include <SPI.h>
  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>

  //initialize LCD screens
  LiquidCrystal_I2C sigLcd(0x27, 16, 2); //16 chars, 2 lines
  LiquidCrystal_I2C recLcd(0x26, 16, 2);

  // timers
  const byte debounce = 10;     //debounce timer in ms
  const int blinkTime = 300;    //How quickly should the reward light blink?

  //Arrays to hold button info
  const byte buttons[] = {2, 3, 4, 5, 6};     //pin #s for the buttons
  const byte numButtons = sizeof(buttons);    //how many buttons do we have, anyway?
  byte pressed[numButtons];                   //is each button pressed or not?
  byte justpressed[numButtons];               //has each button newly pressed?

  //Button names
  // these refer to the button's location in the arrays, NOT their pin #s
  const byte controlKey = 0;  //Press to start a trial
  const byte sigKey1 = 1;   //yellow
  const byte sigKey2 = 2;   //blue
  const byte recKey1 = 3;   //red
  const byte recKey2 = 4;   //green

  // Pin numbers of the lights
  const byte sigLed1 = 7;   //red
  const byte sigLed2 = 8;   //green
  const byte sigRewardLed = 9;

  const byte recLed1 = 10;   //yellow
  const byte recLed2 = 11;   //blue
  const byte recRewardLed = 12;

  //within-trial variables
  byte condition = 1;
  byte sigChoice = 1;
  byte recChoice = 1;
  byte trialState = 0;
    // 0 =  No trial active, waiting for control button press
    // 1 = Control button pressed, condition displayed, waiting for signaler action
    // 2 = Signaler button pressed, signal displayed, waiting for receiver action

  //variables for keeping track of success rate
  int numTrials = 0;
  int numCorrect = 0;
  float pCorrect = 0;

void setup() {

  //Set random seed by reading from empty analog pin 0
  randomSeed(analogRead(0));

  //set buttons as inputs & enable internal pullup resistors
  for (byte i = 0; i < numButtons; i++) {
    pinMode(buttons[i], INPUT_PULLUP);
    }

  //set LEDs as outputs
  //this is a stupid shortcut
  for (byte i = 7; i < 13; i++) {
    pinMode(i, OUTPUT);
  }

  //Initialize LCD displays
  sigLcd.init();
  sigLcd.backlight();

  recLcd.init();
  recLcd.backlight();

  update_display(sigLcd);
  update_display(recLcd);

}

void loop() {

  check_buttons(); //get the current state of all buttons

  if (trialState == 0) {
    // check master button, if pressed:
    // randomize condition, turn on lights for signaler, progress to trialState == 1

    if(justpressed[controlKey] == HIGH) {
      condition = random(1,3);      //Randomize condition between 1-2

      if (condition == 1) {
        digitalWrite(sigLed1, HIGH);    //Turn on light 1
      }
      else {
        digitalWrite(sigLed2, HIGH);    //Turn on light 2
      }

      trialState = 1;                   //Progress to next stage
    }

    }
  else if (trialState == 1) {
    // check the 2 signaler buttons, if one is pressed:
    // turn on the corresponding light for receiver, progress to trialState == 2

    //XOR - we only want one key pressed at a time
    if (((justpressed[sigKey1] == HIGH) && (pressed[sigKey2] == LOW)) ||
    ((pressed[sigKey1] == LOW) && (justpressed[sigKey2] == HIGH))) {

      if (justpressed[sigKey1] == HIGH) {   //Pressed button 1
        digitalWrite(recLed1, HIGH);        //Turn on signal 1
      }
      else {                                 //Pressed button 2
        digitalWrite(recLed2, HIGH);         //Turn on signal 2
      }

      trialState = 2;                       //Progress to next stage
    }


  }
  else if (trialState == 2) {
    // check the 2 receiver buttons, if pressed:
    // check if button matches condition, if so, blink reward lights
    // update info, turn off all lights, restart at trialState == 0

    //XOR - we only want one key pressed at a time
    if (((justpressed[recKey1] == HIGH) && (pressed[recKey2] == LOW)) ||
    ((pressed[recKey1] == LOW && (justpressed[recKey2] == HIGH)))) {

      numTrials++;                //Increment trial counter

      if (justpressed[recKey1] == HIGH) {     //Pressed button 1
        recChoice = 1;
      }
      else {                                  //Pressed button 2
        recChoice = 2;
      }

      if (recChoice == condition) {         //Did the receiver correctly match the condition?

        numCorrect++;                       //If so, increment correct trials
        pCorrect = numCorrect / numTrials;  //Calculate p(Correct)

        for(byte i = 0; i < 3; i++) {       //Blink reward lights 3x
          digitalWrite(sigRewardLed, HIGH);
          digitalWrite(recRewardLed, HIGH);
          delay(blinkTime);
          digitalWrite(sigRewardLed, LOW);
          digitalWrite(recRewardLed, LOW);
          delay(blinkTime);
        }
      }

      digitalWrite(sigLed1, LOW);           //Turn off all the lights
      digitalWrite(sigLed2, LOW);
      digitalWrite(recLed1, LOW);
      digitalWrite(recLed2, LOW);

      update_display(sigLcd);                //Display updated results
      update_display(recLcd);

      trialState = 0;                       //Trial is complete, restart
    }
  }

 }

 // Check the state of all buttons
 void check_buttons() {

   static byte currentstate[numButtons];
   static byte previousstate[numButtons];
   static long lasttime;

   //Take 2 readings, 10 MS apart, only keeps the reading if both reads are the same
   if (millis() < lasttime) {
     lasttime = millis();
     }

    if ((lasttime + debounce) > millis()) {
     return; // not enough time has passed to debounce
    }

    // we have waited DEBOUNCE milliseconds, reset the timer
    lasttime = millis();

   for(byte i = 0; i < numButtons; i++) {
     justpressed[i] = 0; // clear out the "just pressed" indicators

     currentstate[i] = !digitalRead(buttons[i]); // read the button, invert to compensate for pullup resistor stuff

     if (currentstate[i] == previousstate[i]) {  //If the 2 reads were the same, probably not noise
       if ((pressed[i] == LOW) && (currentstate[i] == HIGH)) {
         // button was just pressed
         justpressed[i] = 1;
         }
       pressed[i] = currentstate[i];   // record the reading
       }
      previousstate[i] = currentstate[i];
     }

 }

 //Update a display
 //will this work?  still needs testing
 // "passing by reference "
 void update_display(LiquidCrystal_I2C &lcd) {
   lcd.clear();             //clear display
   lcd.setCursor(0,0);      //1st row 1st column
   lcd.print("Trials: ");
   lcd.print(numTrials);
   lcd.setCursor(0,1);      //2nd row 1st column
   lcd.print("Correct: ");
   lcd.print(numCorrect);
 }
