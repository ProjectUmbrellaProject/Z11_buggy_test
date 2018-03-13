#define speedPIn 3
#define leftMotorDirection 4 //H-bridge setup is probably more complex than this.
#define leftMotorSpeed 5
#define rightMotorSpeed 6
#define rightMotorDirection 7
String inputString = "";
bool stringComplete, start;

const float K_d = 100, K_p = 1, setPoint; //K_p < K_d the derivative term must be large to have a significant influence
int maxMotorSpeed = 255, baseSpeed = 200;

void setup() {
  pinMode(leftMotorDirection, OUTPUT);
  pinMode(leftMotorSpeed, OUTPUT);
  pinMode(rightMotorDirection, OUTPUT);
  pinMode(rightMotorSpeed, OUTPUT);
  pinMode(speedPIn, OUTPUT);
  
      digitalWrite(speedPIn, LOW);
      analogWrite(rightMotorSpeed, 0);
      analogWrite(rightMotorDirection, 0);
      analogWrite(leftMotorDirection, 0);
      analogWrite(leftMotorSpeed, 0);

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
  if (stringComplete){

    int command = inputString.toInt();
    
    if (command == 0){
      digitalWrite(speedPIn, LOW);
      analogWrite(rightMotorSpeed, 0);
      analogWrite(rightMotorDirection, 0);
      analogWrite(leftMotorDirection, 0);
      analogWrite(leftMotorSpeed, 0);
      Serial.println("Stop");

    }
    
    else if (command == 1){
      digitalWrite(speedPIn, HIGH);
         // analogWrite(rightMotorDirection, 0);
          digitalWrite(rightMotorDirection, LOW);
           analogWrite(rightMotorSpeed, 255);
  
 

         //   analogWrite(leftMotorDirection, 0);
            digitalWrite(leftMotorDirection, LOW);
      analogWrite(leftMotorSpeed, 100);



      Serial.println("Start");
    } 
    

    inputString = "";
    stringComplete = false;
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
