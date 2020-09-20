// ---------------------------------------------------------------------------
// This Arduino sketch accompanies the OpenBot Android application.
//
// The sketch has the following functinonalities:
//  - receive control commands from Android application (USB serial)
//. - produce low-level controls (PWM) for the vehicle
//  - toggle left and right indicator signals
//  - wheel odometry based on optical speed sensors
//  - estimate battery voltage via voltage divider
//  - estimate distance based on sonar sensor
//  - send sensor readings to Android application (USB serial)
//
// By Matthias Mueller, Intelligent Systems Lab, 2020
// ---------------------------------------------------------------------------


// PIN_PWM1,PIN_PWM2,PIN_PWM3,PIN_PWM4      Low-level control of left DC motors via PWM
// PIN_SPEED_L, PIN_SPEED_R          	      Measure left and right wheel speed
// PIN_VIN                                  Measure battery voltage via voltage divider
// PIN_TRIGGER                              Arduino pin tied to trigger pin on ultrasonic sensor.
// PIN_ECHO                                 Arduino pin tied to echo pin on ultrasonic sensor.
// MAX_DISTANCE                             Maximum distance we want to ping for (in centimeters).
// PIN_LED_RL, PIN_LED_RR                   Toggle left and right rear LEDs (indicator signals)


//------------------------------------------------------//
//DEFINITIONS
//------------------------------------------------------//

#define PIN_PWMA 16
#define PIN_PWMB 2

#define PIN_PWM1 27
#define PIN_PWM2 4
#define PIN_PWM3 32
#define PIN_PWM4 33
#define PIN_SPEED_L 36
#define PIN_SPEED_R 39
#define PIN_VIN 15
#define PIN_TRIGGER   25
#define PIN_ECHO      26
#define PIN_LED_RL 12
#define PIN_LED_RR 14


//------------------------------------------------------//
//INITIALIZATION
//------------------------------------------------------//

#include "esphome.h"
#include "analogWrite.h"
#include <Ultrasonic.h>

//Sonar sensor
const int MAX_DISTANCE = 300;
const int ping_interval = 100; // How frequently are we going to send out a ping (in milliseconds).

//Vehicle Control
int ctrl_left = 0;
int ctrl_right = 0;

//Voltage measurement
unsigned int counter_voltage = 0;
const unsigned int vin_array_sz = 100;
unsigned int vin_array[vin_array_sz]={0};

//Speed sensors
const unsigned long ignoremilli = 0;
unsigned long oldtime_left = 0;
unsigned long curtime_left = 0;
unsigned long oldtime_right = 0;
unsigned long curtime_right = 0;
int counter_left = 0;
int counter_right = 0;

//Indicator Signal
const unsigned long indicator_interval = 500;
unsigned long indicator_time = 0;
int indicator_val = 0;
bool ctrl_rx = 0;
bool indicator_rx = 0;

//Serial communication
const unsigned long send_interval = 1000;
unsigned long send_time = 0;
String inString = "";
String cmd = "";

//------------------------------------------------------//
//SETUP
//------------------------------------------------------//

class OpenBot : public Component {

 static OpenBot *instance;

 AddressableLight &it = *fastled_base_fastledlightoutput;
 AsyncWebSocket ws = AsyncWebSocket("/ws");
 HighFrequencyLoopRequester high_freq_;

 Ultrasonic *ultrasonic;
 unsigned int distance_cm = MAX_DISTANCE;
 unsigned long ping_time;

 public:

  void setup() override {
    this->instance = this;

    //Outputs
    pinMode(PIN_PWMA,OUTPUT);
    pinMode(PIN_PWM1,OUTPUT);
    pinMode(PIN_PWM2,OUTPUT);

    pinMode(PIN_PWMB,OUTPUT);
    pinMode(PIN_PWM3,OUTPUT);
    pinMode(PIN_PWM4,OUTPUT);

    pinMode(PIN_LED_RL,OUTPUT);
    pinMode(PIN_LED_RR,OUTPUT);

    //Inputs
    pinMode(PIN_VIN,INPUT);
    pinMode(PIN_SPEED_L,INPUT);
    pinMode(PIN_SPEED_R,INPUT);

    attachInterrupt(digitalPinToInterrupt(PIN_SPEED_L), speed_left, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_SPEED_R), speed_right, RISING);

