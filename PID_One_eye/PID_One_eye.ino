//Defining all the pins used
#define speedPin 3
#define leftMotorMinus 4
#define leftMotorPlus 5
#define rightMotorPlus 6
#define rightMotorMinus 7
#define leftEyePin 5
#define rightEyePin 0
#define echoPin 8
#define trigPin 9

//Variable to handle Xbee communication and store the most recent command
String inputString = "";
bool stringComplete, start, startCommand;

//PID variables
//Setpoint must be half of black value
float K_d = 19, K_i = 0.0, K_p = 0.75, setPoint = 400; //K_p < K_d the derivative term must be large to have a significant influence
int maxMotorSpeed = 255, baseSpeed = 255;
int rightMotorSpeed;
int leftMotorSpeed;
int previousError;
int integral;
  int difference ;

//Variables used for object detection
unsigned long previousPingTime;
unsigned long previousTime;
const short pingInterval = 200;
const short minimumDistance = 10; //Determines how close an object must be to stop the buggy
bool objectDetected;



void setup() {
  //Defining input and output pins
  pinMode(echoPin, INPUT);
  pinMode(leftEyePin, INPUT);
  pinMode(rightEyePin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(leftMotorMinus, OUTPUT);
  pinMode(leftMotorPlus, OUTPUT);
  pinMode(rightMotorMinus, OUTPUT);
  pinMode(rightMotorPlus, OUTPUT);
  pinMode(speedPin, OUTPUT);

  //Writing to the motor pins the buggy is stationary
  analogWrite(speedPin, 0);
  analogWrite(rightMotorPlus, 0);
  analogWrite(rightMotorMinus, 0);
  analogWrite(leftMotorMinus, 0);
  analogWrite(leftMotorPlus, 0);

  //Establishing the Xbee connection and running the AT commands
  Serial.begin(9600); 
  Serial.print("+++"); 
  delay(1500);
  Serial.println("ATID 3311, CH C, CN");
  delay(1100);
  while(Serial.read() != -1) {};
    inputString.reserve(200);

  //Send status to monitoring program
  Serial.println("Buggy: Setup Complete.");

  //Initialise global variables
  stringComplete = false;
  start = false;
  integral = 0;
  previousTime = millis();
}

void loop() {

   
  if (stringComplete){
    moveCommand(inputString);
    
    inputString = "";
    stringComplete = false;
  }

  unsigned long currentTime2 = millis(); //Update the time variable with the current time
  //Data from the ultrasonic sensor is read periodically, where the period between readings is defined by pingInterval
  if (currentTime2 - previousPingTime >= pingInterval){
    previousPingTime = currentTime2; 
        
    handleObjectDetection();

  }

  //If the flag to start has been set to true by the command function then begin PID control
  if (start){
    int eyeOutput = readSensors();
    unsigned long currentTime = 1;
    int error = eyeOutput - setPoint; //The setpoint is the reading we are aiming to achieve
    integral += error * (currentTime - previousTime); //Add the error to the integral
    bool leftReverse = false, rightReverse = false;
    
    
    //difference = (currentTime - previousTime);
    difference = 1;
    int motorDifference = K_p * error + K_i * integral + K_d * ((error - previousError)/difference); //Update the motor speed in proportion to the current error, and approximate rate of change of error
    previousError = error; //Storing the current error for the next loop
    //previousTime = currentTime;

    //Updating the motor speeds using the new motor speed
    rightMotorSpeed = baseSpeed + motorDifference;
    leftMotorSpeed = baseSpeed - motorDifference;
 
    if (rightMotorSpeed < 0){ //Consider changing this to run the motor in reverse instead
      rightMotorSpeed = -rightMotorSpeed;
      digitalWrite(rightMotorMinus, HIGH);
    }
    else
      digitalWrite(rightMotorMinus, LOW);

    if (leftMotorSpeed < 0){
      digitalWrite(leftMotorMinus, HIGH);
     leftMotorSpeed = -leftMotorSpeed;
    }
    else 
      digitalWrite(leftMotorMinus, LOW);
    
    //Constrain motors to a range of output between 0 and maxMotorSpeed
    if (rightMotorSpeed > maxMotorSpeed){
      rightMotorSpeed = maxMotorSpeed;
    }
    else if (rightMotorSpeed < 0)
      rightMotorSpeed = 0;
      
    if (leftMotorSpeed > maxMotorSpeed)
      leftMotorSpeed = maxMotorSpeed;
    else if (leftMotorSpeed < 0)
      leftMotorSpeed = 0;


    
    analogWrite(speedPin, 255);
    analogWrite(leftMotorPlus, leftMotorSpeed);
    analogWrite(rightMotorPlus, rightMotorSpeed);


   
      
  }else{
    //If the command to move has not been received ensure the buggy is stationary
    analogWrite(rightMotorPlus, 0);
    analogWrite(rightMotorMinus, 0);
    analogWrite(leftMotorMinus, 0);
    analogWrite(leftMotorPlus, 0);

  }


}

void serialEvent() {
  //If there is new information in the buffer
  while (Serial.available()) {
    char inChar = (char)Serial.read(); //Read the character from the buffer

    inputString += inChar; //Add the character to the string

    //If the character is the endline character then the end of the command has been reached
    if (inChar == '\n') {
      stringComplete = true; //The command can now be passed to the command function to be interpretted
    }
  }
}

//This function combines the output of the two eyes into a value which can be used to determine the buggy's position
int readSensors(){
  //Read and store the current output of both eyes
  int rightEye = analogRead(rightEyePin);
  int leftEye = analogRead(leftEyePin);
  int output;
  //Negative on the right, positive on the left

  /*This method uses both eyes to determine the position of the buggy but only one eye is used to measure the error.
   * By using a setpoint which tries to keep the eye on the edge of black and white the 'deadzone' in the measurment is reduced.
   * Ideally this method should keep the right eye above the edge of the line at all times. This means that any small
   * movement will result in the eye crossing the line and will have an instantaneous effect on the error. However,
   * this method is limited because it cannot effectively determine the position of the buggy. By using the second eye to 
   * verify measuremens fromt first eye, this method is made more reliable.
   */
  //If the left eye is above white
  //Buggy is on the right side of the line
  if (leftEye < 100){
    output = rightEye; 
  }
  else if (leftEye > 500 && rightEye < 500 && previousError + setPoint < 120) //If the buggy moves enough for the right eye to pass beyond white then the error should still be treated as if the right eye is seeing white
    //This condition is also true if the buggy 
    output = 35;

  else if (previousError + setPoint > 700)
    output = 1000;

    
  return output;

  
}
void handleObjectDetection(){
    long distance = obstacleDistance();

    //The command to stop should only be called if the control program has told the buggy to move and an object hasnt already been detected
    //i.e. the buggy isnt already stationary
    //This if statement stops the buggy if: an object has been detected within the range specified by minimumDistance, an object has not already been detected,
    //and the buggy is already moving (it has received the command to move)
    if (!objectDetected && (distance <= minimumDistance && distance != 0) && startCommand){
      
      objectDetected = true; //The objectDetected boolean prevents the if statement from being repeatedly executed while the object is still present
      moveCommand("/0 ");
      startCommand = true;
     // forward = true; //Calling move command 0 also sets forward to false which is unintended in this case
      Serial.print("~6 ");
      Serial.println(distance);

    }
    //If an object was detected but is no longer present then start again
    else if (objectDetected && (distance > minimumDistance || distance == 0) && startCommand){
        objectDetected = false;
        moveCommand("/1 ");
    }
}

//This function returns the distance to the nearest object as measured by the ultrasonic sensor
long obstacleDistance(){
  long distance, duration;

  //Clear the trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  //Send out a signal for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  //Read the time taken for the echo to return
  duration = pulseIn(echoPin, HIGH, 2000);
  
  //Calculate the distance using the speed of sound
  distance = round(duration*0.034/2);

  return distance;

  
}

//Function to interpret command and modify the arduino outputs or global variables as required
void moveCommand(String command){
  int commandNum = (command.substring(1, 3)).toInt(); //Commands are 2 digit integers

  switch (commandNum){
    //Stop command
    case 0:
      start = false;
      startCommand = false;
      Serial.println("~10"); //Report back to monitoring program to confirm the command was received and executed
    break;

    //Start command
    case 1:
     start = true;
     startCommand = true;
     Serial.println("~9 "); //Report back to monitoring program to confirm the command was received and executed
    break;

    //Change base speed
    case 2:
      baseSpeed = (command.substring(3)).toInt();
      Serial.println("~7 "); //Report back to monitoring program to confirm the command was received and executed
      Serial.println((command.substring(3)).toInt());
    break;

    //Change I gain
    case 3:
      K_i = (command.substring(3)).toFloat();
      Serial.println("~7 "); //Report back to monitoring program to confirm the command was received and executed
      Serial.println((command.substring(3)).toFloat());
    break;

    //Change P gain
    case 4:
      K_p = (command.substring(3)).toFloat();
      Serial.println("~7 "); //Report back to monitoring program to confirm the command was received and executed
      Serial.println((command.substring(3)).toFloat());
    break;

    //Change D gain
    case 5:
      K_d = (command.substring(3)).toFloat();
      Serial.println("~7 "); //Report back to monitoring program to confirm the command was received and executed
      Serial.println((command.substring(3)).toFloat());
    break;

    default:
      Serial.println("~20"); //Command was not recognised
    break;
  }

}

