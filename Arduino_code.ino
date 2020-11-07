/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/         
#define        resolution12bit                 4095           
#define        REF_VOLTAGE                     3.3         


#include <stdlib.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "Arduino.h"
#include <ESP32AnalogRead.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t something = 0;
int fire = 0 ;
//char triggerVoltage[3];
//uint32_t voltage=0;
String inString = ""; // string to hold input
int triggerVoltage = 0;




float setTriggerVoltage(uint32_t something, int i){
//  triggerVoltage[i] = float(something);
//  Serial.print("triggerVoltage[");
//  Serial.print(i);
//  Serial.print("] = ");
//  Serial.println(triggerVoltage[i]);
inString += char(something);
if(something == '.'){
//  Serial.print("Value:");
//  Serial.println(inString.toInt());
  triggerVoltage = inString.toInt();
  inString = "";
}
}



class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic){
    std::string value = pCharacteristic->getValue();

    if(value.length() > 0){
      for(int i=0; i < value.length(); i++){
        if(value[i]=='f'){
          fire=1;
          }
        else{
          setTriggerVoltage(value[i],i);
          }  
      }
      Serial.println("End of Callback");
   }
  }
};

//IO define pin
//const int vmeas1=32;
ESP32AnalogRead ADC_Voltage;
const int triggerPin=2;

//variables
float VoltageADC;
//float v1;



void setup() {
//  pinMode(vmeas1,INPUT);
  ADC_Voltage.attach(32);
  pinMode(triggerPin,OUTPUT);
  
  Serial.begin(115200);
    
  
  // Create the BLE Device
  BLEDevice::init("ESP32 BLE Device");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());               

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  //get adc reading 
//  VoltageADC=analogRead(vmeas1);
  VoltageADC = ADC_Voltage.readVoltage();
  //VoltageADC=Filter(VoltageADC);
  
//  v1=(VoltageADC*REF_VOLTAGE/resolution12bit);

  Serial.print("V1: ");
  Serial.print(VoltageADC);
  Serial.println(" V");  

   
    // notify changed value
    if (deviceConnected) {
      String str = "";
        str += VoltageADC;
        
        pCharacteristic->setValue((char*)str.c_str());
        pCharacteristic->notify();
        
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }

      Serial.println(triggerVoltage);

    if(VoltageADC>=triggerVoltage){
      fire=1;
      triggerVoltage = 10000000;
    }
//  Serial.print("the value in main loop is");
//  Serial.println(triggerVoltage);
    if(fire==1){
      digitalWrite(triggerPin, HIGH);
      delay(1000);
      fire=0;
      digitalWrite(triggerPin, LOW);
      }

   
    delay(500);
}
