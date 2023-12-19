
/*********************************************************************************/
/*
 * Project "Water Quality Monitoring system"
 * 
 */
/*********************************************************************************/

#include <OneWire.h>
#include <Wire.h> 
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

const int rs = 8, en = 9, d4 = 10, d5 = 11, d6 = 12, d7 = 13;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

SoftwareSerial mySerial(6,7);   // gsm(rx,tx)
SoftwareSerial esp(4,5);        //ESP8266

/**************************************************************************************************************/

boolean thingSpeakWrite(float value1, float value2, float value3 );

String ssid="Doremon";                                 // Wifi network SSID
String password ="ammupattar@29";                         // Wifi network password
String apikey="C6OUS3TKXD2I0D9I";


boolean DEBUG=true;
int count=0;

/**************************************************************************************************************/
OneWire  ds(2);  // temperature PIN 2
byte i;
byte present = 0;
byte type_s;
byte data[12];
byte addr[8];
float celsius, fahrenheit;

/*********************************************************************************/
                        // on pin 2 (a 4.7K resistor is necessary)
int PH_Pin = A0 ;       //(PIN P0) pH meter Analog output to Arduino Analog Input 0
float PH;

/******************************************************************************/

int turbidity_pin = A1 ;
float voltage,turbidity ;

/******************************************************************************/

int gTEMP, gPH, gTURBIDITY ;

/**************************************************************************************************************/

void showResponse(int waitTime)
{
    long t=millis();
    char c;
    while (t+waitTime>millis())
    {
      if (esp.available())
      {
        c = esp.read();
        if (DEBUG) 
        {
          Serial.print(c);
        }
      }
    }               
}

/****************************************************************************************************************/

void setup()
{ 
      lcd.begin(16, 2);
      Serial.begin(9600);
      mySerial.begin(9600);           // Setting the baud rate of GSM Module  
      esp.begin(115200);
      pinMode(turbidity_pin,INPUT);
      delay(200);

       esp.println("AT+CWMODE=1");                                             // set esp8266 as client
       showResponse(1000);
      
       esp.println("AT+CWJAP=\""+ssid+"\",\""+password+"\"");                  // set your home router SSID and password
       showResponse(5000);
       Serial.println();
       if (DEBUG)
       {
          Serial.println("Setup completed");
       }
       
      Serial.println();
      Serial.println("WATER QUALITY MONITORING SYSTEM");
      Serial.println();
      Serial.println("**********************************************************************");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WATER QUALITY");
      lcd.setCursor(0, 1);
      lcd.print("MONITORING SYSTEM");
      delay(2000);      
} 

/**********************************************************************************************/

boolean thingSpeakWrite( float gTEMP, float gPH, float gTURBIDITY )
{
      String cmd = "AT+CIPSTART=\"TCP\",\"";                                                    
      cmd += "184.106.153.149";                                                                
      cmd += "\",80";
    
      esp.println(cmd);
      if (DEBUG)
      {
        //Serial.println(cmd);
      }
      if(esp.find("Error"))
      {
        if (DEBUG) 
        {
          //Serial.println("AT+CIPSTART error");
        }
        return false;
      }
      
      String getStr = "GET /update?api_key=";             // prepare GET string
      getStr += apikey;
      
      getStr +="&field1=";
      getStr += String(gTEMP);
      getStr +="&field2=";
      getStr += String(gPH);
      getStr +="&field3=";
      getStr += String(gTURBIDITY);
      getStr += "\r\n\r\n";
    
      cmd = "AT+CIPSEND=";
      cmd += String(getStr.length());
      esp.println(cmd);
      if (DEBUG)  //Serial.println(cmd);
      
      delay(100);
      if(esp.find(">"))
      {
        esp.print(getStr);
        if (DEBUG) 
        {
          //Serial.print(getStr);
        }
      }
      else
      {
        esp.println("AT+CIPCLOSE");
        if (DEBUG)   
        {
          //Serial.println("AT+CIPCLOSE");
        }
        return false;
      }
      return true;
}

/**************************************************************************************************************************/

