#include <SoftwareSerial.h>
#define RxD 2
#define TxD 3

#include <Wire.h>
#include "rgb_lcd.h"

rgb_lcd lcd;

const int colorR = 255;
const int colorG = 125;
const int colorB = 0;

#define MASTER 1

SoftwareSerial blueToothSerial(RxD,TxD);

char recv_str[100];

void print_msg(int row, char* msg) {
  lcd.setCursor(0, row);
  lcd.print(msg);
}

void setup() 
{ 
    lcd.begin(16, 2);
    lcd.setRGB(colorR, colorG, colorB);
    print_msg(0,"Starting...");
    int count = 0;

    Serial.begin(115200);   //Serial port for debugging
    pinMode(RxD, INPUT);    //UART pin for Bluetooth
    pinMode(TxD, OUTPUT);   //UART pin for Bluetooth
    Serial.println("\r\nPower on!!");
    if(setupBlueToothConnection() != 0) while(1){};   //initialize Bluetooth
    //this block is waiting for connection was established.
    
    print_msg(0,"Connexion en");
    while(1)
    {
      count ++;
      if(count == 0){
        print_msg(1,"cours.  ");
      }else if(count == 1){
        print_msg(1,"cours.. ");
      }else {
        print_msg(1,"cours...");
        count = -1;
      }
      
      if(recvMsg(1000) == 0)
      {
        if(strcmp((char *)recv_str, (char *)"OK+CONB") == 0)
        {
          print_msg(1,"       ");
          print_msg(0,"Connexion OK");
          break;
        }
      }
      delay(200);
    }
} 

void loop() 
{ 
  #if MASTER  //central role
  //in master mode, the bluetooth send message periodically. 
  delay(400);
  Serial.println("send: hi");
  blueToothSerial.print("hi");
  delay(100);
  //get any message to print
  if(recvMsg(1000) == 0)
  {
    Serial.print("recv: ");
    Serial.print((char *)recv_str);
    Serial.println("");
  }
  #else   //peripheral role
  delay(200);
  //the slave role only send message when received one.
  if(recvMsg(1000) == 0)
  {
    Serial.print("recv: ");
    Serial.print((char *)recv_str);
    Serial.println("");
    Serial.println("send: hello");
    blueToothSerial.print("hello");//return back message
  }
  #endif
}

//used for compare two string, return 0 if one equals to each other
int strcmp(char *a, char *b)
{
  unsigned int ptr = 0;
  while(a[ptr] != '\0')
  {
    if(a[ptr] != b[ptr]) return -1;
    ptr++;
  }
  return 0;
}

//configure the Bluetooth through AT commands
int setupBlueToothConnection()
{
  #if MASTER
  Serial.println("this is MASTER\r\n");
  #else
  Serial.println("this is SLAVE\r\n");
  #endif

  Serial.print("Setting up Bluetooth link\r\n");
  delay(3500);//wait for module restart

  //send command to module in different baud rate
  while(1)
  {
    delay(500);
    blueToothSerial.begin(9600);
    delay(500);
    Serial.print("try 9600\r\n");
    if(sendBlueToothCommand("AT") == 0)
      break;
    delay(500);
    blueToothSerial.begin(115200);
    delay(500);
    Serial.print("try 115200\r\n");
    if(sendBlueToothCommand("AT") == 0)
      break;
  }
  
  //we have to set the baud rate to 9600, since the soft serial is not stable at 115200
  sendBlueToothCommand("AT+RENEW");//restore factory configurations
  sendBlueToothCommand("AT+BAUD2");//reset the module's baud rate
  sendBlueToothCommand("AT+AUTH1");//enable authentication
  sendBlueToothCommand("AT+RESET");//restart module to take effect
  blueToothSerial.begin(9600);//reset the Arduino's baud rate
  delay(3500);//wait for module restart
  //configure parameters of the module
  sendBlueToothCommand("AT+VERS?");//get firmware version
  sendBlueToothCommand("AT+ADDE?");//get EDR MAC
  sendBlueToothCommand("AT+ADDB?");//get BLE MAC
  sendBlueToothCommand("AT+NAMEHM-13-EDR");//set EDR name
  sendBlueToothCommand("AT+NAMBHM-13-BLE");//set BLE name
  sendBlueToothCommand("AT+PINE123451");//set EDR password
  sendBlueToothCommand("AT+PINB123451");//set BLE password
  sendBlueToothCommand("AT+SCAN0");//set module visible
  sendBlueToothCommand("AT+NOTI1");//enable connect notifications
  //sendBlueToothCommand("AT+NOTP1");//enable address notifications
  sendBlueToothCommand("AT+PIO01");//enable key function
  #if MASTER
  sendBlueToothCommand("AT+ROLB1");//set to master mode
  #else
  sendBlueToothCommand("AT+ROLB0");//set to slave mode
  #endif
  sendBlueToothCommand("AT+RESET");//restart module to take effect
  delay(3500);//wait for module restart
  if(sendBlueToothCommand("AT") != 0) return -1;//detect if the module exists
  Serial.print("Setup complete\r\n\r\n");
  return 0;
}

//send command to Bluetooth and return if there is a response
int sendBlueToothCommand(char command[])
{
  Serial.print("send: ");
  Serial.print(command);
  Serial.println("");

  blueToothSerial.print(command);
  delay(200);

  if(recvMsg(200) != 0) return -1;

  Serial.print("recv: ");
  Serial.print(recv_str);
  Serial.println("");
  return 0;
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