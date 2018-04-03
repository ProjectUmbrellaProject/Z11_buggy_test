#define speedPin 3
#define leftMotorDirection 4 //H-bridge setup is probably more complex than this.
#define leftEyePin 5
#define rightEyePin 0
#define rightMotorDirection 7

#define leftMotorMinus 4
#define leftMotorPlus 5
#define rightMotorPlus 6
#define rightMotorMinus 7
String inputString = "";
bool stringComplete, start;

const float K_d = 100, K_p = 1, setPoint; //K_p < K_d the derivative term must be large to have a significant influence
int maxMotorSpeed = 255, baseSpeed = 200;

unsigned long previousPingTime;
const short pingInterval = 400;
const short minimumDistance = 15; //Determines how close an object must be to stop the buggy
bool objectDetected;

void setup() {

    pinMode(leftEyePin, INPUT);
  pinMode(rightEyePin, INPUT);
    pinMode(leftMotorMinus, OUTPUT);
  pinMode(leftMotorPlus, OUTPUT);
  pinMode(rightMotorMinus, OUTPUT);
  pinMode(rightMotorPlus, OUTPUT);
  pinMode(speedPin, OUTPUT);
  
  Serial.begin(9600); // initiate serial commubnication at 9600 baud rate
  Serial.print("+++"); //Enter xbee AT commenad mode, NB no carriage return here
  delay(1500);  // Guard time
  Serial.println("ATID 3311, CH C, CN"); // PAN ID 3311
  delay(1100);
  while(Serial.read() != -1) {}; // get rid of OK
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
      Serial.println(readSensors() - 500);
      
    }
    digitalWrite(speedPin, HIGH);
    digitalWrite(rightMotorPlus, LOW);
    analogWrite(rightMotorMinus, 255);
    digitalWrite(leftMotorMinus, LOW);
    analogWrite(leftMotorPlus, 255);


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


int getSensorValue(){
  int leftEye = floor(analogRead(leftEyePin) / 50);
  int rightEye = floor(analogRead(rightEyePin) / 50);

  Serial.println(leftEye);
  Serial.println(rightEye);
  Serial.print("\n");

  int error;

 /*  if (leftEye > 950 && rightEye > 950){//if both eyes see black the buggy is no longer following the line. Should probably do something more sophisticated here
    start = false;

    return 0; //Bad practice to have a condition that leads to nothing being returned
  }
  else*/if (leftEye <= 650/50 && rightEye >= 650/50) //Buggy is positioned with the left eye on the edge of white and the right on black
    error = -(leftEye + rightEye);
  else if (leftEye >= 650/50 && rightEye <= 650/50) //Buggy is positioned with the right eye on the edge of white and the left on black
    error = (leftEye + rightEye);
  else
    error = (leftEye - rightEye);//Buggy is positioned with both eyes on white
  //Might be worth adding if (error > X) baseSpeed = corneringSpeed;
  
 /* if (leftEye > 50 || rightEye > 50{
    if (leftEye > rightEye)
      error = leftEye;
    else
      error = -rightEye;
    
  }
  else
    error = 0;
*/
  //if (error < 50 && error > -50)
  //  error = 0;
  /*Explanation:
   * In theory (leftEye - rightEye) should work for binary situations where one eye sees white and the other sees black.
   * E.g: leftEye = 30, rightEye = 1030, => output = -1000
   * However, when the buggy drifts further and one of the eyes is above the edge of white and the other is on black this approach fails because
   * the error now decreases instead of increasing.
   * E.g: leftEye = 530, rigtEye = 1030, => output = -500, Even thought the positioning is worse
   * The if statements above fix this by changing the output to 1560 for the example above.
   */
   
   return error;
  /*Explanation:
   * In theory (leftEye - rightEye) should work for binary situations where one eye sees white and the other sees black.
   * E.g: leftEye = 30, rightEye = 1030, => output = -1000
   * However, when the buggy drifts further and one of the eyes is above the edge of white and the other is on black this approach fails because
   * the error now decreases instead of increasing.
   * E.g: leftEye = 530, rigtEye = 1030, => output = -500, Even thought the positioning is worse
   * The if statements above fix this by changing the output to 1560 for the example above.
   */
 
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
  else if (leftEye > 500 && rightEye < 500) //If the buggy moves enough for the right eye to pass beyond white then the error should still be treated as if the right eye is seeing white
    //This condition is also true if the buggy 
    output = 35;

  return output;

  
}

