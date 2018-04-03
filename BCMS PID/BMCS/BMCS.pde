import meter.*;
import controlP5.*;
import processing.serial.*;

Serial port; //Object to facilitate sending and receiving information via the Xbee
Meter motorOutput; //A voltmeter to display the current voltage applied to the motors
short currentMotorValue; //Variable to track the current motor speed (8 bit value)
boolean controlToggle; //Boolean to track the state of the buggy start/stop toggle
boolean moving; //Boolean to track whether or not the buggy is currently moving
boolean obstacleDetected; //Boolean to track if the buggy has detected an object with the ultrasonic sensor
ControlP5 cp5; //Object required for handling control p5 GUI elements
Textarea consoleArea; //A textarea used to display buggy commands on screen
float pGain,iGain, dGain; //Variables to store the current proportional and derivative gain
int baseSpeed; //Variable to store the base speed of the buggy with no error

void setup(){
  //Specifying the window dimensions, refresh rate and background colour
  frameRate(60);
  size(900, 720);
  background(255);
  
  //Initialising the global variables
  cp5 = new ControlP5(this);
  controlToggle = false;
  moving = false;
  obstacleDetected = false;
  currentMotorValue = 0;
  motorOutput = new Meter(this, 462, 460, true);
  
  //Reading the previous values from a txt and updating the respective global variables
  String settings[] = loadStrings("values.txt");
  pGain = Float.valueOf(settings[0].substring(2));
  iGain = Float.valueOf(settings[1].substring(2));
  dGain = Float.valueOf(settings[2].substring(2));
  baseSpeed = Integer.valueOf(settings[3].substring(10).trim());
 
  
  PImage[] imgs = {loadImage("Assets/go.png"),loadImage("Assets/stop.png")}; //Loading the images used for the start/stop toggle
  cp5.addToggle("controlToggle") //Adding the start stop toggle, specifying its initial state, position and dimensions
  .setValue(false)
  .setPosition(0, 620)
  .setImages(imgs)
  .setSize(200,100);
  
  //Adding a slider to adjust the proportional gain
  cp5.addSlider("P Gain")
   .setPosition(225,450)
   .setSize(200,20)
   .setRange(0,10)
   .setValue(pGain) //Setting the initial value of the slider to be the same as the current global variable
   .setTriggerEvent(Slider.RELEASE); //Setting the slider callback to occur only when the slider is released
  cp5.getController("P Gain").getCaptionLabel().setColor(color(75)); //Setting the colour of the caption to greyscale value 75
  cp5.getController("P Gain").getCaptionLabel().align(ControlP5.LEFT, ControlP5.TOP_OUTSIDE).setPaddingX(0); //Moving the caption so it appears in the top left above the slider
  
  //Adding a slider to adjust the integral gain
  cp5.addSlider("I Gain")
   .setPosition(225,520)
   .setSize(200,20)
   .setRange(0,1)
   .setValue(iGain) //Setting the initial value of the slider to be the same as the current global variable
   .setTriggerEvent(Slider.RELEASE);
  cp5.getController("I Gain").getCaptionLabel().setColor(color(75));
  cp5.getController("I Gain").getCaptionLabel().align(ControlP5.LEFT, ControlP5.TOP_OUTSIDE).setPaddingX(0);
   
  //Adding a slider to adjust the derivative gain
  cp5.addSlider("D Gain")
   .setPosition(225,590)
   .setSize(200,20)
   .setRange(0,50)
   .setValue(dGain) //Setting the initial value of the slider to be the same as the current global variable
   .setTriggerEvent(Slider.RELEASE);
  cp5.getController("D Gain").getCaptionLabel().setColor(color(75));
  cp5.getController("D Gain").getCaptionLabel().align(ControlP5.LEFT, ControlP5.TOP_OUTSIDE).setPaddingX(0);
  

  //Adding a slider to adjust the buggy's base speed
  cp5.addSlider("Base Speed")
   .setPosition(225,660)
   .setSize(200,20)
   .setRange(0,255)
   .setValue(baseSpeed) //Setting the initial value of the slider to be the same as the current global variable
   .setTriggerEvent(Slider.RELEASE);
  cp5.getController("Base Speed").getCaptionLabel().setColor(color(75));
  cp5.getController("Base Speed").getCaptionLabel().align(ControlP5.LEFT, ControlP5.TOP_OUTSIDE).setPaddingX(0);
  

  
  //Adding the textarea used to display the console information
  consoleArea = cp5.addTextarea("console")
    .setPosition(0,0)
    .setSize(900,400)
    .setFont(createFont("arial",18))
    .setLineHeight(18)
    .setColor(color(75))
    .setColorBackground(color(200))
    .setColorForeground(color(255,100))
    .scroll(1).showScrollbar(); //The textarea will automatically scroll when a new string is appended         

  //A basic check to ensure that there is at least 1 COM device connected (which is most likely to be the Xbee)
  if (Serial.list().length > 0){
    //Initialising the port variable and running the AT commands to establish the Xbee connection
    String portName = Serial.list()[0];
    port = new Serial(this, portName, 9600);
    port.write("+++");
    delay(1100);
    port.write("ATID 3311, CH C, CN");
    delay(1100);
    port.bufferUntil( 10 );
    
  }
  else{
    print("Warning: No Xbee detected"); //If there are no COM devices detected then the Xbee cannot be connected
    consoleArea.append("Warning: No Xbee detected\n"); //Message must also appear in the console textarea
  }
   
}


void draw(){
  
  if (obstacleDetected) //If an obstacle is detected inform the user on screen
      image(loadImage("Assets/obstacleDetected.png"), 0, 410);
  else if (moving) //If the buggy is moving illustrate this on screen
      image(loadImage("Assets/forward.png"), 0, 410);
  else if (!moving) //If the buggy is stopped illustrate this on screen
      image(loadImage("Assets/stopped.png"), 0, 410);
  
    motorOutput.updateMeter(baseSpeed); //Update the value of the meter with the current base speed (even if it's unchanged)

}

