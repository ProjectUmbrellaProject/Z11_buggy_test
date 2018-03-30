#define speedPIn 3
#define leftMotorDirection 4 //H-bridge setup is probably more complex than this.
#define leftEyePin 5
#define rightEyePin 0
#define rightMotorDirection 7
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
      Serial.println(getSensorValue());
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


