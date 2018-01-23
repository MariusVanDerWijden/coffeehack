/* 
 Sharespresso by c't/Peter Siering
 https://github.com/psct/sharespresso
 
 is an Arduino-based RFID payment system for coffeemakers with toptronic logic unit, as Jura 
 Impressa S95 and others without modifying the coffeemaker itself. 
 Based on Oliver Krohns famous Coffeemaker-Payment-System 
 at https://github.com/oliverk71/Coffeemaker-Payment-System
 
 Hardware used: Arduino Uno, 16x2 LCD I2C, pn532/mfrc522 rfid card reader (13.56MHz), 
 HC-05 bluetooth, male/female jumper wires (optional: ethernet shield, buzzer, button)

 The code has been modified by Marius van der Wijden for private use
*/

// options to include into project
//#define SERVICEBUT 8 // button to switch to service mode 
#define DEBUG 1 // some more logging

// coffemaker model
//#define X7 1 // x7/saphira
#define S95 1

// include selected libraries
#include <Wire.h>
#include <SoftwareSerial.h>
#include <EEPROMex.h>
#include <SPI.h>

// hardware specific settings
SoftwareSerial myCoffeemaker(2,3); // RX, TX

// product codes send by coffeemakers "?PA<x>\r\n", just <x>
#if defined(S95)
char products[] = "EFABJIG";
#endif
#if defined(X7)
char products[] = "ABCHDEKJFG";
#endif

// general variables (used in loop)
boolean buttonPress = false;
boolean override = false;  // to override payment system by the voice-control/button-press app
int inservice=0;
int price=0;
String last_product="";

void setup()
{
#if defined(SERLOG) || defined(DEBUG) || defined(MEMDEBUG)
  Serial.begin(9600);
#endif
#if defined(DEBUG)
  EEPROM.setMaxAllowedWrites(100);
  EEPROM.setMemPool(0, EEPROMSizeUno);
  Serial.println(sizeof(products));
#endif
  message_print(F("sharespresso"), F("starting up"));
  myCoffeemaker.begin(9600);         // start serial communication at 9600bps
#if defined(DEBUG)
  serlog(F("Initializing rfid reader"));
#endif
  SPI.begin();


  // configure service button
#if defined(SERVICEBUT)
  pinMode(SERVICEBUT,INPUT);
#endif
  message_print(F("Ready to brew"), F(""));
  // activate coffemaker connection and inkasso mode
  myCoffeemaker.listen();
  inkasso_on();
}

void loop()
{
#if defined(SERVICEBUT)
  if ( digitalRead(SERVICEBUT) == HIGH) {
    servicetoggle();
  }
#endif  
  
  //Read BTstring from Raspberry
  BTstring = "FA_04";
  
  if(BTstring.length() > 0)
  {
    if(BTstring == "?M3"){
      inkasso_on();
    }
    if(BTstring == "?M1"){
      inkasso_off();  
    }
    if(BTstring == "FA:04"){        // small cup ordered via app
      toCoffeemaker("FA:04\r\n"); 
      override = true;
    }
    if(BTstring == "FA:06"){        // large cup ordered via app
      toCoffeemaker("FA:06\r\n");  
      override = true;
    }
    if(BTstring == "FA:0C"){        // extra large cup ordered via app
      toCoffeemaker("FA:0C\r\n");  
      override = true;
    }    
  }          

  // Get key pressed on coffeemaker
  String message = fromCoffeemaker();   // gets answers from coffeemaker 
  if (message.length() > 0){
    serlog( message);

    if (message.charAt(0) == '?' && message.charAt(1) == 'P'){     // message starts with '?P' ?
      buttonPress = true;
      int product = 255;
      for (int i = 0; i < sizeof(products); i++) {
        if (message.charAt(3) == products[i]) {
          product = i;
          break;
        }
      }
      if ( product != 255) {
        String productname;
          switch (product) {
#if defined(S95)
            case 0: productname = F("Small cup"); break;
            case 1: productname = F("2 small cups"); break;
            case 2: productname = F("Large cup"); break;
            case 3: productname = F("2 large cups"); break;
            case 4: productname = F("Steam 2"); break;
            case 5: productname = F("Steam 1"); break;
            case 6: productname = F("Extra large cup"); break;
#endif
#if defined(X7)
            case 0: productname = F("Cappuccino"); break;
            case 1: productname = F("Espresso"); break;
            case 2: productname = F("Espresso dopio"); break;
            case 3: productname = F("Milchkaffee"); break;
            case 4: productname = F("Kaffee"); break;
            case 5: productname = F("Kaffee gross"); break;
            case 6: productname = F("Dampf links"); break;
            case 7: productname = F("Dampf rechts"); break;
            case 8: productname = F("Portion Milch"); break;
            case 9: productname = F("Caffee Latte"); break;
#endif
          }
        last_product= String(message.charAt( 3))+ "/"+ String(product)+ " ";
        message_print(productname, "price");
      } 
      else {
        message_print(F("Error unknown"), F("product"));
        buttonPress = false;
      }
    }
    if(buttonPress)
      toCoffeemaker("?ok\r\n");
  }  
}

