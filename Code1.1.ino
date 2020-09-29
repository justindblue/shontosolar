//For SD Card reader:
#include <SdFat.h>                            // https://github.com/greiman/SdFat/
#include  <SPI.h>
SdFat SD;
#define MOSIpin 11
#define MISOpin 12
const int chipSelect = 10;

//For RTC (Real Time Clock): 
#include <Wire.h>
#include <RTClib.h>
RTC_DS1307 RTC;                               // The real time clock object is "RTC"
#define DS1307_I2C_ADDRESS 0x68
char tmeStrng[ ] = "0000/00/00,00:00:00";     // a template for a data/time string
long utc;
int logSeconds = 30;                           // ****** Enter ****** the number of seconds between logging events
long logMillis = logSeconds * 1000;

//For Temp Sensors:
#include <OneWire.h> 
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 3 
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
DeviceAddress Probe01 = { 0x28, 0xAC, 0x84, 0x26, 0x00, 0x00, 0x80, 0x1E }; 
DeviceAddress Probe02 = { 0x28, 0xFF, 0x3D, 0x73, 0xB0, 0x16, 0x04, 0x9B };
DeviceAddress Probe03 = { 0x28, 0x16, 0xB1, 0x79, 0x97, 0x07, 0x03, 0x90 };

double temp_1 = 0;
double temp_2 = 0;
double temp_3 = 0;

//For AmpsRMS( Power Meter):
const int sensorIn = A0;
int mVperAmp = 66; // use 185 fo 10A Module and use 100 for 20A Module and 66 for 30A Module
double VRMS = 0;
double AmpsRMS = 0;  
double Voltage = 0;

//For Voltage Sensor of Battery:
const int batteryIn = A1;
double volt_1 = 0;
double Battery = 0;

//For Voltage 2 Solar Cell: 
const int In_2 = A7;
float volt_2 = 0;
float int_2 = 0;

//For Voltage 3 Solar Panel:
const int In_3 = A3;
double volt_3 = 0;
double int_3 = 0;
                    
void setup() {

  //For RTC
  Wire.begin();                    // initialize the I2C interface
  RTC.begin();                     // initialize the RTC 
// Uncomment the line below and load the sketch to the Nano to set the RTC time. Then the line must 
  // be commented out and the sketch loaded again or the time will be wrong.
//RTC.adjust(DateTime((__DATE__), (__TIME__)));    //sets the RTC to the time the sketch was compiled.
  
  //Intializes Temp sensors:
  sensors.begin(); 
  sensors.setResolution(Probe01, 10);
  sensors.setResolution(Probe02, 10);
  sensors.setResolution(Probe03, 10);

  //Intializes serial monitor:
  Serial.begin(9600);
  while (!Serial) {
    ;                    
  }

  //Intializes SD card:
  if (!SD.begin(chipSelect)) {
    while(1);
  }
 
  delay(2000);
}


void loop() 
{
  //For RTC
   DateTime now = RTC.now();                                     // read the time from the RTC
   utc = (now.unixtime());  
   sprintf(tmeStrng, "%04d/%02d/%02d,%02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()); // [added seconds]
  
  //print RTC info:
  Serial.print("RTC utc Time: ");
  Serial.print(utc);
  Serial.print("\t");
  Serial.print("RTC time: ");
  Serial.print(tmeStrng);
  Serial.print("\t");

  //For AmpsRMS(Power Meter):
  Voltage = getVPP();
  VRMS = (Voltage/2.0) *0.707; 
  AmpsRMS = (VRMS * 1000)/mVperAmp;

  //Prints AmpsRMS info:
  Serial.print("Current for surge protector: ");
  Serial.print(AmpsRMS);
  Serial.print("\t");

  //For Temp sensors:
  sensors.requestTemperatures(); // Send the command to get temperature readings
  temp_1 = sensors.getTempC(Probe01);
  temp_2 = sensors.getTempC(Probe02);
  temp_3 = sensors.getTempC(Probe03);

  //Prints Temp info:
  Serial.print("Probe 1(Ox0E): ");
  Serial.print(temp_1);
  Serial.print("\t");
  Serial.print("Probe 2(0xA1): ");
  Serial.print(temp_2);
  Serial.print("\t");
  Serial.print("Probe 3(0xA1): ");
  Serial.print(temp_3);
  Serial.print("\t");

  //For voltage sensor for Battery:
  Battery = analogRead(batteryIn);
  volt_1 = (Battery*((118500 + 22200)/(22200))*5)/1024;

  //Prints voltage of battery info:
  Serial.print("Voltage 1 Batteries(A1): ");
  Serial.print(volt_1);
  Serial.print("\t");
   
  //For Solar Cell:
  int_2 = analogRead(In_2);
  volt_2 = (int_2 * 5)/1024;

  //For Solar Panel:
  int_3 = analogRead(In_3);
  volt_3 = (int_3 *((82100 + 3700)/(3700)) *5)/1024;

  //Prints voltage for Solar Cell
  Serial.print("Voltage 2 Solar Cell(A2): ");
  Serial.print(volt_2, 4);
  Serial.print("\t");

  //Prints voltage for Solar Panel
  Serial.print("Voltage 3 solar Panel(A3): ");
  Serial.println(volt_3);

  // write the data to the SD card:                                                                 
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  dataFile.println(String(utc) + "," + String(tmeStrng) + "," + String(AmpsRMS) + "," + String(temp_1, 3) + "," + String(temp_2) + "," + String(temp_3) + "," + String(volt_1) + "," + String(volt_2, 4) + "," + String(volt_3));
  dataFile.flush();
  dataFile.close();

  delay(logMillis);
}    


float getVPP()
{
  float result;
  
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
  int minValue = 1024;          // store min value here
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(sensorIn);
       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
       if (readValue < minValue) 
       {
           /*record the maximum sensor value*/
           minValue = readValue;
       }
   }
   
   // Subtract min from max
   result = ((maxValue - minValue) * 5.0)/1024.0;
   
   return result;
 }
