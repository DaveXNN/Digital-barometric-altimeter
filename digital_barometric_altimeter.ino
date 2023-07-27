/*****************************************************************************************************************************************/
/*                                                                                                                                       */
/*                                                      DIGITAL BAROMETRIC ALTIMETER                                                     */ 
/*                                                                                                                                       */
/*                                                             David Nenicka                                                             */
/*                                                                                                                                       */
/*****************************************************************************************************************************************/

#include <Adafruit_BMP280.h>                                // library for BMP280 sensor
#include <LiquidCrystal_I2C.h>                              // library for LCD display (I2C version)
#include <SD.h>                                             // library for communication with SD card module
#include <SPI.h>                                            // library for SPI communication
#include <TimerOne.h>                                       // library for timer TIMER1

LiquidCrystal_I2C lcd(0x27, 20, 4);                         // for LCD display
Adafruit_BMP280 bmp;                                        // for BMP280 sensor
File myFile;                                                // csv file for writing data

volatile bool bit1 = false;                                 // bit for running datalog() function
volatile bool bit2 = false;                                 // bit for running finish() function
volatile bool bit3 = true;                                  // bit for writing value on LCD display
volatile bool encoder_A;                                    // value of pin A (rotary encoder)
volatile bool encoder_B;                                    // value of pin B (rotary encoder)
volatile bool encoder_A_prev = 0;                           // previous value of pin A
int A = 3;                                                  // number of digital pin A
int B = 5;                                                  // number of digital pin B
int C = 2;                                                  // number of digital pin B
unsigned long period;                                       // period of measurement
unsigned long time = 0;                                     // time since the beginning of the measurement (in miliseconds)
double pressure;                                            // current atmospheric pressure
double pressure0;                                           // initial atmospheric pressure (at the sea level)
double altitude;                                            // current altitude
double altitude0;                                           // initial altitude
String filename = "data000.csv";                            // csv file name                                  

void digpin_init()                                          // initialization of digital pins
{
  pinMode (A, INPUT_PULLUP);                                // set pin A as input
  pinMode (B, INPUT_PULLUP);                                // set pin B as input
  pinMode (C, INPUT_PULLUP);                                // set pin C as input
}

void lcd_init()                                             // inicialization of the display
{
  lcd.init();                                               
  lcd.backlight();                                          // switch on backlight of the display
}

void bmp280_init()                                          // initialization of BMP280 sensor
{
  if (!bmp.begin(0x76))                                     // check if BMP280 sensor is connected
  {
    lcd.print("BMP280-missing");
    while (1);
  }
}

void sd_card_init()                                         // initialization of SD card
{
  if(SD.begin(4))                                           // check SD card
  {
    int meas_num = 0;                                       // measurement number
    while(SD.exists(filename))                              // create a new csv file with number of measurement
    {
      meas_num += 1;                                        // increase measurement number
      if(meas_num < 10)
      {
        filename = "data00" + String(meas_num) + ".csv";
      }
      else if(meas_num < 100)
      {
        filename = "data0" + String(meas_num) + ".csv";
      }
      else
      {
        filename = "data" + String(meas_num) + ".csv";
      }
    }
    myFile = SD.open(filename, FILE_WRITE);                 // open csv file for writing
    write_line_to_csv("ALTITUDE MEASUREMENT");
    write_line_to_csv("Measurement: " + String(meas_num));
    write_line_to_csv("Initial altitude: "  + String(altitude0) + " m ASL");
    write_line_to_csv("Initial air pressure: " + String(pressure0/100) + " hPa");
    write_line_to_csv("");
    write_line_to_csv("Time [ms],Air pressure [Pa],Altitude [m ASL]");
  }
  else
  {
    lcd_write("SD card is missing", "");
    delay(2000);
  }
}

void lcd_write(String row1, String row2)                    // write two rows on LCD display
{
  lcd.clear();                                              // clear the display
  lcd.setCursor(0, 0);                                      // set cursor to 0,0
  lcd.print(row1);                                          // write the first row
  lcd.setCursor(0, 1);                                      // set cursor to 0,1
  lcd.print(row2);                                          // write the second row
}

void write_line_to_csv(String line)
{
  if(myFile)
  {
    myFile.println(line);
  }
}

int set_value(String name, String unit, int min, int max, int defaultvalue, int step)
{
  int value = defaultvalue;
  while (step >= 1)
  {
    while((digitalRead(C)) == HIGH)
    {
      encoder_A = digitalRead(A);                           // read digital pin A
      encoder_B = digitalRead(B);                           // read digital pin B
      if((!encoder_A) && (encoder_A_prev))
      {                                            
        bit3 = true;
        if(encoder_B)
        {
          value = value + step;
          if(value > max)
          {
            value = max;       
          }
        }
        else
        {
          value = value - step;
          if(value < min)
          {
            value = min;   
          }
        }
      }
      if(bit3)
      {
        lcd_write(name, String(value) + " " + unit);    // write 
        bit3 = false;
      }
      encoder_A_prev = encoder_A;
    }
    while((digitalRead(C)) == LOW)
    {
      
    }
    step = step/10;                                         // divide settings step by 10
  }
  bit3 = true;
  return value;
}

void timer()                                                // run datalog() function
{
  bit1 = true;
}

void ending()                                               // run finish() function
{
  bit2 = true;
}

void datalog()
{
  pressure = bmp.readPressure();
  altitude = (pressure0 * log(pressure0/pressure)) / (12.01725) + altitude0;  // calculate altitude
  lcd_write("Altitude:", String(altitude) + " m ASL");      // write data to LCD display
  write_line_to_csv(String(time) + "," + String(pressure) + "," + String(altitude)); // write data to csv file
  time += period;
  bit1 = false;
}

void finish()
{
  myFile.close();                                           // close csv file
  lcd_write("Meauserement", "ended");                       // write Measurement ended on LCD display
  delay(2000);                                              // wait 2 seconds
  lcd.noBacklight();                                        // switch off the backlight
  lcd.noDisplay();                                          // switch off the display
  bit2 = false;
}

void setup()
{
  digpin_init();                                            // initialization of digital pins
  lcd_init();                                               // initialization of lcd display
  bmp280_init();                                            // initialization of BMP280 sensor
  altitude0 = double(set_value("Init. altitude:", "m ASL", 0, 8000, 300, 100));
  period = (unsigned long)(set_value("Period:", "ms", 100, 30000, 1000, 1000));
  lcd_write("Calibration...", "");                
  for(int i=1; i<=1000; i++)
  {
    pressure0 += bmp.readPressure();
  }
  pressure0 = pressure0 / 1000;
  sd_card_init();                                           // initialization of SD card
  lcd_write("Measurement", "started");                        
  delay(2000);                                           
  Timer1.initialize(period * 1000);                         // set a period for TIMER1
  Timer1.attachInterrupt(timer);                            // run timer() function after one period
  attachInterrupt(digitalPinToInterrupt(C), ending, LOW);   // external interrupt on D2 pin, runs ending() function
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
