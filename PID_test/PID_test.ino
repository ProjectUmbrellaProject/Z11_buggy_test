#define leftMotor 4 //H-bridge setup is probably more complex than this.
#define rightMotor 4

String inputString = "";
bool stringComplete, start;

const float K_d = 100, K_p = 1, setPoint; //K_p < K_d the derivative term must be large to have a significant influence
int maxMotorSpeed = 255, baseSpeed = 200;

void setup() {
  pinMode(leftMotor, OUTPUT);
  pinMode(rightMotor, OUTPUT);
  
  analogWrite(leftMotor, 0);
  analogWrite(rightMotor, 0);

  stringComplete = false;
  start = false;

}

void loop() {
  if (stringComplete){
    
    if (inputString == "/0")
      start = false;
    else if (inputString == "/1")
      start = true;

    inputString = "";
    stringComplete = false;
  }

  //Basic start/stop commands
  if (start){
    float previousError;
    float error = setPoint - getSensorValue();
  
    int motorSpeed = K_p * error + K_d * (error - previousError);
    previousError = error;
  
    int rightMotorSpeed = baseSpeed + motorSpeed;
    int leftMotorSpeed = baseSpeed - motorSpeed;
  
    //Constrain motors to a range of output between 0 and maxMotorSpeed
    if (rightMotorSpeed > maxMotorSpeed)
      rightMotorSpeed = maxMotorSpeed;
    else if (rightMotorSpeed < 0) //Consider changing this to run the motor in reverse instead
      rightMotorSpeed = 0;
      
    if (leftMotorSpeed > maxMotorSpeed)
      leftMotorSpeed = maxMotorSpeed;
    else if (leftMotorSpeed < 0)
      leftMotorSpeed = 0;
  
    analogWrite(leftMotor, leftMotorSpeed);
    analogWrite(rightMotor, rightMotorSpeed);
  }

}

float getSensorValue(){
  //Do something to combine the values of all IR sensors into one number

  return 1000;

  
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
