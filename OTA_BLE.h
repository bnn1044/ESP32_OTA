uint8_t updater[16384];
uint8_t updater2[16384];
uint8_t fw_version[] = "1.0.1";
#define SERVICE_UUID                "fb1e4001-54ae-4a28-9f74-dfccb248601d"
#define CHARACTERISTIC_UUID_RX      "fb1e4002-54ae-4a28-9f74-dfccb248601d"
#define CHARACTERISTIC_UUID_TX      "fb1e4003-54ae-4a28-9f74-dfccb248601d"
#define CHARACTERISTIC_UUID_FW      "fb1e4004-54ae-4a28-9f74-dfccb248601d"
#define CHARACTERISTIC_UUID_SPEED   "fb1e4005-54ae-4a28-9f74-dfccb248601d"
#define FORMAT_FFAT_IF_FAILED true

// FFat is faster
#include "FFat.h"
#define FLASH FFat
const bool FASTMODE = true;

#define NORMAL_MODE   0   // normal
#define UPDATE_MODE   1   // receiving firmware
#define OTA_MODE      2   // installing firmware
long int StartTime,EndTime,DeltaTime;

static int MODE = NORMAL_MODE;
static BLECharacteristic* pCharacteristicTX;
static BLECharacteristic* pCharacteristicFW;
static BLECharacteristic* pCharacteristicSPEED;
static BLECharacteristic* pCharacteristicRX;
static bool deviceConnected = false, sendMode = false, sendSize = true;
static bool writeFile = false, request = false;
static int writeLen = 0, writeLen2 = 0;
static bool current = true;
static int parts = 0, next = 0, cur = 0, MTU = 0;
unsigned long rParts, tParts;
static void rebootEspWithReason(String reason) {
  Serial.println(reason);
  delay(1000);
  ESP.restart();
}
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
    }
};
class MyCallbacks: public BLECharacteristicCallbacks {
   void onStatus(BLECharacteristic* pCharacteristic, Status s, uint32_t code) {
          Serial.print("Status ");
          Serial.print(s);
          Serial.print(" on characteristic ");
          Serial.print(pCharacteristic->getUUID().toString().c_str());
          Serial.print(" with code ");
          Serial.println(code);
        }
    void onNotify(BLECharacteristic *pCharacteristic) {
      uint8_t* pData;
      std::string value = pCharacteristic->getValue();
      int len = value.length();
      pData = pCharacteristic->getData();
      if (pData != NULL) {
           Serial.print("Notify callback for characteristic ");
           Serial.print(pCharacteristic->getUUID().toString().c_str());
           Serial.print(" of data length ");
           Serial.println(len);
           Serial.print("TX  ");
           for (int i = 0; i < len; i++) {
              Serial.printf("%02X ", pData[i]);
          }
          Serial.println();
      }
    }
    void onWrite(BLECharacteristic *pCharacteristic) {
      uint8_t* pData;
      std::string value = pCharacteristic->getValue();
      int len = value.length();
      pData = pCharacteristic->getData();
      /*if (pData != NULL) {
           Serial.print("Write callback for characteristic ");
           Serial.print(pCharacteristic->getUUID().toString().c_str());
           Serial.print(" of data length ");
           Serial.println(len);
           Serial.print("RX  ");
           for (int i = 0; i < len; i++) {         // leave this commented
             Serial.printf("%02X ", pData[i]);
            }
           }*/
        if (pData[0] == 0xFB) {                       // write data to the flash
          for(int i =0;i < 2;i ++){
           Serial.printf("%02X ", pData[i]);
          }
          Serial.print(" Current :");
          Serial.print(current);
          Serial.print(" len :");
          Serial.println(len);
          
          int pos = pData[1];
          for (int x = 0; x < len - 2; x++) {
            if (current) {
              updater[(pos * MTU) + x] = pData[x + 2];
            } else {
              updater2[(pos * MTU) + x] = pData[x + 2];
            }
          }
        } else if  (pData[0] == 0xFC) {
          for(int i =0;i < len;i ++){
            Serial.printf("%02X ", pData[i]);
          }
          if (current) {
            writeLen = (pData[1] * 256) + pData[2];
          } else {
            writeLen2 = (pData[1] * 256) + pData[2];
          }
          current = !current;
          cur = (pData[3] * 256) + pData[4];
          writeFile = true;
          if (cur < parts - 1) {
            request = !FASTMODE;
          }
          Serial.print(" request : ");
          Serial.print(request);
          Serial.print(" Cur = ");
          Serial.print(cur);
          Serial.print(" Parts = ");
          Serial.print(parts);
          Serial.println(" ");
          
        } else if (pData[0] == 0xFD) {
          for(int i =0;i < len;i ++){
            Serial.printf("%02X ", pData[i]);
          }
          Serial.println(" ");
          sendMode = true;
          if (FLASH.exists("/update.bin")) {
            FLASH.remove("/update.bin");
          }
          Serial.println("update started");
        } else if (pData[0] == 0xFE) {                  // app call update FW
          for(int i =0;i<len; i++){
            Serial.printf("%02X ", pData[i]);
          }
          Serial.println(" ");
          rParts = 0;
          tParts = (pData[1] * 256 * 256 * 256) + (pData[2] * 256 * 256) + (pData[3] * 256) + pData[4];
          Serial.print("Available space: ");
          Serial.println(FLASH.totalBytes() - FLASH.usedBytes());
          Serial.print("File Size: ");
          Serial.println(tParts);

        } else if  (pData[0] == 0xFF) {               // change to update mode
          for(int i =0; i < len ;i ++){
            Serial.printf("%02X ", pData[i]);
          }
          Serial.println(" ");
          parts = (pData[1] * 256) + pData[2];         //
          MTU = (pData[3] * 256) + pData[4];          // also update how many byte it send over. 500 byte of data per frame
          Serial.print(" parts : ");
          Serial.print(parts);
          Serial.print(" MTU : ");
          Serial.println(MTU);
          MODE = UPDATE_MODE;

        } else if (pData[0] == 0xEF) {
          for(int i =0;i < len;i ++){
            Serial.printf("%02X ", pData[i]);
          }
          Serial.println(" ");
          FLASH.format();
          sendSize = true;
        }else if (pData[0] == 0xF0){                            // APP REQUEST FOR fw Version
          Serial.println("App request For FW Version");
          pCharacteristicFW->setValue(fw_version, 5);
          pCharacteristicFW->notify();
        }
      }
};
static void writeBinary(fs::FS &fs, const char * path, uint8_t *dat, int len) {
  Serial.printf("Write binary file %s\r\n", path);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  file.write(dat, len);
  file.close();
  writeFile = false;
  rParts += len;
}
void sendOtaResult(String result) {
  pCharacteristicTX->setValue(result.c_str());
  pCharacteristicTX->notify();
  delay(200);
}
void performUpdate(Stream &updateSource, size_t updateSize) {
  char s1 = 0x0F;
  String result = String(s1);
  if (Update.begin(updateSize)) {
    size_t written = Update.writeStream(updateSource);
    if (written == updateSize) {
      Serial.println("Written : " + String(written) + " successfully");
    }
    else {
      Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
    }
    result += "Written : " + String(written) + "/" + String(updateSize) + " [" + String((written / updateSize) * 100) + "%] \n";
    if (Update.end()) {
      Serial.println("OTA done!");
      result += "OTA Done: ";
      if (Update.isFinished()) {
        Serial.println("Update successfully completed. Rebooting...");
        result += "Success!\n";
      }
      else {
        Serial.println("Update not finished? Something went wrong!");
        result += "Failed!\n";
      }
    }
    else {
      Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      result += "Error #: " + String(Update.getError());
    }
  }
  else
  {
    Serial.println("Not enough space to begin OTA");
    result += "Not enough space for OTA";
  }
  if (deviceConnected) {
    sendOtaResult(result);
    delay(5000);
  }
}
void updateFromFS(fs::FS &fs) {
  File updateBin = fs.open("/update.bin");
  if (updateBin) {
    if (updateBin.isDirectory()) {
      Serial.println("Error, update.bin is not a file");
      updateBin.close();
      return;
    }

    size_t updateSize = updateBin.size();

    if (updateSize > 0) {
      Serial.println("Trying to start update");
      performUpdate(updateBin, updateSize);
    }
    else {
      Serial.println("Error, file is empty");
    }
    updateBin.close();
    // when finished remove the binary from spiffs to indicate end of the process
    Serial.println("Removing update file");
    fs.remove("/update.bin");
    rebootEspWithReason("Rebooting to complete OTA update");
  }
  else {
    Serial.println("Could not load update.bin from spiffs root");
  }
}
void initBLE() {
  BLEDevice::init("JARVIS AST");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristicTX = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY );
  pCharacteristicFW = pService->createCharacteristic(CHARACTERISTIC_UUID_FW, BLECharacteristic::PROPERTY_NOTIFY );
  pCharacteristicSPEED = pService->createCharacteristic(CHARACTERISTIC_UUID_SPEED, BLECharacteristic::PROPERTY_NOTIFY );
  pCharacteristicRX = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pCharacteristicRX->setCallbacks(new MyCallbacks());
  pCharacteristicTX->setCallbacks(new MyCallbacks());
  pCharacteristicTX->addDescriptor(new BLE2902());
  pCharacteristicFW->addDescriptor(new BLE2902());
  pCharacteristicSPEED->addDescriptor(new BLE2902());
  pCharacteristicTX->setNotifyProperty(true);
  pService->start();
  
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}
