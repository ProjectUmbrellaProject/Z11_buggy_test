#include <Pixy.h>
#include <SPI.h> 

const int leftMotorPin = 4, rightMotorPin = 7, speedPin = 3, trigPin = 9, echoPin = 8, gantryIRPIN = 2;

String inputString = "";

unsigned long previousPingTime;
unsigned long gantryDetectionTime;
const int pingInterval = 400; //Determines how frequently the distance is measured from the ultrasonic sensor
const short minimumDistance = 15; //Determines how close an object must be to stop the buggy
const int gantryWaitTime = 1500; //Determines how long the buggy waits after detecting a gantry
int motorPower = 170;
bool forward, objectDetected, stringComplete, gantryDetected, onHalfSpeed;

// This is the main Pixy object
Pixy pixy;
static int frameCounter = 0;
const int minimumDetection = 4; //Determines how many times a colour must be detected in 10 frames before the buggy responds


void setup() {
  //Declare output and input pins
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(speedPin, OUTPUT);
  pinMode(leftMotorPin, OUTPUT);
  pinMode(rightMotorPin, OUTPUT);
  pinMode(gantryIRPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(gantryIRPIN), gantryInterrupt, CHANGE);


  //Set the motor states to max power but off
  analogWrite(speedPin, motorPower);
  digitalWrite(leftMotorPin, HIGH);
  digitalWrite(rightMotorPin, HIGH);
  
  stringComplete = false;
  objectDetected = false;
  forward = false;
  gantryDetected = false;
  onHalfSpeed = false;
    
  Serial.begin(9600); // initiate serial commubnication at 9600 baud rate
  Serial.print("+++"); //Enter xbee AT commenad mode, NB no carriage return here
  delay(1500);  // Guard time
  Serial.println("ATID 3311, CH C, CN"); // PAN ID 3311
  delay(1100);
  while(Serial.read() != -1) {}; // get rid of OK
    inputString.reserve(200);

  //Send status to monitoring program
  Serial.println("Buggy: Setup Complete.");

  pixy.init();
}

void loop() {
  
  unsigned long currentTime = millis(); //Update the time variable with the current time
  if (currentTime - previousPingTime >= pingInterval){
    previousPingTime = currentTime; 

    handleObjectDetection();
  }
  
  if (stringComplete) {
    //The stringComplete bool indicates whether or not a new command has been recieved
     moveCommand(inputString.toInt());
    
    // clear the string:
    inputString = "";
    stringComplete = false;
  }

  if (gantryDetected){
    Serial.println("~7");
    moveCommand(0);
    delay(gantryWaitTime); //Delay because nothing useful has to happen under the gantry
    moveCommand(1);
    delay(500); //Allow the buggy to move away from the gantry before checking the IR receiver again
    
    gantryDetected = false;
  }

 // detectSigns();
}

void moveCommand(int command){
    switch (command){

      //Stop
      case 0:
        delay(20);
        digitalWrite(leftMotorPin, HIGH);
        digitalWrite(rightMotorPin, HIGH);
        forward = false;
        Serial.println("~4");
        Serial.print("~8");
        Serial.println(0);
        break;
  
      //Move Forward
      case 1:
        delay(20);
        digitalWrite(leftMotorPin, LOW);
        digitalWrite(rightMotorPin, LOW);
        Serial.println("~9");
        Serial.print("~8");
        Serial.println(motorPower);
        forward = true;
      
        break;

       //Reduce speed
       case 2:
        if(!onHalfSpeed){
          motorPower = motorPower/2;
          analogWrite(speedPin, motorPower);
          Serial.print("~8");
          Serial.println(motorPower);
          onHalfSpeed = true;
        }
        break;
      
      //Resume full speed
      case 3:
        if(onHalfSpeed){
          onHalfSpeed = false;
          motorPower = 2* motorPower;
          analogWrite(speedPin, motorPower);
          Serial.print("~8");
          Serial.println(motorPower);
        }
        break;

      //Turn right
      case 4:
        delay(200);
        digitalWrite(leftMotorPin, HIGH);
        delay(200);
        digitalWrite(leftMotorPin, LOW);
        break;

      //Turn left
      case 5:
        delay(200);
        digitalWrite(rightMotorPin, HIGH);
        delay(200);
        digitalWrite(rightMotorPin, LOW);
        break;
        
      default:
        Serial.println("~6");
        break;
  }
  
}
//Moved from main loop to improve readability and reduce loop lenght
void handleObjectDetection(){
    long distance = obstacleDistance();

    //The command to stop should only be called if the control program has told the buggy to move and an object hasnt already been detected
    //i.e. the buggy isnt already stationary
    if (!objectDetected && distance <= minimumDistance && forward){
      
      objectDetected = true; //The objectDetected boolean prevents the if statement from being repeatedly executed while the object is still present
      moveCommand(0);
      forward = true; //Calling move command 0 also sets forward to false which is unintended in this case
      Serial.print("~5");
      Serial.println(distance);

    }
    else if (objectDetected && distance > minimumDistance && forward){
        objectDetected = false;
        moveCommand(1);
    }
}

long obstacleDistance(){
  long distance, duration;

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculating the distance
  distance = round(duration*0.034/2);

  return distance;

  
}

void gantryInterrupt(){
  //Interrupts have to be as short as possible to avoid slowing done the program. The flag updated in this function will allow the loop to handle the gantrydetection
  gantryDetected = true;
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    
    // add it to the inputString:
    inputString += inChar;
    
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

void detectSigns(){
  uint16_t blocks = pixy.getBlocks();
  int detections[4] = {0, 0, 0, 0};
  
  if (blocks > 0){
    frameCounter++;

    //Check after every 10 frames (frame rate is 50fps)
    if (frameCounter%10==0) {
       
       /*for(int k = 0; k < blocks; k++){
          if(pixy.blocks[k].y > closestY){
            closestY = pixy.blocks[k].y;
            closestID = k;
          }
       }*/
      for (int i = 0; i < blocks; i++){
        detections[pixy.blocks[i].signature - 1]++; //Add to the counter for how many times the colour has been detected

        if (detections[pixy.blocks[i].signature - 1] > minimumDetection)
        //do stuff
        
      }
       
       moveCommand(pixy.blocks[closestID].signature +1);
       
      }
    }
  }  