String fromCoffeemaker(){
  String inputString = "";
  byte d0, d1, d2, d3;
  char d4 = 255;
  while (myCoffeemaker.available()){    // if data is available to read
    d0 = myCoffeemaker.read();
    delay (1); 
    d1 = myCoffeemaker.read();
    delay (1); 
    d2 = myCoffeemaker.read();
    delay (1); 
    d3 = myCoffeemaker.read();
    delay (7);
    bitWrite(d4, 0, bitRead(d0,2));
    bitWrite(d4, 1, bitRead(d0,5));
    bitWrite(d4, 2, bitRead(d1,2));
    bitWrite(d4, 3, bitRead(d1,5));
    bitWrite(d4, 4, bitRead(d2,2));
    bitWrite(d4, 5, bitRead(d2,5));
    bitWrite(d4, 6, bitRead(d3,2));
    bitWrite(d4, 7, bitRead(d3,5));
    inputString += d4;
  }
  inputString.trim();
  if ( inputString != "") {
        return(inputString);
  } 
}

void toCoffeemaker(String outputString)
{
  for (byte a = 0; a < outputString.length(); a++){
    byte d0 = 255;
    byte d1 = 255;
    byte d2 = 255;
    byte d3 = 255;
    bitWrite(d0, 2, bitRead(outputString.charAt(a),0));
    bitWrite(d0, 5, bitRead(outputString.charAt(a),1));
    bitWrite(d1, 2, bitRead(outputString.charAt(a),2));  
    bitWrite(d1, 5, bitRead(outputString.charAt(a),3));
    bitWrite(d2, 2, bitRead(outputString.charAt(a),4));
    bitWrite(d2, 5, bitRead(outputString.charAt(a),5));
    bitWrite(d3, 2, bitRead(outputString.charAt(a),6));  
    bitWrite(d3, 5, bitRead(outputString.charAt(a),7)); 
    myCoffeemaker.write(d0); 
    delay(1);
    myCoffeemaker.write(d1); 
    delay(1);
    myCoffeemaker.write(d2); 
    delay(1);
    myCoffeemaker.write(d3); 
    delay(7);
  }
}

void serlog(String msg) {
  Serial.println(msg);
}

void message_print(String msg1, String msg2)
{
  if (msg1 != "") { Serial.print(msg1 + " "); }
  if (msg2 != "") { Serial.print(msg2); }
  if ((msg1 != "") || (msg2 != "")) { Serial.println(""); }
}


void servicetoggle(void){
    inservice=not(inservice);
    if ( inservice) {
      message_print(F("Service Mode"),F("started"));
      inkasso_off();
    } else {
      message_print(F("Service Mode"),F("exited"));
      myCoffeemaker.listen();
      inkasso_on();
    }
}

void inkasso_on(void){
  toCoffeemaker("?M3\r\n");  // activates incasso mode (= no coffee w/o "ok" from the payment system! May be inactivated by sending "?M3" without quotation marks)
  delay (100);               // wait for answer from coffeemaker

  if (fromCoffeemaker() == "?ok"){
    message_print(F("Inkasso mode"),F("activated!"));  
  } else {
    message_print(F("Coffeemaker"),F("not responding!"));  
  }  
}

void inkasso_off(void){
  toCoffeemaker("?M1\r\n");  // deactivates incasso mode (= no coffee w/o "ok" from the payment system! May be inactivated by sending "?M3" without quotation marks)
  delay (100);               // wait for answer from coffeemaker
  if (fromCoffeemaker() == "?ok"){
    message_print(F("Inkasso mode"),F("deactivated!"));  
  } else {
    message_print(F("Coffeemaker"),F("not responding!"));  
  }
}