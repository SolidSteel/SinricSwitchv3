#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <ESP32Servo.h>

#define WIFI_SSID "RouterMarcos1"
#define WIFI_PASS "marcoswifi"

// CONFIG PARA EL SWITCH TESTER
//#define APP_KEY "42b4604f-ff00-4e9f-9392-2c5d1e33d357"
//#define APP_SECRET "e31b40ce-afac-40d5-9177-0500e0b0e03a-d7737520-7d24-4131-b534-d977d0ae2f16"
//#define SWITCH_ID "62e7ecb04dd95ec7bdbdf895"

#define APP_KEY "f0b0f07e-5442-4b10-b4f3-36c140c6c369"
#define APP_SECRET "e3308926-5798-4753-8c8b-cf05a216fdb0-c7e8f350-bc0f-4089-a46b-99f53857cb91"
#define SWITCH_ID "62e157b54dd95ec7bdbadf58"

#define SWITCH_PIN 2  // Change this to the desired ESP32 pin
#define servoPin 13   //Pin al que conectamos el cable naranja del servo.
Servo miServo;

//PINES   15 naranja 5 azul  0 verde 4 amarillo
const int RELAY1 = 15;
const int RELAY2 = 5;
const int RELAY3 = 0;
const int RELAY4 = 4;


//CONS/VARS GENERALES
const int BLE_SCAN_DURATION = 2;           // BLE scanning duration in seconds
const int BLE_PAUSE_DURATION = 3;          // Pause duration between scans in seconds
const int VALID_DETECTION_DURATION = 25;   // Valid MAC detection duration in seconds
unsigned long DuracionAperturams = 11000;  // duracion base de apertura de la puerta
unsigned long BLEREESCANFlag;


//POSTURAS POR DEFECTO DEL SERVO QUE CONTROLA EL BULON
const int ServoClosedPos = 160;
const int ServoOpenPos = 50;


//MACS VALIDAS
const char* AuthorisedMACs[] = {
  "0000feaa-0000-1000-8000-00805f9b34fb",
  "dc:0d:30:03:50:94"
};


const int NUM_AUTHORISED_MACS = sizeof(AuthorisedMACs) / sizeof(AuthorisedMACs[0]);
bool IsDoorClosed = false;
bool IsDoorBeingClosed = false;
bool SinricswitchState = false;
bool ValidMACDetected = false;          // Flag to store the valid MAC detection status
unsigned long validDetectionStartTime;  // Variable to store the start time of valid detection

unsigned long DoorInOperationFlag;  // evitar operaciones con la puerta si esta esta ocupada
unsigned long lastScanTime;
BLEScan* pBLEScan;
SinricProSwitch& mySwitch = SinricPro[SWITCH_ID];


// FUNCIONES
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String macAddress = advertisedDevice.getAddress().toString().c_str();
    Serial.println(macAddress);

    for (int i = 0; i < NUM_AUTHORISED_MACS; i++) {
      if (macAddress.equals(AuthorisedMACs[i])) {
        ValidMACDetected = true;
        validDetectionStartTime = millis();
        Serial.print("Valid MAC detected , expiry in:");
        Serial.println(String(VALID_DETECTION_DURATION));

        return;
      }
    }
  }
};

// Callback function to handle switch events
bool onSwitchState(const String& deviceId, bool& state) {
  // Do something when the switch state is changed
  Serial.print("Switch state changed to: ");
  Serial.println(state ? "ON" : "OFF");
  SinricswitchState = state;
  return true;
}

void setupPINS() {
  pinMode(RELAY1, OUTPUT);  //RELAY N1
  pinMode(RELAY2, OUTPUT);  //RELAY N2
  pinMode(RELAY3, OUTPUT);  //RELAY N3
  pinMode(RELAY4, OUTPUT);  //RELAY N4


  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);
  digitalWrite(RELAY4, HIGH);

  //CTRL SERVO
  miServo.attach(servoPin);
}


void bleScanAndDetect() {

  if (millis() > lastScanTime + ((BLE_SCAN_DURATION + BLE_PAUSE_DURATION) * 1000)) {
    Serial.println("BLE SCAN");
    lastScanTime = millis();
    // Start scanning for BLE_SCAN_DURATION seconds

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->start(BLE_SCAN_DURATION);
  }
}



