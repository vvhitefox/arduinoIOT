#include <Wire.h>
#include "LSM6DS3.h"
#include <SoftwareSerial.h>

#define CLEAR_STEP      true
#define NOT_CLEAR_STEP  false

SoftwareSerial blueToothSerial(2,3); // init bluetooth

LSM6DS3 pedometer(I2C_MODE, 0x6A);   // init pedometer

char recv_str[100];                  // msg received by raspberry
int trame = 0;

void setup() 
{ 
  Serial.begin(9600);
    
  blueToothSerial.begin(9600);
  pinMode(2, INPUT);
  pinMode(3, OUTPUT);

  Wire.begin();
  pedometer.begin();

  if (0 != config_pedometer(NOT_CLEAR_STEP)) {
    Serial.println("Configure pedometer fail!");
  }
  Serial.println("End setup");
} 

void loop() 
{  
  uint8_t dataByte = 0;
  uint16_t stepCount = 0;

  Wire.requestFrom(0xA0 >> 1, 1);

  // heart rate
  unsigned char heart_rate = Wire.read();

  // pedometer
  pedometer.readRegister(&dataByte, LSM6DS3_ACC_GYRO_STEP_COUNTER_H);
  stepCount = (dataByte << 8) & 0xFFFF;

  pedometer.readRegister(&dataByte, LSM6DS3_ACC_GYRO_STEP_COUNTER_L);
  stepCount |=  dataByte;

  String json = "{\"step\":"+ String(stepCount) +",\"heart_rate\":"+ String(heart_rate)+"}"; 
  Serial.println(trame++);
  Serial.println(json);
  delay(200);
  blueToothSerial.println(json); 
            
}

//receive message from Bluetooth with time out
int recvMsg(unsigned int timeout)
{
  unsigned int time = 0;
  unsigned char num;
  unsigned char i;
  
  i = 0;
  while(1)
  {
    if(blueToothSerial.available())
    {
      recv_str[i] = char(blueToothSerial.read());
      i++;
      break;
    }
    delay(50);
    time++;
    if(time > (timeout / 50)) return -1;
  }

  //read other characters from uart buffer to string
  while(blueToothSerial.available() && (i < 100))
  {                                              
    recv_str[i] = char(blueToothSerial.read());
    i++;
  }
  recv_str[i] = '\0';


  return 0;
}

int config_pedometer(bool clearStep) {
    uint8_t errorAccumulator = 0;
    uint8_t dataToWrite = 0;  //Temporary variable

    dataToWrite = 0;

    dataToWrite |= LSM6DS3_ACC_GYRO_FS_XL_2g;
    dataToWrite |= LSM6DS3_ACC_GYRO_ODR_XL_26Hz;

    errorAccumulator += pedometer.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL, dataToWrite);

    if (clearStep) {
      errorAccumulator += pedometer.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, 0x3E);
    } else {
      errorAccumulator += pedometer.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, 0x3C);
    }

    errorAccumulator += pedometer.writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG1, 0x40);

    errorAccumulator += pedometer.writeRegister(LSM6DS3_ACC_GYRO_INT1_CTRL, 0x10);

    return errorAccumulator;
}