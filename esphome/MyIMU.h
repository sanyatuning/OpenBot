// green
#define I2C_SDA 12
// yellow
#define I2C_SCL 14

#include <Adafruit_9DOF.h>
#include <MadgwickAHRS.h>

class MyIMU : public Component {
 static const char *TAG;
 static const unsigned int calibration_length = 100;
 static MyIMU *instance;

 Adafruit_L3GD20_Unified gyro = Adafruit_L3GD20_Unified(20);
 Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(30301);
 Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(30302);
 bool accelOK = false;
 bool gyroOK = false;
 bool magOK = false;
 unsigned long beginInterval, nextBegin;
 unsigned long microsPerReading, microsPrevious;


 TaskHandle_t Task1;
 unsigned int calibration_index = 0;
 sensors_vec_t calib_array_accel[calibration_length];
 sensors_vec_t calib_array_gyro[calibration_length];
 sensors_vec_t calib_zero_accel;
 sensors_vec_t calib_zero_gyro;


 public:

 sensors_event_t event_gyro;
 sensors_event_t event_accel;
 sensors_event_t event_mag;

 SemaphoreHandle_t mutex1 = NULL;
 Madgwick filter = Madgwick();

  void setup() override {
    this->instance = this;

    Wire.begin(I2C_SDA, I2C_SCL, 0);

    beginInterval = 5000;
    nextBegin = millis() + beginInterval;

    gyro.enableAutoRange(true);
    initSensors();

    filter.begin(25);
    microsPerReading = 1000000 / 25;
    microsPrevious = micros();

    vSemaphoreCreateBinary(mutex1);
    if (mutex1 != NULL) {
      xTaskCreatePinnedToCore(
          task1,   /* Function to implement the task */
          "IMU",   /* Name of the task */
          10000,   /* Stack size in words */
          NULL,    /* Task input parameter */
          0,       /* Priority of the task */
          &Task1,  /* Task handle. */
          0        /* Core where the task should run */
      );
    }
  }

  void initSensors() {
    if (!accelOK) {
      delay(100);
      accelOK = accel.begin();
      ESP_LOGD(TAG, "init accel: %d", accelOK);
    }
    if (!gyroOK) {
      delay(100);
      gyroOK = gyro.begin();
      ESP_LOGD(TAG, "init gyro: %d", gyroOK);
    }
//    if (!magOK) {
//      delay(100);
//      magOK = mag.begin();
//      ESP_LOGD(TAG, "init mag: %d", magOK);
//    }
  }

  void dump_config() override {
    ESP_LOGCONFIG(TAG, "accel: %d", accelOK);
    ESP_LOGCONFIG(TAG, "gyro: %d", gyroOK);
//    ESP_LOGCONFIG(TAG, "mag: %d", magOK);
    ESP_LOGCONFIG(TAG, "gyro_zero: %.3f %.3f %.3f", calib_zero_gyro.x, calib_zero_gyro.y, calib_zero_gyro.z);
    calibration_index = 0;

//    write8(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_MR_REG_M, 0x00);
//    uint8_t reg1_a = read8(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_CRA_REG_M);
//    ESP_LOGCONFIG(TAG, "id: %d", reg1_a);
  }

  static void task1(void * parameter) {
    for(;;) {
      instance->loop();
    }
  }

  void loop() {
    if (millis() > nextBegin) {
      initSensors();
      nextBegin += beginInterval;
    }
    if (micros() - microsPrevious < microsPerReading) {
      return;
    }

    xSemaphoreTake(mutex1, portMAX_DELAY);
    gyro.getEvent(&event_gyro);
    accel.getEvent(&event_accel);
//    mag.getEvent(&event_mag);

    if (calibration_index < calibration_length) {
      calib_loop();
    } else {
//      vec_calib(&event_accel.acceleration, calib_zero_accel);
      vec_calib(&event_gyro.gyro, calib_zero_gyro);
    }

    filter.update(
      event_gyro.gyro.x / 0.0174533f,
      event_gyro.gyro.y / 0.0174533f,
      event_gyro.gyro.z / 0.0174533f,
      event_accel.acceleration.x,
      event_accel.acceleration.y,
      event_accel.acceleration.z,
      0.0f, 0.0f, 0.0f
//      event_mag.magnetic.x, event_mag.magnetic.y, event_mag.magnetic.z
    );
    xSemaphoreGive(mutex1);
    microsPrevious += microsPerReading;
  }

  void calib_loop() {
    calib_array_accel[calibration_index] = event_accel.acceleration;
    calib_array_gyro[calibration_index] = event_gyro.gyro;

    calibration_index++;
    if (calibration_index == calibration_length) {
      calib_zero_accel = vec_avg(calib_array_accel, calibration_length);
      calib_zero_gyro = vec_avg(calib_array_gyro, calibration_length);

      ESP_LOGD(TAG, "calibration done");
      ESP_LOGD(TAG, "accel: %.3f %.3f %.3f", calib_zero_accel.x, calib_zero_accel.y, calib_zero_accel.z);
      ESP_LOGD(TAG, "gyro: %.3f %.3f %.3f", calib_zero_gyro.x, calib_zero_gyro.y, calib_zero_gyro.z);
    }
  }

  sensors_vec_t vec_avg(sensors_vec_t calib_array[], unsigned int length) {
    sensors_vec_t calib_zero;
    calib_zero.x = 0.0f;
    calib_zero.y = 0.0f;
    calib_zero.z = 0.0f;
    for (unsigned int index = 0; index < length; index++) {
      calib_zero.x += calib_array[index].x;
      calib_zero.y += calib_array[index].y;
      calib_zero.z += calib_array[index].z;
    }
    calib_zero.x /= length;
    calib_zero.y /= length;
    calib_zero.z /= length;

    return calib_zero;
  }

  void vec_calib(sensors_vec_t *what, sensors_vec_t val) {
    what->x -= val.x;
    what->y -= val.y;
    what->z -= val.z;
    return;
    if (what->x > -0.01 && what->x < 0.01) {
      what->x = 0.0f;
    }
    if (what->y > -0.01 && what->y < 0.01) {
      what->y = 0.0f;
    }
    if (what->z > -0.01 && what->z < 0.01) {
      what->z = 0.0f;
    }
  }

  void i2cScan() {
    byte error, address;
    int nDevices;

    ESP_LOGD("OpenBot", "Scanning...");

    nDevices = 0;
    for (address = 1; address < 127; address++) {
      // The i2c_scanner uses the return value of
      // the Write.endTransmisstion to see if
      // a device did acknowledge to the address.
      Wire.beginTransmission(address);
      error = Wire.endTransmission();

      if (error == 0) {
        ESP_LOGD(TAG, "I2C device found at address 0x%2X", address);
        if (address == 0x69) {
          uint8_t id = read8(address, GYRO_REGISTER_WHO_AM_I);

          ESP_LOGD(TAG, "ID 0x%2X", id);
        }



        nDevices++;
      } else if (error == 4)  {
        ESP_LOGD(TAG, "Unknown error at address 0x%2X", address);
      }
    }
    if (nDevices == 0)
      ESP_LOGD(TAG, "No I2C devices found");
    else
      ESP_LOGD(TAG, "Scan done");
  }

  byte read8(byte addr, byte reg) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(addr, (byte)1);
    return Wire.read();
  }

  void write8(byte address, byte reg, byte value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
  }
};

const char *MyIMU::TAG = "MyIMU";
MyIMU *MyIMU::instance = NULL;