void LoopDoorLogic() {

  if (ValidMACDetected == true && ((validDetectionStartTime + (VALID_DETECTION_DURATION * 1000)) < millis())) {
    Serial.println("Valid MAC RESET");
    ValidMACDetected = false;
  }

  if (ValidMACDetected && IsDoorClosed == true && (DoorInOperationFlag < millis())) {


    //EMPUJAR Hacia ADENTRO
    digitalWrite(RELAY1, LOW);
    digitalWrite(RELAY2, HIGH);
    Serial.println("Empuje ayuda hacia adentro");

    //Abrir el servo
    miServo.write(ServoOpenPos);
    Serial.println("Se abre servo OVERRIDE BLE y delay");
    delay(800);

    digitalWrite(SWITCH_PIN, HIGH);
    // ESTO ABRE
    Serial.println("OPEN BY BLE");
    digitalWrite(RELAY1, HIGH);
    digitalWrite(RELAY2, LOW);
    DoorInOperationFlag = millis() + DuracionAperturams;
    IsDoorClosed = false;
  }


  if (SinricswitchState == true && IsDoorClosed == true && DoorInOperationFlag < millis() && !ValidMACDetected) {
    //EMPUJAR Hacia ADENTRO
    digitalWrite(RELAY1, LOW);
    digitalWrite(RELAY2, HIGH);
    Serial.println("Empuje ayuda hacia adentro");

    //Abrir el servo
    miServo.write(ServoOpenPos);
    Serial.println("Se abre servo por SRWITCH y delay");
    delay(800);

    digitalWrite(SWITCH_PIN, HIGH);

    // ESTO ABRE
    Serial.println("OPEN BY SRWITCH");
    digitalWrite(RELAY1, HIGH);
    digitalWrite(RELAY2, LOW);
    DoorInOperationFlag = millis() + DuracionAperturams;
    IsDoorClosed = false;
  }


  if (SinricswitchState == false && IsDoorClosed == false && DoorInOperationFlag < millis() && !ValidMACDetected) {
    //EMPUJAR Hacia ADENTRO
    digitalWrite(RELAY1, LOW);
    digitalWrite(RELAY2, HIGH);
    Serial.println("CLOSE SRWITCH ");

    DoorInOperationFlag = millis() + DuracionAperturams + 1000;
    digitalWrite(SWITCH_PIN, LOW);

    IsDoorBeingClosed = true;
  }


  //CIERRE PESTILLO X SERVO Y CUTOFF DE MOTOR EN CIERRE
  if (IsDoorBeingClosed == true && (DoorInOperationFlag - 700) < millis()) {
    miServo.write(ServoClosedPos);
    Serial.println("Pestillo echado");
    delay(800);
    digitalWrite(RELAY1, HIGH);
    digitalWrite(RELAY2, HIGH);

    Serial.println("PARE TRAS CIERRE");
    digitalWrite(SWITCH_PIN, LOW);

    IsDoorClosed = true;
    IsDoorBeingClosed = false;
  }
}




void setup() {
  Serial.begin(115200);
  delay(200);
  setupPINS();
  pinMode(SWITCH_PIN, OUTPUT);
  digitalWrite(SWITCH_PIN, LOW);

  // Connect to Wi-Fi
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected.");

  // Set callback function to receive switch events
  mySwitch.onPowerState(onSwitchState);

  // Begin SinricPro
  SinricPro.begin(APP_KEY, APP_SECRET);
  // Restore last state from the server
  SinricPro.restoreDeviceStates(true);

  IsDoorClosed = !SinricswitchState;
  digitalWrite(SWITCH_PIN, SinricswitchState ? HIGH : LOW);

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
}

void loop() {
  // Handle Sinric Pro commands
  SinricPro.handle();
  LoopDoorLogic();
  //NO SCANEAR CUANDO HAYA OPERACIONES + margen de seguridad
  if (millis() > (DoorInOperationFlag + 2000)) {
    bleScanAndDetect();
  }


  //GESTIONAR EL USO DEL BOTON DERECHO DE LA ESP32 COMO SWITCH PARA ACTIVAR LA PUERTA
  // Check if the EN button is pressed
  if (digitalRead(0) == LOW) {
    SinricswitchState = !SinricswitchState;           // Toggle the switch state
    mySwitch.sendPowerStateEvent(SinricswitchState);  // Notify Sinric Pro about the state change
    Serial.print("EN button pressed. Switch state changed to: ");
    Serial.println(SinricswitchState ? "ON" : "OFF");
    delay(450);  // Debounce delay to avoid multiple toggles when the button is held
  }
}
