#define speedPin 3
#define leftMotorDirection 4
#define leftMotorSpeedPin 5
#define rightMotorSpeedPin 6
#define rightMotorDirection 7
#define eyeOne 5 //Originally left override
//#define eyeTwo to be determined
//#define eyeThree to be determined
#define eyeFour 0 //Originally right override
#define echoPin 8
#define trigPin 9

String inputString = "";
bool stringComplete, start;
//d = 0.15, p = 0.04  or  K_d = 0.11, K_p = 0.055, 
//most successsful value: Kd=0.11, kp = 0, basespeed = 240

const float K_d = 0.15, K_p = 0.04, setPoint = 2500; //K_p < K_d the derivative term must be large to have a significant influence
int maxMotorSpeed = 255, baseSpeed = 240;

unsigned long previousPingTime;
const short pingInterval = 400;
const short minimumDistance = 15; //Determines how close an object must be to stop the buggy
bool objectDetected;

    int rightMotorSpeed;
    int leftMotorSpeed;

void setup() {
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(leftMotorDirection, OUTPUT);
  pinMode(leftMotorSpeedPin, OUTPUT);
  pinMode(rightMotorDirection, OUTPUT);
  pinMode(rightMotorSpeedPin, OUTPUT);
  pinMode(speedPin, OUTPUT);
  pinMode(eyeOne, INPUT);
  pinMode(eyeFour, INPUT);
  
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
}

void loop() {
 
  if (stringComplete){
    int command = inputString.toInt();
    
      if (command == 0){
        start = false;
        Serial.println("~10");
      }
      else if (command == 1){
        start = true;
        Serial.println("~10");
      }

    inputString = "";
    stringComplete = false;
  }

  //Basic start/stop commands
  if (start){
    float previousError;
    float error = setPoint - getSensorValue();
  
    int motorSpeed = K_p * error + K_d * (error - previousError);
    previousError = error;
  
    rightMotorSpeed = baseSpeed + motorSpeed;
    leftMotorSpeed = baseSpeed - motorSpeed;

    unsigned long currentTime = millis(); //Update the time variable with the current time
    if (currentTime - previousPingTime >= pingInterval){

      Serial.print("left: ");
      Serial.print(leftMotorSpeed);
      Serial.print(" right: ");
      Serial.println(rightMotorSpeed);
      previousPingTime = currentTime; 
          
      handleObjectDetection();
    }

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
  getSensorValue(); //The sensor value must be updated as frequently as possible for this to work

}


float getSensorValue(){
  //Do something to combine the values of all IR sensors into one number
  int eyeOutputs[4];
  eyeOutputs[3] = analogRead(eyeOne);
  eyeOutputs[0] = analogRead(eyeFour);

  float k[2] = {3.125, 1.5635}; //Constants to constrain output within range 0 to 2*setpoint

  /*This assumes the output values correspond to the following conditions: 
   * Output < 50 Open air
   *  50 < Output < 300 Black
   *  300 < Output < 900 White
   */
 // float output = -k[0] * eyeOutputs[0] - k[1] * eyeOutputs[1] + k[1] * eyeOutputs[2] + k[0] * eyeOutputs[3] + setPoint;
  float output = -k[0] * eyeOutputs[0] + k[0] * eyeOutputs[3] + setPoint;
  return output;
 
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
    if (!objectDetected && distance <= minimumDistance && start){
      
      objectDetected = true; //The objectDetected boolean prevents the if statement from being repeatedly executed while the object is still present
      moveCommand(0);
     // forward = true; //Calling move command 0 also sets forward to false which is unintended in this case
      Serial.print("~6 ");
      Serial.println(distance);

    }
    else if (objectDetected && distance > minimumDistance && start){
        objectDetected = false;
        moveCommand(1);
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

void moveCommand(int command){

  if (command == 1){
    start = true;

  }
  else if (command == 0)
    start = false;



}

