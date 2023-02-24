#include <Update.h>
#include "FS.h"
#include "FFat.h"
#include <BLEDevice.h>
//#include <BLEUtils.h>
//#include <BLEServer.h>
#include <BLE2902.h>
#include <Adafruit_NeoPixel.h>
#include "RGBLed.h"
#include "OTA_BLE.h"
#include "Hardware.h"

float SensorSpacing = 1.0;           //default 1 inch
float BoltSpeed = 0.0;              
long int looptime = 0;
// Variables will change:
boolean ledState = LOW;              // ledState used to set the LED
long    ButtonMills;                
boolean ConnectionState = false;
int LEDon = 1;
int ButtonInput;
int latchTestButton;

//#endif
/*
 * Top sensor Intterupt
 */
void IRAM_ATTR Start(){
  StartTime = micros();
  Serial.println("Top Sensor On");
  detachInterrupt(TopSensorInput);
}
/*
 * Bottom sensor Intterupt
 */
void IRAM_ATTR End(){
   //detachInterrupt(BottomSensorIntput);
  Serial.println("Bom Sensor On");
  if(StartTime >=0){
    EndTime = micros();
    DeltaTime = EndTime - StartTime ; 
  }
}
void setup() {
  Serial.begin(115200);
  Serial.println("Setup Some hardware!!");
  
    //attach the sensor to the intterupt
  pinMode(TopSensorInput,INPUT_PULLUP);
  pinMode(BottomSensorIntput,INPUT_PULLUP);
  pinMode(Analog_Pin1,INPUT);
   //attach the sensor interrupt to the pin
  attachInterrupt(digitalPinToInterrupt(TopSensorInput),Start,FALLING);
  attachInterrupt(digitalPinToInterrupt(BottomSensorIntput),End,FALLING);
  
  if (!FFat.begin()) {
    Serial.println("FFat Mount Failed");
    if (FORMAT_FFAT_IF_FAILED) FFat.format();
    return;
  }
  initBLE();

}
void loop() {
  switch (MODE) {
    case NORMAL_MODE:
      //Serial.println("Running normal mode");
      if (deviceConnected) {
        if (sendMode) {
          uint8_t fMode[] = {0xAA, FASTMODE};
          pCharacteristicTX->setValue(fMode, 2);
          pCharacteristicTX->notify();
          delay(50);
          sendMode = false;
        }
        if (sendSize) {
          unsigned long x = FLASH.totalBytes();
          unsigned long y = FLASH.usedBytes();
          uint8_t fSize[] = {0xEF, (uint8_t) (x >> 16), (uint8_t) (x >> 8), (uint8_t) x, (uint8_t) (y >> 16), (uint8_t) (y >> 8), (uint8_t) y};
          pCharacteristicTX->setValue(fSize, 7);
          pCharacteristicTX->notify();
          delay(50);
          sendSize = false;
        }
          TurnOnRGBBlue();   
      } else {
        blinkLed(500,1);   // blink Blue when there is no conenction
      }
      break;
    case UPDATE_MODE:
      blinkLed(100,2); 
      if (request) {
        uint8_t rq[] = {0xF1, (cur + 1) / 256, (cur + 1) % 256};
        Serial.println( "in request mode");
        Serial.print("send the app : ");
        for (int i = 0; i < 3; i++){
          Serial.println(rq[i]);
        }
        pCharacteristicTX->setValue(rq, 3);
        pCharacteristicTX->notify();
        delay(50);
        request = false;
      }
      if (cur + 1 == parts) {                                   // received complete file
        uint8_t com[] = {0xF2, (cur + 1) / 256, (cur + 1) % 256};
        pCharacteristicTX->setValue(com, 3);
        pCharacteristicTX->notify();
        delay(50);
        MODE = OTA_MODE;
      }
      if (writeFile) {
        if (!current) {
          writeBinary(FLASH, "/update.bin", updater, writeLen);
        } else {
          writeBinary(FLASH, "/update.bin", updater2, writeLen2);
        }
      }
      break;
    case OTA_MODE:
      blinkLed(500,2);
      //Serial.println("Running OTA mode");
      if (writeFile) {
        if (!current) {
          writeBinary(FLASH, "/update.bin", updater, writeLen);
        } else {
          writeBinary(FLASH, "/update.bin", updater2, writeLen2);
        }
        
      }
      if (rParts == tParts) {
        Serial.println("Complete");
        delay(5000);
        updateFromFS(FLASH);
      } else {
        writeFile = true;
        Serial.println("Incomplete");
        Serial.print("Expected: ");
        Serial.print(tParts);
        Serial.print("Received: ");
        Serial.println(rParts);
        delay(2000);
      }
      break;
  }
}
