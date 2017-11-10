#include <stdio.h>
#include<EEPROM.h>

#include "HX711.h" //load cell amplifier library
#define DOUT  A1
#define CLK  A0
HX711 scale(DOUT, CLK);   
float calibration_factor = 1320; //1935 previously

/*for 0.96" OLED display ssd1306 clone from ebay */
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 4  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);


String modeLookup[3] = { "animation", "measuring", "saving" }; 
int mode;

//variables to hold scores by lookup
float gripForceHS[15];
char firstNameHS[15][15];
int sexHS[15];


//using a struct so it can be saved to EEPROM easily
struct gripperProfile {
  float gripForce; //how strong they are  
  char firstName[15]; //user's name
  int sex; //female 0, male 1, unsure 2
};


void load_high_scores() {  
 
  int addressJump = sizeof(gripperProfile);
  int j = 0;
  
   //loop through all possible spaces in EEPROM save the high scores in arrays (up to 15)
   for( int i=0; i<addressJump*15;i+=addressJump ) {
       //if address is not empty then get it and save it
       if( EEPROM.read(i) > 0 && EEPROM.read(i) != 255 ) {
         struct highscore {  
           float gf;
           char fn[15];
           int s;    
         };
        highscore hs;
        EEPROM.get(i, hs);          
        gripForceHS[j] = hs.gf;
        firstNameHS[j][15] = hs.fn;
        sexHS[j] = hs.s;
       } else {
        break;
       }
       j++;       
   }
}

   
class Gripper
{
  //Class member variables are initialized at startup   
  public: gripperProfile gp;
  //Constructor creates a Gripper
  
  public:
  Gripper(float GripForce, char FirstName[15], int Sex)
  {   
      gp.gripForce = GripForce;
      strcpy(gp.firstName, FirstName);
      gp.sex = Sex;           
  }

  public:
  void Save(int address)
  {    
    //Save the new high score name and grip strength to EEPROM (top 15 scores);
    display.setCursor(0,20);
    display.print(gp.gripForce);    
    display.print(" ");    
    display.print(gp.firstName);    
    display.print(" ");    
    display.print(gp.sex);    
    EEPROM.put(address, gp);   
  }
  
  public:
  void updateHS(int a) {
  //now need to refresh the highscores
  //a is the array index
   gripForceHS[a] = gp.gripForce;
   firstNameHS[a][15] = gp.firstName;
   sexHS[a] = gp.sex;
  } 
  
};

int isHighScore(float gripForce) {
   
   //loop through all highscores and check if this current score beats one (up to 15)
   for( int i=0; i<15; i++ ) {         
       if( gripForce > gripForceHS[i] ) {            
        return i;
        break;
       }        
   }
   //not a new highscore
   return -1;
}


void setup() {
  
  mode = 1;
 //for scale  
  scale.set_gain(64);  
  scale.tare(50);  //Reset the scale to 0  
  scale.set_gain(32);  
  scale.tare(50);  //Reset the scale to 0  
  scale.set_scale();

  //load all the high scores into memory
  load_high_scores();
  
   ///////////for OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C 
   
  // init done 
  display.display();
   // Clear the buffer.
  display.clearDisplay(); 
  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
 
  display.display();  
  display.clearDisplay();
  /////////////////end OLED display 
  
  //Serial.begin(9600);
  
}


void loop() {
  
  struct myscore {  
    float gf;
    char fn[15];
    int s;    
  };
  myscore ms;
  
  EEPROM.get(0, ms);
    display.setCursor(0,10);        
    display.print(ms.gf);     
    display.print(" ");
    display.print(ms.fn);     
    display.print(" ");      
    display.print(ms.s); 
  myscore ms2;
  
  EEPROM.get(21, ms2);
    display.setCursor(0,20);        
    display.print(ms2.gf);     
    display.print(" ");
    display.print(ms2.fn);     
    display.print(" ");      
    display.print(ms2.s); 
       
    display.display();
  float gripForceNow = checkScale();
   display.clearDisplay();
   //debugging
   display.setCursor(80,24);  
   display.print(modeLookup[mode]); 
   
    switch(mode) {
      case 0:
        animation(); 
        break;
      case 1:    
        display.setCursor(0,0);  
        display.print("Power: ");      
        display.print(gripForceNow);
        display.print(" ");      
        if (gripForceNow > 150) {   
          int newHSAddress = isHighScore(gripForceNow);  
          int eeAddress = newHSAddress * sizeof(gripperProfile);
          if(newHSAddress >= 0) {
            Gripper g1(gripForceNow, "Nick", 1);
              display.print(eeAddress);                
              g1.Save(eeAddress);
              g1.updateHS(newHSAddress);
          }
                
         // mode = 2;
        }
        break;
      case 2: 
        //save
        display.setCursor(0,0);  
        display.print("High: Name: ");
        display.setCursor(0,10);
        display.print(ms.gf);
        display.print(" ");
        display.print(ms.fn);
        display.print(" ");
        display.print(ms.s);      
        break;
       case 3:
        display.setCursor(0,0);  
        display.print(firstNameHS[0]);
        display.print(" ");        
        display.print(gripForceHS[0]);
        display.print(" ");
        display.print(sexHS[0]);
        display.print(" ");
        display.setCursor(0,10);  
        display.print(firstNameHS[1]);
        display.print(" ");        
        display.print(gripForceHS[1]);
        display.print(" ");
        display.print(sexHS[1]);
        display.print(" ");
        display.setCursor(0,20);  
        display.print(firstNameHS[2]);
        display.print(" ");        
        display.print(gripForceHS[2]);
        display.print(" ");
        display.print(sexHS[2]);
        display.print(" ");
                
       break;
    }
    display.display();
}

float checkScale() {    
  scale.set_scale(calibration_factor);   
  
  scale.set_gain(64); //reads from channel A on hx711 board 
  float loadA = scale.get_units();   
  
  scale.set_gain(32); //reads from channel B on hx711 board for 2nd load cell 
  float loadB = 2*scale.get_units(); //returns in Decagrams (100s of grams)  
  
  float avg = (loadA+loadB)/2; 
  return avg;
}

void animation(void) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(15,0);
  display.clearDisplay();
  display.print("PRESS START");
  display.display();
  delay(1);
 
  display.startscrollright(0x00, 0x0a); 
  delay(1000);
  display.stopscroll();
    
  delay(1500);
  display.startscrollleft(0x00, 0x0a);
  delay(1000);
  display.stopscroll();
  
}
