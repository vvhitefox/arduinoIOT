#include <Wire.h>
#include <ArduinoJson.h>
#include "LSM6DS3.h"
#include <SoftwareSerial.h>   //Software Serial Port
#define RxD 2
#define TxD 3

#define CLEAR_STEP      true
#define NOT_CLEAR_STEP  false

#define MASTER 0    //change this macro to define the Bluetooth as Master or not 

SoftwareSerial blueToothSerial(RxD,TxD);//the software serial port 
LSM6DS3 pedometer(I2C_MODE, 0x6A); 
StaticJsonBuffer<200> jsonBuffer;

char recv_str[100];

void setup() 
{ 
    Serial.begin(115200);   //Serial port for debugging
    blueToothSerial.begin(9600);
    Wire.begin();
    pedometer.begin();
    pinMode(RxD, INPUT);    //UART pin for Bluetooth
    pinMode(TxD, OUTPUT);   //UART pin for Bluetooth

    if (0 != config_pedometer(NOT_CLEAR_STEP)) {
      Serial.println("Configure pedometer fail!");
    }
    Serial.println("Success to Configure pedometer!");
} 

void loop() 
{  
  uint8_t dataByte = 0;
  uint16_t stepCount = 0;

  delay(200);
  //get any message to print
  if(recvMsg(5000) == 0){
    Wire.requestFrom(0xA0 >> 1, 1);
    while(Wire.available()) {          // slave may send less than requested
      unsigned char c = Wire.read();   // receive heart rate value (a byte)
      Serial.print("Heart rate: ");
      Serial.println(c, DEC);         // print heart rate value
      pedometer.readRegister(&dataByte, LSM6DS3_ACC_GYRO_STEP_COUNTER_H);
      stepCount = (dataByte << 8) & 0xFFFF;

      pedometer.readRegister(&dataByte, LSM6DS3_ACC_GYRO_STEP_COUNTER_L);
      stepCount |=  dataByte;

      Serial.print("Step: ");
      Serial.println(stepCount);
      String json = "{\"step\":"+ String(stepCount) +",\"heart_rate\":"+ String(c)+ "}"; 
      blueToothSerial.print(json);
    }
  }
}

//receive message from Bluetooth with time out
int recvMsg(unsigned int timeout)
{
    //wait for feedback
    unsigned int time = 0;
    unsigned char num;
    unsigned char i;
    
    //waiting for the first character with time out
    i = 0;
    while(1)
    {
        delay(50);
        if(blueToothSerial.available())
        {
            recv_str[i] = char(blueToothSerial.read());
            i++;
            break;
        }
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

    //Setup the accelerometer******************************
    dataToWrite = 0;

    //  dataToWrite |= LSM6DS3_ACC_GYRO_BW_XL_200Hz;
    dataToWrite |= LSM6DS3_ACC_GYRO_FS_XL_2g;
    dataToWrite |= LSM6DS3_ACC_GYRO_ODR_XL_26Hz;


    // Step 1: Configure ODR-26Hz and FS-2g
    errorAccumulator += pedometer.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL, dataToWrite);

    // Step 2: Set bit Zen_G, Yen_G, Xen_G, FUNC_EN, PEDO_RST_STEP(1 or 0)
    if (clearStep) {
        errorAccumulator += pedometer.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, 0x3E);
    } else {
        errorAccumulator += pedometer.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, 0x3C);
    }

    // Step 3:	Enable pedometer algorithm
    errorAccumulator += pedometer.writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG1, 0x40);

    //Step 4:	Step Detector interrupt driven to INT1 pin, set bit INT1_FIFO_OVR
    errorAccumulator += pedometer.writeRegister(LSM6DS3_ACC_GYRO_INT1_CTRL, 0x10);

    return errorAccumulator;
}