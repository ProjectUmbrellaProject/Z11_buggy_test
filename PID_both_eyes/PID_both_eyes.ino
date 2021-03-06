#include <Pixy.h>
#include <SPI.h> 

#define speedPin 3
#define leftMotorDirection 4
#define leftMotorSpeedPin 5
#define rightMotorSpeedPin 6
#define rightMotorDirection 7
#define leftEyePin 5
#define rightEyePin 0
#define echoPin 8
#define trigPin 9

String inputString = "";
bool stringComplete, start;

//PID variables

float black = 700, white = 35;
float K_p = 0.40, K_d = 10, K_i = 0.001, setPoint = 0, setPoint2 = black + white; //K_p < K_d the derivative term must be large to have a significant influence
int maxMotorSpeed = 255, baseSpeed = 170, corneringSpeed = 230;
int rightMotorSpeed;
int leftMotorSpeed;
int previousError;
int integral;

unsigned long previousPingTime;
const short pingInterval = 200;
const short minimumDistance = 15; //Determines how close an object must be to stop the buggy
bool objectDetected;
bool startCommand;

//Pixy variables
Pixy pixy;
const int minimumDetections = 2;
int previousDetected = -1;
int detections[4]= {0,0,0,0};

void setup() {
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(leftMotorDirection, OUTPUT);
  pinMode(leftMotorSpeedPin, OUTPUT);
  pinMode(rightMotorDirection, OUTPUT);
  pinMode(rightMotorSpeedPin, OUTPUT);
  pinMode(speedPin, OUTPUT);
  pinMode(leftEyePin, INPUT);
  pinMode(rightEyePin, INPUT);
  
  digitalWrite(speedPin, HIGH);
  analogWrite(rightMotorSpeedPin, 0);
  analogWrite(rightMotorDirection, 0);
  analogWrite(leftMotorDirection, 0);
  analogWrite(leftMotorSpeedPin, 0);

  Serial.begin(9600); 
  Serial.print("+++"); 
  delay(1500);
  Serial.println("ATID 3311, CH C, CN");
  delay(1100);
  while(Serial.read() != -1) {};
    inputString.reserve(200);

  //Send status to monitoring program
  Serial.println("Buggy: Setup Complete.");
  stringComplete = false;
  start = false;
  startCommand = false;
  integral = 0;

}

