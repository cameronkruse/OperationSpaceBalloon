#include <LowPower.h>
#include <BH1750.h>
#include <SFE_BMP180.h>
#include <DHT.h>
#include <Wire.h>
#include "Sodaq_DS3231.h"
#include <SD.h>
#include <SPI.h>
#include <SoftwareServo.h> 

#define DHTPIN 2
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define pinServo A4  //servo pin for payload dropping (analog 4 hopefully)
#define ALTITUDE 216.0 // Altitude in Sparta, Greece

char TEMPERATURE = 'F' ;  //Change it to F to log degrees Fahrenheit
int INTERVAL = 1; //interval of measurements in minutes
DateTime dt(2016, 12, 29, 10, 40, 0, 0);  //Date and time

int CS_PIN = 10;
int ledPin = 7;
int POWERPIN = 3;
int potpin = 0; //analog pin used to connect the potentiometer (hopefully) 
int val; //variable to read the value from the analog pin (hopefully)

//Servo stuff below
int speed = 1; 
int limits[2] = {30,150};  
boolean refresh = false;  


File file;
DHT dht(DHTPIN,DHTTYPE);
SFE_BMP180 pressureSensor;
BH1750 lightMeter;
int id = 0;

void setup () 
{
   pinMode(POWERPIN, OUTPUT);
   digitalWrite(POWERPIN,HIGH);
   pinMode(ledPin, OUTPUT);
   initializeSD();
   
   //rtc.setDateTime(dt); //Adjust date-time as defined 'dt' above 
    
   //Servo stuff below
   Serial.begin(9600);
   // attaches the servo on pin to the servo object
   myservo.attach(pinServo);  
   // init angle of servo inbetween two limitations
   myservo.write((limits[1]-limits[0])/2); 
}

void loop () 
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

  status = pressureSensor.startTemperature();
  if (status != 0)
  {
    delay(status);
    status = pressureSensor.getTemperature(T);
    if (status != 0)
    { 
      status = pressureSensor.startPressure(3);
      if (status != 0)
      {
        delay(status);
        status = pressureSensor.getPressure(P,T);
        if (status != 0)
        {
          p0 = pressureSensor.sealevel(P,ALTITUDE);       
          return p0;
        }
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


//servo example loop below
void loop() 
{ 
  // refresh angle
  int angle = myservo.read();

  // change direction when limits
  if (angle >= limits[1] || angle <= limits[0])  speed = -speed;

  myservo.write(angle + speed); 

  // set refresh one time / 2
  refresh = refresh ? false : true;
  if (refresh) SoftwareServo::refresh();

  Serial.print("Angle: ");
  Serial.println(angle);
} 
