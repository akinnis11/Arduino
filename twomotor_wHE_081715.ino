//AK note: biggest change between version "twomotor_wHE_021115" and "twomotor_wHE_041515" is that for lick detection
//switched from using the MPR121 board (which causes electrical artifacts) to an analog IR detector circuit. This can
//either a transmitive "beam-break" design or a reflective, one-sided design (which i'm currently using). Most likely
//they can be implented within arduino in the same way.


// Initialize STEPPER library info
#include <Stepper.h>

String programName = "twomotor_wHE_011715";

//Digital pins
int rewPin = 4; //Digital 4; sends TTL whenever solenoid is opened/closed
int trialStartPin = 5; //Digital pin 0; sends TTL pulse at the start of every trial
int motorPin1_upper = 6;  // Digital output pins for one of the motors (middle h-bridge)
int enable_upper = 7;
int motorPin2_upper = 8;
int lickPin = 9;
int punPin = 10; //Digital 10; sends TTL when animal recevies timeout
int motorPin1_lower = 11;  // Digital output pins for one of the motors
int enable_lower = 12;
int motorPin2_lower = 13;

//Analog pins
int hallSensPinUpper = A1; //analog pin 1 on the board
int hallSensPinLower = A2; //analog pin 2 on the board
int IRdetectPin = A3;//Analog input pin for lick detector

//Trial variables
long trialstart = 0;
long respDur_start = 0;
long iti_start = 0;
long randNumber; //random number to determine trial type
const int iti = 1500;
const int stimDur = 2000;
const int respDur = 1000;
const int solTime = 40; //time soleniod valve is open for delivery of water; controls how much water comes out each time
const int punTime = 12000; //time in msec of timeout for unrewarded licks
int sessionStart = 1;
int randNum3 = 0;

//Stimulus variables
const int rewType = 1; //Determines whether top stimulus is rewarded (= 1) or bottom stimulus is rewarded (= 2)
int stimType = 0; // Value changes depending on trial type (1 = rewarded, 2 = unrewarded, 3 = both stim, 4 = no stim)
const int rewStim = 25; //Percent rewarded stim only trials
const int unrewStim = 25; //Percent unrewarded stim only trials
const int bothStim = 25; //Percent trials with both stimuli present (rewarded)
const int noStim = 25; //Percent trials with no stimuli present (unrewarded)

//Motor-related variables
const int motorSteps = 200; // defines the # steps in one revolution in the motor.
const int stepNum = 20; // step size (50 = 1/4 rotation of arm)
const int stepNumMax = 80;
const int motorSpeed = 60; //speed at which motors move

Stepper lowerStepper(motorSteps, motorPin1_lower, motorPin2_lower);
Stepper upperStepper(motorSteps, motorPin1_upper, motorPin2_upper);

int stepCount = 1;         // number of steps the motor has taken

//Hall effect sensor variables
int hallSensValUpper = 500;
int hallSensValLower = 500;
int hallThreshUpper = 200; //Changing the thresholds can alter the exact position at which the motors stop
int hallThreshLower = 200;
int stepsUpper = stepNum;
int stepsLower = stepNum;
int upperMove = 0;
int lowerMove = 0;

//Lick detector variables
int IRthresh = 500; //970; //Analog threshold for detecting a lick event
int lickLockout = 100; //in ms; the 'refractory' period after each lick before next can be detected
int lickState = 0; //Whether animal is currently licking ('1') or not ('0')
int rewState = 0; //Value changed to 1 when animal is reward or punished on each trial.
int rewVar = 0;
int respTime = 0; //Time in msec of reward/punish for each trial
int currRead;
int currTime2;
int lickTime;
int lickRewDelay = 200; //in ms; time between lick response and solenoid opening