void loop() {
 
  if (stringComplete){
    moveCommand(inputString);
    
    inputString = "";
    stringComplete = false;
  }

  unsigned long currentTime = millis(); //Update the time variable with the current time
  if (currentTime - previousPingTime >= pingInterval){

      /*Serial.print("left: ");
      Serial.print(leftMotorSpeed);
      Serial.print(" right: ");
      Serial.println(rightMotorSpeed);
      */
      previousPingTime = currentTime; 
      handleObjectDetection();

    }


  //Basic start/stop commands
  if (start){
   // int eyeOutput = analogRead(leftEyePin) - analogRead(rightEyePin);// This should produce a minimum of -1000 and a max of 1000

    int error = -getSensorValue(); //Setpoint = 0 because the getSensorValue() function already returns values ~= 0 when there is very little error
    integral += error;
    //int motorSpeed = K_p * error + K_i * integral + K_d * (error - previousError);
    int motorSpeed = K_p * error + K_d * (error - previousError);
    previousError = error;
    
  
    rightMotorSpeed = baseSpeed + motorSpeed;
    leftMotorSpeed = baseSpeed - motorSpeed;


    //Constrain motors to a range of output between 0 and maxMotorSpeed
    if (rightMotorSpeed > maxMotorSpeed)
      rightMotorSpeed = maxMotorSpeed;
    else if (rightMotorSpeed < 0) //Consider changing this to run the motor in reverse instead
      rightMotorSpeed = 0;
      
    if (leftMotorSpeed > maxMotorSpeed)
      leftMotorSpeed = maxMotorSpeed;
    else if (leftMotorSpeed < 0)
      leftMotorSpeed = 0;
  
      analogWrite(speedPin, 255);
      digitalWrite(rightMotorDirection, LOW);
      analogWrite(rightMotorSpeedPin, rightMotorSpeed);
      digitalWrite(leftMotorDirection, LOW);
      analogWrite(leftMotorSpeedPin, leftMotorSpeed);
      
  } else{
    
    analogWrite(rightMotorSpeedPin, 0);
    analogWrite(rightMotorDirection, 0);
    analogWrite(leftMotorDirection, 0);
    analogWrite(leftMotorSpeedPin, 0);

  }

  //If tuned correctly it might not be necessary to reduce speed for corners
  //detectSigns(); 
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();

    inputString += inChar;
   
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

void handleObjectDetection(){
    long distance = obstacleDistance();

    //The command to stop should only be called if the control program has told the buggy to move and an object hasnt already been detected
    //i.e. the buggy isnt already stationary
    if (!objectDetected && distance <= minimumDistance && startCommand){
      
      objectDetected = true; //The objectDetected boolean prevents the if statement from being repeatedly executed while the object is still present
      moveCommand("/0 ");
      startCommand = true;
     // forward = true; //Calling move command 0 also sets forward to false which is unintended in this case
      Serial.print("~6 ");
      Serial.println(distance);

    }
    else if (objectDetected && distance > minimumDistance && startCommand){
        objectDetected = false;
        moveCommand("/1 ");
    }
}

//This function returns the distance to the nearest object as measured by the ultrasonic sensor
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

void moveCommand(String command){
  int commandNum = (command.substring(1, 3)).toInt();

  switch (commandNum){
    case 0:
      start = false;
      startCommand = false;
      Serial.println("~10");
    break;
    
    case 1:
     start = true;
     startCommand = true;
     Serial.println("~9 ");
    break;

    case 2:
      baseSpeed = (command.substring(3)).toInt();
      Serial.println("~7 ");
      Serial.println((command.substring(3)).toInt());
    break;

    case 3:
      corneringSpeed = (command.substring(3)).toInt();
      Serial.println("~7 ");
      Serial.println((command.substring(3)).toInt());
    break;

    case 4:
      K_p = (command.substring(3)).toFloat();
      Serial.println("~7 ");
      Serial.println((command.substring(3)).toFloat());
    break;

    case 5:
      K_d = (command.substring(3)).toFloat();
      Serial.println("~7 ");
      Serial.println((command.substring(3)).toFloat());
    break;

    default:
      Serial.println("~20");
    break;
  }

}

int getSensorValue(){
  int rightEye = analogRead(rightEyePin);
  int leftEye = analogRead(leftEyePin);
  int error;

  /*

   if (leftEye > 950 && rightEye > 950){//if both eyes see black the buggy is no longer following the line. Should probably do something more sophisticated here
    start = false;

    return 0; //Bad practice to have a condition that leads to nothing being returned
  }
  else*/ if (leftEye <= 650 && rightEye >= 650) //Buggy is positioned with the left eye on the edge of white and the right on black
    error = -(leftEye + rightEye);
  else if (leftEye >= 650 && rightEye <= 650) //Buggy is positioned with the right eye on the edge of white and the left on black
    error = (leftEye + rightEye);
  else
    error = (leftEye - rightEye); //Buggy is positioned with both eyes on white
  //Might be worth adding if (error > X) baseSpeed = corneringSpeed;
  /*
  //if (error < 50 && error > -50)
  //  error = 0;
  Explanation:
   * In theory (leftEye - rightEye) should work for binary situations where one eye sees white and the other sees black.
   * E.g: leftEye = 30, rightEye = 1030, => output = -1000
   * However, when the buggy drifts further and one of the eyes is above the edge of white and the other is on black this approach fails because
   * the error now decreases instead of increasing.
   * E.g: leftEye = 530, rigtEye = 1030, => output = -500, Even thought the positioning is worse
   * The if statements above fix this by changing the output to 1560 for the example above.
   

   if (leftEye > 50 || rightEye > 50){
    if (leftEye > rightEye)
      error = leftEye;
    else
      error = -rightEye;
    
  }
  else
    error = 0;
    */
   return error;
 
}

int getSensorValue2(){
  int rightEye = analogRead(rightEyePin);
  int leftEye = analogRead(leftEyePin);
  int output;
  //Negative on the right, positive on the left
  
  if (leftEye < 500 && rightEye > 500){ //Ideal case: left on white, right on black, or left on edge of white/black, right on black
    output = leftEye + rightEye; 
  }
  else if (leftEye < 500 && rightEye < 500) //left on black, right on white
    output = leftEye + rightEye;

  else if (leftEye > 500 && rightEye < 500) //left on black, right on the edge of white/black
    //output = -leftEye + rightEye;
    output = 0;

    //this wont work because output doesnt have equal range on both sides


  
 return (setPoint2 - output);
}

void detectSigns(){ 
  uint16_t blocks = pixy.getBlocks();
    
  if (blocks > 0) {
    
    for (int i = 0; i < blocks; i++){
      if (pixy.blocks[i].y > 160) //Only detections above y = 180 are considered so the buggy doesnt react to signs prematurely 
               
        detections[pixy.blocks[i].signature - 1]++;

        if (detections[pixy.blocks[i].signature -1] >= minimumDetections){
              
            if(previousDetected != pixy.blocks[i].signature && previousDetected != pixy.blocks[i].signature + 3 && previousDetected != pixy.blocks[i].signature - 3 ){
              Serial.print("~11");
              Serial.println(pixy.blocks[i].signature);
              //moveCommand(pixy.blocks[i].signature +1);

              if(pixy.blocks[i].signature == 1) //Green
                baseSpeed = maxMotorSpeed;
              else if (pixy.blocks[i].signature == 2) //Blue
                baseSpeed = corneringSpeed;
              else if (pixy.blocks[i].signature == 3) //Green 2
                 baseSpeed = maxMotorSpeed;
              else if (pixy.blocks[i].signature == 4) //Blue 2
                baseSpeed = corneringSpeed;
    
               
              previousDetected = pixy.blocks[i].signature;
            }
            else if(previousDetected == -1)
              previousDetected = pixy.blocks[i].signature;  
               
            break; //Only one detection will be considered

        }     
      }        
    }
 }