//Function for handling serial communication
void serialEvent(Serial p){
   String receivedString = p.readString();
   
   //If the string starts with ~ it contains useful information
   if (receivedString.charAt(0) == '~'){
       //printCommandInformation(receivedString);
       //Pass the information to other functions to be analysed and eventually inform the user
       commandInterpreter(receivedString);      
       print(receivedString);
   }
   else{
     print(receivedString);
     //If the buggy has just started make sure its gain values and base speed agree with the values stored in the global variables
     if ((receivedString.trim()).equals("Buggy: Setup Complete.")){
         port.write("/4 " + nf(pGain, 1, 4) + "\n");
         port.write("/3 " + nf(iGain, 1, 4) + "\n"); 
         port.write("/5 " + nf(dGain, 6, 3) + "\n");                                   
         port.write("/2 " + baseSpeed + "\n");
     }

     
     if (millis() > 3000) //If this line is executed before the window has opened (which may take several seconds) it will result in an error
       consoleArea.append(receivedString + "\n");
   }
   
   p.clear(); //Clear the buffer for the next string
   
 
}

//Function to extract information from newly received commands and update respective global variables
void commandInterpreter(String command){
  //Make sure the command is information bearing before trying to extract information (command list specifies that commands beginning with ~ are information bearing)
  if (command.charAt(0) == '~'){
    //Commands are 2 digit numbers so only the 2nd and 3rd characters in the string should be considered
      switch (command.substring(1,3).trim()){
        //6: Obstacle detected
        case "6":
          obstacleDetected = true;
          moving = false;
          
        break;

        //8: Motor power set to XX
        case "8":
          currentMotorValue = Short.valueOf((command.substring(3)).trim());
          
        break;
            
        //9: Move command confirmation
        case "9":
          obstacleDetected = false;
          moving = true;
          
        break;
        
        //10: Stop command confirmation
        case "10":
          obstacleDetected = false;
          moving = false; 
          
         break;

        //20: Unknown command
        case "20":
          print("Unrecognised command recieved by buggy");
          consoleArea.append("Unrecognised command recieved by buggy\n");
        break;
           
    }
  }
}

public void controlEvent(ControlEvent theEvent){
  //This if statement is required to prevent an error occuring on launch
  //Prevent the cp5 event handler from being called before the UI elements have been fully initialised
  if (millis() > 2000){
    
    //The name of the UI element that caused the even can be used to determine how the program should respond
    switch (theEvent.getController().getName()){
      
      case "controlToggle":
        if (controlToggle)
          port.write("/1 \n"); //Send the start command
        else
          port.write("/0 \n"); //Send the stop command
        
        controlToggle = !controlToggle;
      
      break;
      
      case "P Gain":
        pGain = theEvent.getController().getValue();
        port.write("/4 " + nf(pGain, 1, 4) + "\n"); //Send a command to update the p gain on the buggy
        println(nf(pGain, 1, 4));
        
      break;
      
      case "D Gain":
        dGain = theEvent.getController().getValue();
        port.write("/5 " + nf(dGain, 6, 3) + "\n"); //Send a command to update the d gain on the buggy
        println(nf(dGain, 6 , 3));
      break;
      
      case "Base Speed":
        baseSpeed = floor(theEvent.getController().getValue());
        port.write("/2 " + baseSpeed + "\n"); //Send a command to update the base speed on the buggy
        println(baseSpeed);
      
      break;
      
      
      case "I Gain":
        iGain = theEvent.getController().getValue();
        port.write("/3 " + nf(iGain, 1, 4) + "\n"); //Send a command to update the i gain on the buggy
        println(nf(pGain, 1, 4));
        
        
      break;

    }
  }
  
}

//Event handler for key presses
void keyPressed(){
  switch (key){
    //The state of the start/stop toggle can also be toggled by pressing the space bar (ASCII 32)
    case 32:
      float temp; //The setValue function accepts a float argument to change the state of the UI element
      if (cp5.getController("controlToggle").getValue() == 0) //If the current state is 0 change it to 1, if it's 1 change it to 0
        temp = 1;    
      else
        temp = 0;
      //Note: the command to start/stop the buggy does not need to sent here because toggling the toggle will result in controlEvent being called
      cp5.getController("controlToggle").setValue(temp);
  
    break;
  }
  
}

//Function to convert the 'encoded' communication between the buggy and monitoring program into understandable messages printed in the console
//This functionality could be integrated in the commandInterpreter function, however, writing this as a seperate function allows its use to be toggled on and off by
//commenting out line 135. This can be useful when debugging new features.
void printCommandInformation(String command){
  switch (command.substring(1, 3).trim()){
    case "6":
      println("Obstacle detected at " + command.substring(3).trim() + " cm");
      consoleArea.append("Obstacle detected at " + command.substring(3).trim() + " cm\n");
    break;
    
    case "7":
      println("Value Updated");
      consoleArea.append("Value Updated\n");
    break;
    
    case "8":
      println("Motor power: " + (command.substring(3)).trim());
    break;
    
    case "9":
      println("Start command received");
      consoleArea.append("Start command received\n");
    break;
    
    case "10":
      println("Stop command received");
      consoleArea.append("Stop command received\n");
    break;
    

  }
  
}

//This function is called when the program exits to ensure that the current global variables are stored
void exit(){
  String PIDValues[] = {"P " + pGain, "I " + iGain, "D " + dGain, "Base Speed " + baseSpeed};
  saveStrings("values.txt",  PIDValues);
  super.exit();
}