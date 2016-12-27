#include <LowPower.h>
#include <BH1750.h>
#include <SFE_BMP180.h>
#include <DHT.h>
#include <Wire.h>
#include "Sodaq_DS3231.h"
#include <SD.h>
#include <SPI.h>

#include <Servo.h>


#define DHTPIN 2
#define DHTTYPE DHT22   // DHT 22  (AM2302)

#define ALTITUDE 216.0 // Altitude in Sparta, Greece
char TEMPERATURE = 'C' ;  //Change it to F to log degrees Fahrenheit
int INTERVAL = 1; //interval of measurements in minutes
DateTime dt(2015, 12, 14, 9, 28, 40, 1);  //Date and time

int CS_PIN = 10;
int ledPin = 7;
int POWERPIN = 3;

File file;
DHT dht(DHTPIN,DHTTYPE);
SFE_BMP180 pressureSensor;
BH1750 lightMeter;
int id = 0;

Servo myServo; // creates a servo object to control payload drop
int pos = 0;

void setup () 
{
   pinMode(POWERPIN, OUTPUT);
   digitalWrite(POWERPIN,HIGH);
   pinMode(ledPin, OUTPUT);
   initializeSD();
   myServo.attach(9);  // attaches the servo on pin 9 to the servo object 
   payloadShut = true; //changes once payload opens. not sure if this is the best place to put this variable.

  // Get the baseline pressure and write it to file on SD card:
  // Because pressure also varies with weather, you must first take a pressure
  // reading at a known baseline altitude. Then you can measure variations
  // from that pressure
  
  int baseline;
  String baselineEntry;
  
  baseline = getPressure();
  baselineEntry = baseline;
  
  writeEntryToFile("baseline pressure:,"+baselineEntry+"mb");
  delay(2000);

   
  //rtc.setDateTime(dt); //Adjust date-time as defined 'dt' above 
}

void loop () 
/*Not sure if this is the right way to think about it, but it seems the 
  below code block is what runs every time we loop through the code. The 
  code below this block is what defines all the functions run in this main block.*/
{
    String temperature;
    String humidity;
    String light;
    String pressure;
    String entryId;
    String dateEntry;
    id++;
    
    digitalWrite(POWERPIN,HIGH);
    digitalWrite(ledPin,HIGH);
    
    delay(2000);
    initSensors();
    delay(10000);
    
    entryId = String(id);
    humidity = String(dht.readHumidity());
    if(TEMPERATURE =='F')
    {
      temperature = String(dht.readTemperature(true));
    }else
    {
      temperature = String(dht.readTemperature());   
    }
    
    pressure = readPressure();
    light = readLight();
    dateEntry = DateLogEntry();

    String entry = entryId+","+dateEntry+","+temperature+","+humidity+","+pressure+","+light;    
    writeEntryToFile(entry);
    delay(2000);
    sleepForMinutes(INTERVAL);
    
}

void initSensors()
{
   
   Wire.begin();
   rtc.begin();
   dht.begin();
   // Initialize the pressure sensor storing calibration values on device.
   pressureSensor.begin();
   lightMeter.begin();
}

void sleepForMinutes(int interval)
{
  int i=0;
  int seconds = interval*60;
  int iterations = seconds/8;
    for (byte i = 0; i <= A5; i++)
    {
    pinMode (i, OUTPUT);    // changed as per below
    digitalWrite (i, LOW);  //     ditto
    }  
  
  digitalWrite(POWERPIN,LOW);
  for(i=0;i<iterations;i++)
  {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }  
}


float readPressureFromSensor()
{
  char status;
  double T,P,p0,a;
  
  // You must first get a temperature measurement to perform a pressure reading.
  
  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.
  
  status = pressureSensor.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    
    delay(status);
    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Use '&T' to provide the address of T to the function.
    // Function returns 1 if successful, 0 if failure.
    
    status = pressureSensor.getTemperature(T);
    if (status != 0)
    { 
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.
      
      status = pressureSensor.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Use '&P' to provide the address of P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.
        
        status = pressureSensor.getPressure(P,T);
        if (status != 0)
        {
          p0 = pressureSensor.sealevel(P,ALTITUDE);       
          return p0;
        }
        //could add error messages here if we wanted (e.g. 'else Serial.println("error retrieving pressure measurement\n"');
      }
    }
  }
}

String DateLogEntry()
{
  
  String dateEntry;
  String year;
  String month;
  String day;
  String hour;
  String minute;
  
  DateTime now = rtc.now();
  year = String(now.year());
  month = String(now.month());
  day = String(now.date());

  if(now.hour()<10)
  {
    hour = "0"+String(now.hour());
  }else
  {
    hour = String(now.hour());
  }

    if(now.minute()<10)
  {
    minute = "0"+String(now.minute());
  }else
  {
    minute = String(now.minute());
  }
 
  dateEntry = month+"/"+day+"/"+year+" "+hour+":"+minute;
  return dateEntry;
}

String readLight()
{
  uint16_t lux = lightMeter.readLightLevel();
  return String(lux);
}

String readPressure()
{
  String pressure;
  pressure = String(readPressureFromSensor());
  return pressure;
}

void initializeSD()
{
  pinMode(10, OUTPUT); // change this to 53 on a mega  // don't follow this!!
  digitalWrite(10, HIGH); // Add this line
  if (SD.begin(CS_PIN))
  {
    digitalWrite(ledPin,LOW);
  } else
  {
    digitalWrite(ledPin,LOW);
    delay(1000);
    digitalWrite(ledPin,HIGH);
    delay(1000);
     digitalWrite(ledPin,LOW);
    delay(1000);
    digitalWrite(ledPin,HIGH);
    return;
  }
}

int openFileToWrite(char filename[])
{
  file = SD.open(filename, FILE_WRITE);

  if (file)
  {
    digitalWrite(ledPin,HIGH);
    delay(200);
    digitalWrite(ledPin,LOW);
    delay(200);
    digitalWrite(ledPin,HIGH);
    delay(200);
    digitalWrite(ledPin,LOW);
    return 1;
  } else
  {
    return 0;
  }
}

int writeToFile(String text)
{
  if (file)
  {
    file.println(text);
    return 1;
  } else
  {
    return 0;
  }
}

void closeFile()
{
  if (file)
  {
    file.close();
  }
}

void writeEntryToFile(String entry)
{
  openFileToWrite("log.txt");
  writeToFile(entry);
  closeFile();
}

void Payload();
{ //probably should only run this when boolean is true so it doesn't try to open the doors more than once
  if (payloadShut == true)
  { 
    int currentPressure;
    currentPressure = readPressure();
    
    int deltaPressure = baseline - currentPressure;
    if (deltaPressure >= 957); //should be the change in pressure equal to gaining 65,000ft of elevation 95685pa = 957mb
    {
     for (pos = 0; pos <= 180; pos += 1)// goes from 0 degrees to 180 degrees in steps of 1 degree
      { 
        myServo.write(pos);              // tell servo to go to position in variable 'pos'
        delay(15);                       // waits 15ms for the servo to reach the position
      }
     payloadShut == false
    }
  }
}
  int currentPressure 
  currentPressure = readPressure()
  if 
}

