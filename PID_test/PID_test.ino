#define speedPin 3
#define leftMotorDirection 4
#define leftMotorSpeedPin 5
#define rightMotorSpeedPin 6
#define rightMotorDirection 7
#define eyeOne 5 //Originally left override
//#define eyeTwo to be determined
//#define eyeThree to be determined
#define eyeFour 6 //Originally right override

String inputString = "";
bool stringComplete, start;

const float K_d = 100, K_p = 1, setPoint = 2500; //K_p < K_d the derivative term must be large to have a significant influence
int maxMotorSpeed = 255, baseSpeed = 200;

void setup() {
  pinMode(leftMotorDirection, OUTPUT);
  pinMode(leftMotorSpeedPin, OUTPUT);
  pinMode(rightMotorDirection, OUTPUT);
  pinMode(rightMotorSpeedPin, OUTPUT);
  pinMode(speedPin, OUTPUT);
  
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
  
      digitalWrite(speedPin, HIGH);
      digitalWrite(rightMotorDirection, LOW);
      analogWrite(rightMotorSpeedPin, rightMotorSpeed);
      digitalWrite(leftMotorDirection, LOW);
      analogWrite(leftMotorSpeedPin, leftMotorSpeed);
  }
  getSensorValue(); //The sensor value must be updated as frequently as possible for this to work
}


float getSensorValue(){
  //Do something to combine the values of all IR sensors into one number
  int eyeOutputs[4];
  eyeOutputs[0] = analogRead(eyeOne);
  eyeOutputs[3] = analogRead(eyeFour);

  float k[2] = {3.125, 1.5635}; //Constants to constrain output within range 0 to 2*setpoint

  /*This assumes the output values correspond to the following conditions: 
   * Output < 50 Open air
   *  50 < Output < 300 Black
   *  300 < Output < 900 White
   */
  float output = -k[0] * eyeOutputs[0] - k[1] * eyeOutputs[1] + k[1] * eyeOutputs[2] + k[0] * eyeOutputs[3] + setPoint;
  
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
