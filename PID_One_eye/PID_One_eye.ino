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
bool stringComplete, start, startCommand;

//PID variables
//Setpoint must be half of black value
float K_d = 1, K_p = 0.5, setPoint = 500; //K_p < K_d the derivative term must be large to have a significant influence
int maxMotorSpeed = 255, baseSpeed = 255, corneringSpeed = 230;
int rightMotorSpeed;
int leftMotorSpeed;
int previousError;

unsigned long previousPingTime;
const short pingInterval = 200;
const short minimumDistance = 15; //Determines how close an object must be to stop the buggy
bool objectDetected;



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
}

void loop() {

  unsigned long currentTime = millis(); //Update the time variable with the current time
  if (currentTime - previousPingTime >= pingInterval){
    previousPingTime = currentTime; 
        
    handleObjectDetection();
  }

  if (stringComplete){
    moveCommand(inputString);
    
    inputString = "";
    stringComplete = false;
  }

  //Basic start/stop commands
  if (start){
    int eyeOutput = readSensors();

    int error = eyeOutput - setPoint;
  
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

int readSensors(){
  int rightEye = analogRead(rightEyePin);
  int leftEye = analogRead(leftEyePin);
  int output;
  //Negative on the right, positive on the left
  
  if (leftEye < 100){
    output = rightEye; 
  }
  else //If the buggy moves enough for the right eye to pass beyond white then the error should still be treated as
  //if the right eye is seeing white
    output = 35;

  
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

