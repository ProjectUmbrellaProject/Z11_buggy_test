import meter.*;
import controlP5.*;
import processing.serial.*;

Serial port;
Meter motorOutput;
int counter = 0, currentMotorValue, previousTime, currentDetection;
boolean controlToggle, moving, obstacleDetected, pressedToggle;
ControlP5 cp5;

Textarea consoleArea;
int lineCount = 0;

void setup(){
  frameRate(60);
  size(900, 720);
  background(255);
  cp5 = new ControlP5(this);
  controlToggle = false;
  moving = false;
  obstacleDetected = false;
  pressedToggle = false;
  currentMotorValue = 0;
  currentDetection = 0;
  
  motorOutput = new Meter(this, 462, int(pixelHeight*0.64), true);
  
  PImage[] imgs = {loadImage("Assets/go.png"),loadImage("Assets/stop.png")};
  cp5.addToggle("controlToggle")
  .setValue(false)
  .setPosition(0, 620)
  .setImages(imgs)
  .setSize(200,100);
  
  cp5.addSlider("P Gain")
   .setPosition(225,450)
   .setSize(200,20)
   .setRange(0.0001,1)
   .setValue(0.25)
   .setTriggerEvent(Slider.RELEASE)
   ;
  cp5.getController("P Gain").getCaptionLabel().setColor(color(75));
  cp5.getController("P Gain").getCaptionLabel().align(ControlP5.LEFT, ControlP5.TOP_OUTSIDE).setPaddingX(0);
   
  cp5.addSlider("D Gain")
   .setPosition(225,520)
   .setSize(200,20)
   .setRange(0.01,500)
   .setValue(1)
   .setTriggerEvent(Slider.RELEASE)
   ;
  cp5.getController("D Gain").getCaptionLabel().setColor(color(75));
  cp5.getController("D Gain").getCaptionLabel().align(ControlP5.LEFT, ControlP5.TOP_OUTSIDE).setPaddingX(0);
  
  cp5.addSlider("Base Speed")
   .setPosition(225,590)
   .setSize(200,20)
   .setRange(0,255)
   .setValue(240)
   .setTriggerEvent(Slider.RELEASE)
   ;
  cp5.getController("Base Speed").getCaptionLabel().setColor(color(75));
  cp5.getController("Base Speed").getCaptionLabel().align(ControlP5.LEFT, ControlP5.TOP_OUTSIDE).setPaddingX(0);
  
  cp5.addSlider("Cornering Speed")
   .setPosition(225,660)
   .setSize(200,20)
   .setRange(0,255)
   .setValue(230)
   .setTriggerEvent(Slider.RELEASE)
   ;
  cp5.getController("Cornering Speed").getCaptionLabel().setColor(color(75));
  cp5.getController("Cornering Speed").getCaptionLabel().align(ControlP5.LEFT, ControlP5.TOP_OUTSIDE).setPaddingX(0);
  
  
  
  consoleArea = cp5.addTextarea("console")
                  .setPosition(0,0)
                  .setSize(900,400)
                  .setFont(createFont("arial",18))
                  .setLineHeight(18)
                  .setColor(color(75))
                  .setColorBackground(color(200))
                  .setColorForeground(color(255,100))
                  .scroll(1).showScrollbar();               
  
  if (Serial.list().length > 0){
    
    String portName = Serial.list()[0];
    port = new Serial(this, portName, 9600);
    port.write("+++");
    delay(1100);
    port.write("ATID 3311, CH C, CN");
    delay(1100);
    port.bufferUntil( 10 );
    
  }
  else{
    print("Warning: No Xbee detected");
    consoleArea.append("Warning: No Xbee detected\n");
  }
  
  previousTime = millis();
  
  
}


void draw(){
  
  if (obstacleDetected)
      image(loadImage("Assets/obstacleDetected.png"), 0, 410);
  else if (moving)
      image(loadImage("Assets/forward.png"), 0, 410);
  else if (!moving)
      image(loadImage("Assets/stopped.png"), 0, 410);
  
    motorOutput.updateMeter(currentMotorValue);

}

void serialEvent(Serial p){
   String receivedString = p.readString();
   
   //If the string starts with ~ it contains useful information
   if (receivedString.charAt(0) == '~'){
       //printCommandInformation(receivedString);
       commandInterpreter(receivedString);      
       print(receivedString);
   }
   else{
     print(receivedString);
     if (millis() > 3000)
       consoleArea.append(receivedString + "\n");
   }
   counter++; //During the initial startup the serial event callback will be called 9 times. The counter allows these calls to be ignored

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
          consoleArea.append("Unrecognised command recieved by buggy\n");
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
         port.write("/1 \n");

        }
          
        else{
         port.write("/0 \n");
        }
        
         controlToggle = !controlToggle;
      
        break;
      case "P Gain":
        float pGain = theEvent.getController().getValue();
        port.write("/4 " + nf(pGain, 1, 4) + "\n");
        println(nf(pGain, 1, 4));
        
      break;
      
      case "D Gain":
        float dGain = theEvent.getController().getValue();
        port.write("/5 " + nf(dGain, 2, 3) + "\n");
        println(nf(dGain, 2, 3));
      break;
      
      case "Base Speed":
        int baseSpeed = floor(theEvent.getController().getValue());
        port.write("/2 " + baseSpeed + "\n");
        println(baseSpeed);
      
      break;
      
      
      case "Cornering Speed":
        int corneringSpeed = floor(theEvent.getController().getValue());
        port.write("/3 " + corneringSpeed + "\n");
        println(corneringSpeed);
        
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
    
    case "11":
    
      print("Detected colour: ");
      consoleArea.append("Detected colour: ");
      int colourId = Integer.valueOf((command.substring(3)).trim());
        if (colourId == 2 || colourId == 4){
          println("Blue");
          consoleArea.append("Blue\n");
        }
        else if (colourId == 1 || colourId == 3){
          println("Green");
          consoleArea.append("Green\n");
        }
        
      break;

  }
  
}