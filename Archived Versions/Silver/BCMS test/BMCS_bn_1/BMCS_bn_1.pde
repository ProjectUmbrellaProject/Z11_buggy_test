import meter.*;
import controlP5.*;
import processing.serial.*;

Serial port;
Meter motorOutput;
int counter = 0, currentMotorValue, previousTime, currentDetection;
boolean controlToggle, moving, obstacleDetected, pressedToggle;
ControlP5 cp5;

void setup(){
  frameRate(60);
  size(1280, 720);
  background(255);
  cp5 = new ControlP5(this);
  controlToggle = false;
  moving = false;
  obstacleDetected = false;
  pressedToggle = false;
  currentMotorValue = 0;
  currentDetection = 0;
  
  motorOutput = new Meter(this, int(pixelWidth*0.66), int(pixelHeight*0.64), true);
  
  PImage[] imgs = {loadImage("Assets/go.png"),loadImage("Assets/stop.png")};
  cp5.addToggle("controlToggle")
  .setValue(false)
  .setPosition(500, 600)
  .setImages(imgs)
  .setSize(200,100);
  


  if (Serial.list().length > 0){
    
    String portName = Serial.list()[0];
    port = new Serial(this, portName, 9600);
    port.write("+++");
    delay(1100);
    port.write("ATID 3311, CH C, CN");
    delay(1100);
    port.bufferUntil( 10 );
    
  }
  else
    print("Warning: No Xbee detected");
  
  previousTime = millis();
  
}


void draw(){
  image(loadImage("Assets/map.png"), 0, 0);
  
  //The map elements flash every half second
  if (millis() - previousTime < 500){
    highLightLocation();
  }
  else if (millis() - previousTime > 500 && millis() - previousTime < 1000){
    image(loadImage("Assets/map.png"), 0, 0);

  }
  else if (millis() - previousTime > 1000)
    previousTime = millis();

  
  if (obstacleDetected)
      image(loadImage("Assets/obstacleDetected.png"), 0, 520);
  else if (moving)
      image(loadImage("Assets/forward.png"), 0, 520);
  else if (!moving)
      image(loadImage("Assets/stopped.png"), 0, 520);
  
    motorOutput.updateMeter(currentMotorValue);

}

void serialEvent(Serial p){
   String receivedString = p.readString();
   
   //If the string starts with ~ it contains useful information
   if (receivedString.charAt(0) == '~'){
       printCommandInformation(receivedString);
       commandInterpreter(receivedString);          
   }
   else
     print(receivedString);


   counter++; //During the initial startup the serial event callback will be called 9 times. The counter allows these calls to be ignored
  /* 
   if (counter > 9)    
         commandInterpreter(receivedString);
         */  
   p.clear();
   
 
}


void commandInterpreter(String command){
  switch (command.charAt(0)){
    
    case '~':
      switch (command.substring(1,3).trim()){
        //6: Obstacle detected
        case "6":
          obstacleDetected = true;
          moving = false;
          
        break;
        
        //7: Gantry XX detected
        case "7":
          currentDetection = Integer.valueOf((command.substring(3)).trim());     
          
        break;
        
        //8: Motor power set to XX
        case "8":
          currentMotorValue = Integer.valueOf((command.substring(3)).trim());
          
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
         
        //11: Detected colour ID XX
        case "11":

          int colourId = Integer.valueOf((command.substring(3)).trim());
          if (colourId == 1 || colourId == 4){
            
            if (currentDetection == 3)
              currentDetection = 5;
            else if (currentDetection == 6)
              currentDetection = 7;
              
          } else if (colourId == 2 || colourId == 5){
              if (currentDetection == 5)
                currentDetection = 6;
              else if (currentDetection == 2)
                currentDetection = 6;
              else if (currentDetection == 7)
                currentDetection = 8;
                
          } else if (colourId == 3)
              currentDetection = 4;
        break;
        
        //20: Unknown command
        case "20":
          print("Unrecognised command recieved by buggy");
        break;
           
  }

    
  break;
  }
}

public void controlEvent(ControlEvent theEvent){
  //Getting sick of that annoying error on launch...
  if (millis() > 2000){
    switch (theEvent.getController().getName()){
      case "controlToggle":
         pressedToggle = true;
                     
        if (controlToggle){ 
         port.write("1 \n");

        }
          
        else{
         port.write("0 \n");
        }
        
         controlToggle = !controlToggle;
      
        break;

    }
  }
  
}

void keyPressed(){
  switch (key){
    case 32:
      float temp;
      if (cp5.getController("controlToggle").getValue() == 0){
        temp = 1;
      }
      else{
        temp = 0;
      }
      cp5.getController("controlToggle").setValue(temp);
  
    break;
  }
  
}

void highLightLocation(){
  
      switch (currentDetection){
      case 1:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(200, 7, 120, 65, 10);
        break;
      
      case 2:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(696, 7, 100, 65, 10);
        
        break;
        
      case 3:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(618, 132, 70, 70, 10);
        
        break;
        
      case 4:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(520, 20, 125, 45, 10);
        
        break;
        
      case 5:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(715, 248, 90, 73, 10);
        
        break;
        
      case 6:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(575, 452, 135, 40, 10);
        
        break;
        
      case 7:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(228, 452, 125, 40, 10);
        
        break;
        
      case 8:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(8, 355, 90, 72, 10);
        
        break;
     
      
    }
}


void printCommandInformation(String command){
  switch (command.substring(1, 3).trim()){
    case "6":
      println("Obstacle detected at " + command.substring(3).trim() + " cm");
      
    break;
    
    case "7":
      println("Detected gantry #: " + (command.substring(3)).trim());
    break;
    
    case "8":
      println("Motor power: " + (command.substring(3)).trim());
    break;
    
    case "9":
      println("Start command received");
    break;
    
    case "10":
      println("Stop command received");
    break;
    
    case "11":
    
      print("Detected colour: ");
      int colourId = Integer.valueOf((command.substring(3)).trim());
        if (colourId == 1 || colourId == 4)
          println("Blue");
        else if (colourId == 2 || colourId == 5)
          println("Green");
        else if (colourId == 3)
          println("Yellow");

      break;
      
    case "12":
      println("Did not recognise gantry number");
      break;

  }
  
}