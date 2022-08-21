/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager

// Firebase Setup
#include <ArduinoJson.h>
#include <FirebaseESP8266.h>

#include <SoftwareSerial.h>
#include <SPI.h>
#include <MFRC522.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"


// Insert Firebase project API Key
#define API_KEY "AIzaSyBW8Tbb2PlRMcWfpwiPKF6luyPSD0WmdDM "

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://smarthomedemo-8fab6-default-rtdb.asia-southeast1.firebasedatabase.app/" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
SoftwareSerial ESP8266_SoftSerial (1,2);
bool signupOK = false;

// Set web server port number to 80
WiFiServer server(80);
// Assign output variables to GPIO pins
const int output5 = 5;
const int output4 = 4;

constexpr uint8_t RST_PIN = 16;        // Define pin D0 for the RST pin
constexpr uint8_t SDA_PIN = 15;        // Define pin D8 for the SDA pin

MFRC522 mfrc522(SDA_PIN, RST_PIN);  // Create MFRC522 instance
void setup() {
  Serial.begin(115200);
//   ESP8266_SoftSerial.begin(9600);
  // Initialize the output variables as outputs
//  pinMode(output5, OUTPUT);
//  pinMode(output4, OUTPUT);
  // Set outputs to LOW
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  
  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();
  
  // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("SmartHome");
  // or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();
  
  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
  
  server.begin();

  // CONFIGURE FIREBASE
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
//  SPI.begin();      // Initiate  SPI bus
//  mfrc522.PCD_Init();
  pinMode(output4, OUTPUT);
  pinMode(output5, OUTPUT);
  while (!Serial);  // Do nothing if no serial port is opened
  SPI.begin();    // Initialize SPI bus
  mfrc522.PCD_Init(); // Initialize MFRC522
  Serial.println("Approximate your card to the reader...");
}

void loop(){
 getDataDoor();
 getDataLight();
 checkCard();
}
boolean getUID() 
{
  // Getting ready for reading Tags
  if ( ! mfrc522.PICC_IsNewCardPresent()) {   //If a new tag is placed close to the RFID reader, continue
  return false;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {     //When a tag is placed, get UID and continue
  return false;
  }
  return true;
}
void getDataDoor() {
  if(Firebase.RTDB.getString(&fbdo,"Door/Status"))
    { 
      String Status = fbdo.stringData();
//      Serial.print("Door: ");
//      Serial.println(Status);
      if(Status == "On"){
        digitalWrite(output4, HIGH);
      }
      else{
        digitalWrite(output4, LOW);
      }
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
}
void getDataLight(){
  if(Firebase.RTDB.getString(&fbdo,"Light/Status"))
    { 
      String Status = fbdo.stringData();
      if(Status == "On"){
        digitalWrite(output5, HIGH);
      }
      else{
        digitalWrite(output5, LOW);
      }
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
}
void checkCard(){
  while (getUID()) 
  {
  Serial.print("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  if ((content.substring(1) == "14 A8 CF 2B") ) //change here the UID of the card/cards that you want to give access
  {
    Serial.println("Authorized access");
    Serial.println();
    digitalWrite(output4, HIGH);
    Firebase.RTDB.setString(&fbdo, "Door/Status", "On");
    delay(5000);
    Firebase.RTDB.setString(&fbdo, "Door/Status", "Off");
  }
 else   {
    Serial.println(" Access denied");
  }
  }
}