    Serial.begin(115200,SERIAL_8N1); //8 data bits, no parity, 1 stop bit
    send_time = millis() + send_interval; //wait for one interval to get readings
    ping_time = millis();

    updateMotors();
    this->high_freq_.start();

    ultrasonic = new Ultrasonic(PIN_TRIGGER, PIN_ECHO);
    ultrasonic->setMaxDistance(MAX_DISTANCE);

//    ws.onEvent(this->onWsEvent);
//    web_server_base_webserverbase->add_handler(&this->ws);
  }

  void loop() override {

  	//Measure voltage
    vin_array[counter_voltage%vin_array_sz] = analogRead(PIN_VIN);
    counter_voltage++;

    //Measure distance every ping_interval
    if (millis() >= ping_time) {
      distance_cm = ultrasonic->read();
      ping_time += ping_interval;
    }

    // Write voltage to serial every send_interval
    if (millis() >= send_time) {
      sendVehicleData();
      send_time += send_interval;
    }

    // Check indicator signal every indicator_interval
    if (millis() >= indicator_time) {
      updateIndicator();
      indicator_time += indicator_interval;
    }

    if (Serial.available() > 0) {
      char inChar = Serial.read();
      processChar(inChar);
    }
  }

  void processString(String msg) {
    inString = "";
    for (int i = 0; i < msg.length(); i++) {
      processChar(msg[i]);
    }
    processChar('\n');
  }

  void processChar(char inChar) {
    cmd += inChar;
    // Serial.print(inChar);
    if (ctrl_rx) {
      processCtrlMsg(inChar);
    } else if (indicator_rx) {
      processIndicatorMsg(inChar);
    } else {
      checkForMsg(inChar);
    }
  }

  //------------------------------------------------------//
  // FUNCTIONS
  //------------------------------------------------------//

  static void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
    if(type == WS_EVT_CONNECT){
      ESP_LOGD("OpenBot", "ws[%s][%u] connect", server->url(), client->id());
      client->printf("Hello Client %u :)", client->id());
      client->ping();
    } else if(type == WS_EVT_DISCONNECT){
      ESP_LOGD("OpenBot", "ws[%s][%u] disconnect", server->url(), client->id());
    } else if(type == WS_EVT_ERROR){
      ESP_LOGD("OpenBot", "ws[%s][%u] error(%u): %s", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
    } else if(type == WS_EVT_PONG){
      ESP_LOGD("OpenBot", "ws[%s][%u] pong[%u]: %s", server->url(), client->id(), len, (len)?(char*)data:"");
    } else if(type == WS_EVT_DATA){
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      String msg = "";
      if(info->final && info->index == 0 && info->len == len){

        if(info->opcode == WS_TEXT){
          for(size_t i=0; i < info->len; i++) {
            msg += (char) data[i];
          }
        } else {
          char buff[3];
          for(size_t i=0; i < info->len; i++) {
            sprintf(buff, "%02x ", (uint8_t) data[i]);
            msg += buff ;
          }
        }
//        ESP_LOGD("OpenBot", "msg: %s",msg.c_str());

        instance->processString(msg);

//        if(info->opcode == WS_TEXT)
//          client->text("I got your text message");
//        else
//          client->binary("I got your binary message");
      }
    }
  }

  float getVoltage() {
    unsigned long array_sum = 0;
    unsigned int array_size = min(vin_array_sz,counter_voltage);
    for (unsigned int index = 0; index < array_size; index++) {
      array_sum += vin_array[index];
    }
    return float(array_sum)/array_size/1023*4.04*5;
  }

  static void speed_left() {
    // todo use instance
    curtime_left = millis();
    if( (curtime_left - oldtime_left) > ignoremilli ) {
      if (ctrl_left < 0) {
        counter_left--;
      }
      else {
        counter_left++;
      }
      oldtime_left = curtime_left;
    }
  }

  static void speed_right() {
    // todo use instance
    curtime_right = millis();
    if( (curtime_right - oldtime_right) > ignoremilli ) {
      if (ctrl_right < 0) {
        counter_right--;
      }
      else {
        counter_right++;
      }
      oldtime_right = curtime_right;
    }
  }

  void updateMotors() {
//    if (abs(ctrl_left - ctrl_right) > 50) {
//      if (ctrl_left > ctrl_right) {
//        ctrl_right = ctrl_left - 50;
//      } else {
//        ctrl_left = ctrl_right - 50;
//      }
//    }

    if (ctrl_left < 0) {
      analogWrite(PIN_PWMA,-ctrl_left);
      digitalWrite(PIN_PWM1,HIGH);
      digitalWrite(PIN_PWM2,LOW);
    }
    else if (ctrl_left > 0) {
      analogWrite(PIN_PWMA,ctrl_left);
      digitalWrite(PIN_PWM1,LOW);
      digitalWrite(PIN_PWM2,HIGH);
    }
    else { //Motor brake
      digitalWrite(PIN_PWM1,LOW);
      digitalWrite(PIN_PWM2,LOW);
    }
    if (ctrl_right < 0) {
      analogWrite(PIN_PWMB,-ctrl_right);
      digitalWrite(PIN_PWM3,HIGH);
      digitalWrite(PIN_PWM4,LOW);
    }
    else if (ctrl_right > 0) {
      analogWrite(PIN_PWMB,ctrl_right);
      digitalWrite(PIN_PWM3,LOW);
      digitalWrite(PIN_PWM4,HIGH);
    }
    else { //Motor brake
      digitalWrite(PIN_PWM3,LOW);
      digitalWrite(PIN_PWM4,LOW);
    }
  }

  void processCtrlMsg(char inChar) {
    // Serial.write(inChar);
    // comma indicates that inString contains the left ctrl
    if (inChar == ',') {
      ctrl_left = inString.toInt() * 0.95;
      // clear the string for new input:
      inString = "";
    } else if (inChar == '\n') {
      // new line indicates that inString contains the right ctrl
      ctrl_right = inString.toInt(); //* 0.95;
      // clear the string for new input:
      inString = "";
      // end of message
      ctrl_rx = false;

      updateMotors();
      cmd.trim();
      ESP_LOGD("OpenBot", cmd.c_str());
      cmd = "";
    } else {
      // As long as the incoming byte
      // is not a newline or comma,
      // convert the incoming byte to a char
      // and add it to the string
      inString += inChar;
    }
  }

  void processIndicatorMsg(char inChar) {
    // new line indicates that inString contains the indicator signal
    if (inChar == '\n') {
      indicator_val = inString.toInt();
      // clear the string for new input:
      inString = "";
      // end of message
      indicator_rx = false;
    }
    else {
      // As long as the incoming byte
      // is not a newline
      // convert the incoming byte to a char
      // and add it to the string
      inString += inChar;
    }
  }

  void checkForMsg(char inChar) {
    switch (inChar) {
      case 'c':
        ctrl_rx = true;
        break;
      case 'i':
        indicator_rx = true;
        break;
    }
  }

  void sendVehicleData() {
    Serial.print(getVoltage());
    Serial.print(",");
  //  Serial.print(counter_left);
    Serial.print(ctrl_left);
    Serial.print(",");
  //  Serial.print(counter_right);
    Serial.print(ctrl_right);
    Serial.print(",");
    Serial.print(distance_cm);
    Serial.println();
    counter_left = 0;
    counter_right = 0;

    ESP_LOGD("OpenBot", "L: %d R: %d D: %d", ctrl_left, ctrl_right, distance_cm);
  }

  void updateIndicator() {
    it[0] = ESPColor(0, 0, 0);
    it[1] = ESPColor(0, 0, 0);
    it[2] = ESPColor(0, 0, 0);
    it[3] = ESPColor(0, 0, 0);

    switch (indicator_val) {
      case -1:
        it[3] = ESPColor(100, 0, 0);
        break;
        break;
      case 0:
        break;
      case 1:
        it[0] = ESPColor(100, 0, 0);
        break;
    }
    it.schedule_show();
  }
};

OpenBot *OpenBot::instance = NULL;