//////////////////////////////////////////
void setup() {

  // sets up analog pins as input pins
  pinMode(hallSensPinUpper, INPUT);
  pinMode(hallSensPinLower, INPUT);
  pinMode(IRdetectPin, INPUT);

  // sets up digital pins as output pins
  pinMode(rewPin, OUTPUT);
  pinMode(trialStartPin, OUTPUT);
  pinMode(punPin, OUTPUT);
  pinMode(lickPin, OUTPUT);
  pinMode(enable_lower, OUTPUT);
  pinMode(enable_upper, OUTPUT);

  // sets all digital pins to 'LOW' at start of session
  digitalWrite(rewPin, LOW);
  digitalWrite(trialStartPin, LOW);
  digitalWrite(punPin, LOW);
  digitalWrite(lickPin, LOW);
  digitalWrite(enable_lower, LOW);
  digitalWrite(enable_upper, LOW);

  // sets the speed at which the motor will turn in rotations per minute
  lowerStepper.setSpeed(motorSpeed);
  upperStepper.setSpeed(motorSpeed);

  // initialize the serial port: (sets up communication with computer at a speed of 9600 bits per second
  Serial.begin(9600);
  //Wire.begin();

  randomSeed(analogRead(2)); //ensures a new random number seed for each session

  //Display any notes/info at beginning of session:
  Serial.println("Program name:");
  Serial.println(programName);
  Serial.println("Rewarded stimulus (1=top, 2=bot)");
  Serial.println(rewType);
  Serial.println("Percent trials rewarded stim only = ");
  Serial.println(rewStim);
  Serial.println("Percent trials unrewarded stim only = ");
  Serial.println(unrewStim);
  Serial.println("Percent trials both stim = ");
  Serial.println(bothStim);
  Serial.println("Percent trials neither stim = ");
  Serial.println(noStim);
  Serial.println("reward amount/duration =");
  Serial.println(solTime);
  Serial.println("stimDur =");
  Serial.println(stimDur);
  Serial.println("respDur =");
  Serial.println(respDur);
  Serial.println("ITI =");
  Serial.println(iti);
  Serial.println("Timeout/punishment duration = ");
  Serial.println(punTime);
  Serial.println("Motor speed = ");
  Serial.println(motorSpeed);
  Serial.println("Number of steps for motor = ");
  Serial.println(stepNum);
  Serial.println("Notes: ");
  Serial.println("Added hall effect sensors");
  Serial.println("Both motors moving every trials");
  Serial.println("Stimuli painted black");
  Serial.println("Using IR beam detector for lick");
  Serial.println("Put on new black 3D printed shapes for first time"); //Print a blank line at the start of each trial
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void loop()  { //START MAIN LOOP

  if (sessionStart == 0) {

    delay(1000);
    hallSensValLower = analogRead(hallSensPinLower);
    hallSensValUpper = analogRead(hallSensPinUpper);

    moveSteppersForward(3, rewType, stepNumMax); //Move upper stim forward to adjust postitions

    Serial.println("Allow time for set up...");
    delay(30000); //Give a few seconds to setup stimuli in correct positions at whiskers

    moveSteppersBack(3, rewType, stepNumMax); //Move both stim back to begin session

    Serial.println("Starting session");
    sessionStart = 1;
    delay(iti);

  }

  Serial.println(" "); //Print a blank line at the start of each trial
  //Set up trial type
  stimType = 0;
  stimType = trialType(); // determine trial type at the start of each trial

  trialstart = millis(); // start time of each trial in msec
  digitalWrite(trialStartPin, HIGH);
  delay(50);
  digitalWrite(trialStartPin, LOW);
  rewState = 0; //Set to zero at the start of each trial

  Serial.print("Trial start time: ");
  Serial.println(trialstart);
  //Serial.println(analogRead(IRdetectPin));


  //Determine how to move the stimuli based on trial type and which stimulus is rewarded for this animal
  moveSteppersForward(stimType, rewType, stepNumMax);
  delay(20); //motor movement affects lick detector reading; better if wait a few milliseconds before taking the reading

  lickState = 0;
  //Check for licks during the stimulus period
  while ((millis() - trialstart) <= stimDur) {

    readTouchInputs(); //Checks the lick state; changes 'lickState' from 0 to 1 if touch occurred.

    if (lickState == 1) { //If animal licks

      if (rewState == 0) { // if animal hasn't already been rewarded/punished this trial

        respTime = millis();
        delay(lickRewDelay); //puts a short delay before reward/punishment given

        if (rewVar == 1) { // Reward for rewarded stim only trials (== 1) or both stim trials (== 3)
          reward(respTime);
        }
        else { // Punish for unrewarded stim trials (== 2) or neither stim trials (== 4)
          punish(respTime);
        }
      } // end rewState
    } // end lickState
  } // End of stimulus duration

  //Move motors back
  moveSteppersBack(stimType, rewType, stepNumMax);
  delay(10);

  lickState = 0;
  respDur_start = millis();
  //Check for licks during the response period after stimulus is gone
  while ((millis() - respDur_start) <= respDur) {
    readTouchInputs();
    if (lickState == 1) { //If animal licks

      if (rewState == 0) { // if animal hasn't already been rewarded/punished this trial

        respTime = millis();
        delay(lickRewDelay); //puts a short delay before reward/punishment given
        
        if (rewVar == 1) { // Reward for rewarded stim only trials (== 1) or both stim trials (== 3)
          reward(respTime);
        }
        else { // Punish for unrewarded stim trials (== 2) or neither stim trials (== 4)
          punish(respTime);
        }
      } // end rewState
    } // end lickState
  }

  lickState = 0;
  iti_start = millis();
  randNum3 = random(0, 1000);

  //Check for licks during the ITI
  while ((millis() - iti_start) <= (iti + randNum3)) {
    readTouchInputs();
  }
} //END MAIN LOOP


/////////////////////////////////////////////////////////////////////////////////////////////////////
//START OF SUB-FUNCTIONS
int trialType() {

  long randNumber = random(0, 100);

  if (randNumber <= rewStim) {
    Serial.println("Rewarded stimulus only");
    rewVar = 1;
    if (rewType == 1) {
      stimType = 1;
    }
    else {
      stimType = 2;
    }
    digitalWrite(rewPin, HIGH);
    delay(1);
    digitalWrite(rewPin, LOW);
  }
  else if (randNumber > rewStim && randNumber <= (rewStim + unrewStim)) {
    Serial.println("Unrewarded stimulus only");
    rewVar = 0;
    if (rewType == 1) {
      stimType = 2;
    }
    else {
      stimType = 1;
    }    
    digitalWrite(punPin, HIGH);
    delay(1);
    digitalWrite(punPin, LOW);
  }
  else if (randNumber > (rewStim + unrewStim) && randNumber <= (rewStim + unrewStim + bothStim)) {
    Serial.println("Both stimuli");
    stimType = 3;
    rewVar = 1;
    digitalWrite(rewPin, HIGH);
    digitalWrite(punPin, HIGH);
    delay(1);
    digitalWrite(rewPin, LOW);
    digitalWrite(punPin, LOW);
  }
  else if (randNumber > (rewStim + unrewStim + bothStim)) {
    Serial.println("Neither stimuli");
    stimType = 4;
    rewVar = 0;
  }
  return stimType;
}

////////////////////////////////////////////
void moveSteppersForward(int stimtype, int rewtype, int stepNumber) {
  //  int st = stimtype;
  //
  //  if (rewtype == 2) { //Change rewarded/unrewarded motor movement depending on which stim is rewarded
  //    if (stimtype == 1) {
  //      st = 1;
  //    }
  //    else if (stimtype == 2) {
  //      st = 2;
  //    }
  //  }

  
  int moveUpper = 0;
  int moveLower = 0;
  int s = 0;
  int sL = 0;
  int sU = 0;

  digitalWrite(enable_upper, HIGH);
  digitalWrite(enable_lower, HIGH);

  switch (stimtype)
  {
    case 1:  //Upper stim moved into whisker field

      moveUpper = 1; //move into whiskers based on HE sensor
      moveLower = 1; //move away from whiskers based on number of steps

      stepsUpper = 0;
      stepsLower = stepsLower; //still using value from last trial
      //while (moveUpper != 0) {
      for (int i = 0; i <= stepNumber; i++) { //Will loop 80 times; therefore that's the max # steps the motor can make

        if (s < (stepsLower)) {  //moving until number of steps is achieved
          s = s + 1;
        }
        else {
          moveLower = 0;
        }

        hallSensValUpper = analogRead(hallSensPinUpper);

        if (hallSensValUpper > hallThreshUpper) { // && moveUpper != 0) { //moving until HE sensor crossed
          stepsUpper = stepsUpper + 1;
        }
        else {
          moveUpper = 0;
        }
        lowerStepper.step(moveLower);
        delay(1);  // slight delay after movement to ensure proper step before next
        upperStepper.step(moveUpper);
        delay(1);

        hallSensValUpper = analogRead(hallSensPinUpper);
      }
      stepsLower = s;
      break;

    case 2:

      moveUpper = -1; //move away from whiskers based on number of steps
      moveLower = -1; //move into whiskers based on HE sensor

      stepsUpper = stepsUpper; //using value from last trial
      stepsLower = 0;
      //while (moveLower !=0) {
      for (int i = 0; i <= stepNumber; i++) { //Will loop 80 times; therefore that's the max # steps the motor can make

        if (s < (stepsUpper)) {  //moving until number of steps is achieved
          s = s + 1;
        }
        else {
          moveUpper = 0;
        }

        hallSensValLower = analogRead(hallSensPinLower);

        if (hallSensValLower > hallThreshLower && moveLower != 0) { //moving until HE sensor crossed
          stepsLower = stepsLower + 1;
        }
        else {
          moveLower = 0;
        }
        lowerStepper.step(moveLower);
        delay(1);  // slight delay after movement to ensure proper step before next
        upperStepper.step(moveUpper);
        delay(1);

        hallSensValLower = analogRead(hallSensPinLower);
      }
      stepsUpper = s;
      break;

    case 3:
      moveUpper = 1;
      moveLower = -1;

      stepsUpper = 0;
      stepsLower = 0;

      //while (moveUpper != 0 && moveLower != 0) {
      for (int i = 0; i <= stepNumber; i++) { //Will loop 80 times; therefore that's the max # steps the motor can make

        hallSensValUpper = analogRead(hallSensPinUpper);

        if (hallSensValUpper > hallThreshUpper) { // && moveUpper != 0) { //moving until HE sensor crossed
          stepsUpper = stepsUpper + 1;
        }
        else {
          moveUpper = 0;
        }

        hallSensValLower = analogRead(hallSensPinLower);

        if (hallSensValLower > hallThreshLower) { // && moveLower != 0) { //moving until HE sensor crossed
          stepsLower = stepsLower + 1;
        }
        else {
          moveLower = 0;
        }
        lowerStepper.step(moveLower);
        delay(1);  // slight delay after movement to ensure proper step before next
        upperStepper.step(moveUpper);
        delay(1);
      }
      break;

    case 4:
      moveUpper = -1;
      moveLower = 1;

      stepsLower = stepsLower; //using value from last trial
      stepsUpper = stepsUpper; //using value from last trial

      //while (moveLower !=0 || moveUpper != 0) {
      for (int i = 0; i <= stepNumber; i++) { //Will loop 80 times; therefore that's the max # steps the motor can make
        if (sU < (stepsUpper)) {  //moving until number of steps is achieved
          sU = sU + 1;
        }
        else {
          moveUpper = 0;
        }
        if (sL < (stepsLower)) {  //moving until number of steps is achieved
          sL = sL + 1;
        }
        else {
          moveLower = 0;
        }
        lowerStepper.step(moveLower);
        delay(1);  // slight delay after movement to ensure proper step before next
        upperStepper.step(moveUpper);
        delay(1);
      }
      stepsUpper = sU;
      stepsLower = sL;

      break;
  }
  //digitalWrite(enable_upper, LOW);
  //digitalWrite(enable_lower, LOW);
}

void moveSteppersBack(int stimtype, int rewtype, int stepNumber) {
  //  int st = stimtype;
  //
  //  if (rewtype == 2) { //Change rewarded/unrewarded motor movement depending on which stim is rewarded
  //    if (stimtype == 1) {
  //      st = 1;
  //    }
  //    else if (stimtype == 2) {
  //      st = 2;
  //    }
  //  }

  int sU = 0;
  int sL = 0;
  int returnUpper = 0;
  int returnLower = 0;
  stepsUpper = stepNum;
  stepsLower = stepNum;

  //digitalWrite(enable_lower, HIGH);
  //digitalWrite(enable_upper, HIGH);

  switch (stimtype)
  {
    case 1:
      returnUpper = -1;
      returnLower = -1;
      break;
    case 2:
      returnUpper = 1;
      returnLower = 1;
      break;
    case 3:
      returnUpper = -1;
      returnLower = 1;
      break;
    case 4:
      returnUpper = 1;
      returnLower = -1;
      break;
  }
  //while (returnLower !=0 || returnUpper != 0) {
  for (int i = 0; i <= stepNumber; i++) { //Will loop 80 times; therefore that's the max # steps the motor can make

    sU = sU + 1;
    sL = sL + 1;
    if (sU >= (stepsUpper)) {  //moving until number of steps is achieved
      returnUpper = 0;
    }
    if (sL >= (stepsLower)) {  //moving until number of steps is achieved
      returnLower = 0;
    }
    lowerStepper.step(returnLower);
    delay(1);  // slight delay after movement to ensure proper step before next
    upperStepper.step(returnUpper);
    delay(1);
  }
  digitalWrite(enable_lower, LOW);
  digitalWrite(enable_upper, LOW);
}

void reward(int resptime) {
  Serial.println("Reward!");

  digitalWrite(rewPin, HIGH); //Open soleniod valve
  delay(50);
  digitalWrite(rewPin, LOW); //Close solenoid valve
  rewState = 1; //Set equal to 1 when reward/punishment given; zero otherwise. Ensures only 1 reward/punishment given per trial.

}

void punish(int resptime) {

  int t = millis();

  Serial.println("Unrewarded lick, timeout");
  digitalWrite(punPin, HIGH);
  while ((t - resptime) <= punTime) {
    readTouchInputs();
    t = millis();
  }
  digitalWrite(punPin, LOW);
  //delay(punTime);

  rewState = 1; //Set equal to 1 when reward/punishment given; zero otherwise. Ensures only 1 reward/punishment given per trial.

}
///////////
// Checks for touch inputs to the lick detector

void readTouchInputs() {
  currRead = analogRead(IRdetectPin);
  int currRead2 = analogRead(IRdetectPin);

  currTime2 = millis();

  //if (currRead < 1000) {
  //Serial.println(currRead);
  //}
  //Serial.println(lickState);

  if (lickState == 0) { //If animal isn't currently licking
    //if ((currTime2 - lickTime) > lickLockout) { //Sets a refractory period so 'licks' are only detected at a certain frequency. Helps with false positives.
    if (currRead < IRthresh) {
      Serial.print("Lick: ");
      Serial.println(millis());
      digitalWrite(lickPin, HIGH);
      delay(1);
      digitalWrite(lickPin, LOW);
      //Serial.println(currRead);
      lickState = 1;
      lickTime = currTime2;
    }
    //}
    else {
      lickState = 0;
    }
  }
  else {
    if (currRead < IRthresh) {
      lickState = 1;
    }
    else {
      lickState = 0;
    }
  }
}























