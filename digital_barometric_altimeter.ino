/*****************************************************************************************************************************************/
/*                                                                                                                                       */
/*                                                      DIGITAL BAROMETRIC ALTIMETER                                                     */ 
/*                                                                                                                                       */
/*                                                             David Nenicka                                                             */
/*                                                                                                                                       */
/*****************************************************************************************************************************************/

#include <Adafruit_BMP280.h>                               // library for BMP280 sensor
#include <LiquidCrystal_I2C.h>                             // library for LCD display (I2C version)
#include <SD.h>                                            // library for communication with SD card module
#include <SPI.h>                                           // library for SPI communication
#include <TimerOne.h>                                      // library for timer TIMER1

LiquidCrystal_I2C lcd(0x27, 20, 4);                        // for LCD display

Adafruit_BMP280 bmp;                                       // for BMP280 sensor

File myFile;

volatile bool bit1 = false;                                // bit for running datalog() function
volatile bool bit2 = false;                                // bit for running finish() function
volatile bool bit3 = true;                                 // bit for writing value on LCD display
volatile bool encoder_A;                                   // value of pin A (rotary encoder)
volatile bool encoder_B;                                   // value of pin B (rotary encoder)
volatile bool encoder_A_prev = 0;                          // previous value of pin A
unsigned long period;                                      // period of measurement
unsigned long cas = 0;                                     // time since the beginning of the measurement
double atm0;                                               // initial atmospheric pressure (at the sea level)
double altitude;                                           // current altitude
double altitude0;                                          // initial altitude                                  

int value(String velicina, String jednotka, int minh, int maxh, int prom, int krok)
{
  int value = prom;
  while (krok >= 1)
  {
    while((digitalRead(2)) == HIGH)
    {
      encoder_A = digitalRead(3);                          // read digital pin A
      encoder_B = digitalRead(5);                          // read digital pin B
      if((!encoder_A) && (encoder_A_prev))
      {                                            
        bit3 = true;
        if(encoder_B)
        {
          value = value + krok;
          if(value > maxh)
          {
            value = maxh;       
          }
        }
        else
        {
          value = value - krok;
          if(value < minh)
          {
            value = minh;   
          }
        }
      }

      if(bit3)
      {
        lcd.clear();                                       // clear the display
        lcd.setCursor(0, 0);                               // cursor to 0,0
        lcd.print(velicina);                               // write the name of the value
        lcd.setCursor(0, 1);                               // cursor to 0,1
        lcd.print(String(value) + " " + jednotka);         // write a value and unit
        bit3 = false;
      }
      encoder_A_prev = encoder_A;
    }
    while((digitalRead(2)) == LOW)
    {
      
    }
    krok = krok/10;
  }
  bit3 = true;
  return value;
}

void timer()                              // run datalog() function
{
  bit1 = true;
}

void ending()                             // run finish() function
{
  bit2 = true;
}

void datalog()
{
  altitude = (atm0 * log( atm0 / bmp.readPressure() )) / ( 12.01725 ) + altitude0;  // calculates altitude
  lcd.clear();                                                         
  lcd.setCursor(0, 0);                                                       
  lcd.print("Altitude:");                                                      
  lcd.setCursor(0, 1);                                                           
  lcd.print(String(altitude) + " m ASL"); 
  if (myFile) 
  {
    myFile.println(String(cas) + "," + String(altitude));
  }
  cas = cas + period;
  bit1 = false;
}

void finish()
{
  myFile.close();                                          // close data.csv
  lcd.clear();                                             // clear the display
  lcd.print("Meauserement ended");                         // write Meauserement ended
  delay(2000);                                             // wait 2 seconds
  lcd.noBacklight();                                       // switch off the backlight
  lcd.noDisplay();                                         // switch off the display
}

void setup()
{
  pinMode (2, INPUT_PULLUP);                               // set pin A as input
  pinMode (3, INPUT_PULLUP);                               // set pin B as input
  pinMode (5, INPUT_PULLUP);                               // set pin C as input
  lcd.init();                                              // inicialization of the display
  lcd.backlight();                                         // backlight of the display
  if (!bmp.begin(0x76))                                    // check BMP280 sensor
  {
    lcd.print("BMP280 is not connected");
    while (1);
  }
  if (!SD.begin(4))                                        // check SD card
  {
    lcd.print("SD card is missing");
    delay(2000);
  }
  myFile = SD.open("data.csv", FILE_WRITE);                // open data.csv for writing
  altitude0 = double(value("Altitude:", "m ASL", 0, 8000, 300, 100));
  period = (unsigned long)(value("Period:", "ms", 100, 30000, 1000, 1000));
  lcd.clear();                  
  lcd.print("Calibration...");                  
  for(int i=1; i<=1000; i++)
  {
    atm0 = atm0 + bmp.readPressure();
  }
  atm0 = atm0 / 1000;
  if (myFile) 
  {
    myFile.println("MEASUREMENT OF ALTITUDE USING ATMOSPHERIC PRESSURE");
    myFile.println("Atmospheric pressure in "  + String(altitude0) + " m ASL is : " + String(atm0/100) + " hPa");
    myFile.println("Time [ms], Altitude [m n.m.]");
  }
  lcd.clear();                                            
  lcd.print("Measurement started");                          
  delay(2000);                                           
  Timer1.initialize(period * 1000);                        // set a period for TIMER1
  Timer1.attachInterrupt(timer);                           // run timer() function after one period
  attachInterrupt(digitalPinToInterrupt(2), ending, LOW);  // external interrupt on D2 pin, runs ending() function
}

void loop()
{
  if(bit1)
  {
    datalog();
  }
  if(bit2)
  {
    finish();
  }
}