void loop()
{
     
      //******temperature****************************************************
      
       if ( !ds.search(addr)) 
      {
          ds.reset_search();
          delay(250);
          return;
      }
    
      for( i = 0; i < 8; i++) 
      {
      }
    
      if (OneWire::crc8(addr, 7) != addr[7]) 
      {
          return;
      }
    
      switch (addr[0]) 
      {
          case 0x10:
            type_s = 1;
            break;
          case 0x28:
            type_s = 0;
            break;
          case 0x22:
            type_s = 0;
            break;
          default:
            return;
      } 
    
      ds.reset();
      ds.select(addr);
      ds.write(0x44);                                                                                                     // start conversion, use ds.write(0x44,1) with parasite power on at the end
    
      delay(1000);                                                                                                        // maybe 750ms is enough, maybe not
    
      present = ds.reset();
      ds.select(addr);    
      ds.write(0xBE);                                                                                                     // Read Scratchpad
      
      for ( i = 0; i < 9; i++) 
      {                                                                                                                   // we need 9 bytes
        data[i] = ds.read();
      }
      
      int16_t raw = (data[1] << 8) | data[0];
      if (type_s) 
      {
        raw = raw << 3;                                                                                                    // 9 bit resolution default
        if (data[7] == 0x10) 
        {                                                                                                                // "count remain" gives full 12 bit resolution
          raw = (raw & 0xFFF0) + 12 - data[6];
        }
      } 
      else 
      {
        byte cfg = (data[4] & 0x60);
                                                                                            // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7;                                                    // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3;                                               // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1;                                               // 11 bit res, 375 ms                                                                                    //// default is 12 bit resolution, 750 ms conversion time
      }
      celsius = (float)raw / 16.0;
      fahrenheit = celsius * 1.8 + 32.0;
      gTEMP = celsius ;
      
      Serial.print("Temperature :");
      Serial.println(celsius);
      Serial.println();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temperature :");
      lcd.setCursor(0, 1);
      lcd.print(celsius);
      delay(2000);
      
      if ( celsius > 32)
      {
        Serial.println("TEMPERATURE IS MORE ..........");
        Serial.println("");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("TEMPERATURE IS");
        lcd.setCursor(0, 1);
        lcd.print("MORE ......");
        delay(2000);
        
      }
      
      //****PH*****************************************************
      
      int value_ph = analogRead(A0);                                                                //read LDR value from A0-ADC0
      delay(10);
      value_ph = analogRead(A0);
      float millivolts_ph = (value_ph /1023.0)*5000;                                             //calibrate the analog data in terms of milli volts for 10-bit ADC resolution and 3.3v operating voltage
      PH = millivolts_ph /550;  
      delay(150);
      Serial.println(PH);
      
      if ( PH > 6.80 )
      {
          //PH = PH + 0.15 ;
          gPH = PH ;
          
          Serial.print("PH OF WATER :");
          Serial.println(PH);
          Serial.println("WATER is good");
          Serial.println();
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("PH OF WATER:");
          lcd.print(PH);
          lcd.setCursor(0, 1);
          lcd.print("WATER is good");
          delay(2000);
      }
      if ( PH <= 6.80 )
      {
          
          PH = PH + 2;
          gPH = PH ;
          
          Serial.print("PH OF WATER :");
          Serial.println(PH);
          Serial.println("WATER is not good");
          Serial.println();
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("PH OF WATER:");
          lcd.print(PH);
          lcd.setCursor(0, 1);
          lcd.print("WATER not good");
          delay(2000);   
      }    

      //******turbidity*******************************************************************
      
      voltage=0.004888*analogRead(turbidity_pin);                 //in V
      turbidity=-1120.4*voltage*voltage+5742.3*voltage-4352.9;    //in NTU
      gTURBIDITY = turbidity ;
      delay(100);
      
      Serial.println("Voltage="+String(voltage)+" V Turbidity="+String(turbidity)+" NTU");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Voltage=");
      lcd.print(voltage);
      lcd.setCursor(0, 1);
      lcd.print("Turbi = ");
      lcd.print(turbidity);
      delay(2000);

      if ( turbidity > 2000 )
      {
          Serial.println("WATER Quality is Good");
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("WATER Quality");
          lcd.setCursor(0, 1);
          lcd.print("is Good");
          delay(2000);
      }
      
      if ( turbidity < 2000 && turbidity > 1000 )
      {
          Serial.println("TURBIDITY IS NOT IN WATER");
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("TURBIDITY IS");
          lcd.setCursor(0, 1);
          lcd.print("NOT IN WATER");
          delay(2000);
      }
      if ( turbidity < 1000 )
      {
          Serial.println("WATER Quality is not good");
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("WATER Quality");
          lcd.setCursor(0, 1);
          lcd.print("is not Good");
          delay(2000);
      }
      
      //*****thingspeak***************************************************************
      
      thingSpeakWrite( gTEMP, gPH, gTURBIDITY );
      Serial.println("");
      Serial.println("DATA UPLOADED TO CLOUD");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("DATA UPLOADED");
      lcd.setCursor(0, 1);
      lcd.print("TO CLOUD");
      Serial.println("");
      Serial.println("******************************************************");
      delay(3000);
}

/**************************************************************************************************************************/


