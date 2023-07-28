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

bool bit1 = false;                                          // bit for running datalog() function
bool encoder_A;                                             // value of pin A (rotary encoder)
bool encoder_B;                                             // value of pin B (rotary encoder)
bool encoder_A_prev = 0;                                    // previous value of pin A
bool backlight = true;                                      // LCD backlight state (true - on, false - off)
int A = 3;                                                  // number of digital pin A
int B = 5;                                                  // number of digital pin B
int C = 2;                                                  // number of digital pin B
unsigned long period;                                       // period of measurement
unsigned long time = 0;                                     // time since the beginning of the measurement (in miliseconds)
unsigned long time1 = 0;                                    // time, when the button was pressed
unsigned long time2 = 0;                                    // time, when the button was realeased
double pressure;                                            // current atmospheric pressure
double pressure0;                                           // initial atmospheric pressure (at the sea level)
double altitude;                                            // current altitude
double altitude0;                                           // initial altitude
String filename = "data000.csv";                            // csv file name                                  

void digpin_init()                                          // initialize digital pins
{
  pinMode (A, INPUT_PULLUP);                                // set pin A as input
  pinMode (B, INPUT_PULLUP);                                // set pin B as input
  pinMode (C, INPUT_PULLUP);                                // set pin C as input
}

void lcd_init()                                             // inicialize LCD display
{
  lcd.init();                                               
  lcd_backlight(true);
}

void bmp280_init()                                          // initialize BMP280 sensor
{
  if (!bmp.begin(0x76))                                     // check if BMP280 sensor is connected
  {
    lcd.print("BMP280-missing");
    while (1);
  }
}

void measurement_init()                                     // initialize a measurement
{
  time = 0;
  pressure0 = 0;
  altitude0 = double(set_value("Init. altitude:", "m ASL", 0, 8000, 300, 100));
  period = (unsigned long)(set_value("Period:", "ms", 100, 8300, 1000, 1000)) * 1000;
  lcd_write("Calibration...", "", false);                
  for(int i=1; i<=1000; i++)
  {
    pressure0 += bmp.readPressure();
  }
  pressure0 = pressure0 / 1000;
}

void sd_card_init()                                         // initialize SD card
{
  if(SD.begin(4))                                           // check SD card
  {
    int meas_num = 0;                                       // measurement number
    while(SD.exists(filename))                              // create a new csv file with number of measurement
    {
      meas_num += 1;                                        // increase measurement number
      if (meas_num < 10) {filename = "data00" + String(meas_num) + ".csv";}
      else if(meas_num < 100) {filename = "data0" + String(meas_num) + ".csv";}
      else {filename = "data" + String(meas_num) + ".csv";}
    }
    myFile = SD.open(filename, FILE_WRITE);                 // open csv file for writing
    write_line_to_csv("ALTITUDE MEASUREMENT");              // write file header
    write_line_to_csv("Measurement: " + String(meas_num));
    write_line_to_csv("Initial altitude: "  + String(altitude0) + " m ASL");
    write_line_to_csv("Initial air pressure: " + String(pressure0/100) + " hPa");
    write_line_to_csv("");
    write_line_to_csv("Time [ms],Air pressure [Pa],Altitude [m ASL]");
  }
  else
  {
    lcd_write("SD card is missing", "", true);
  }
}

void lcd_backlight(bool state)                              // set display backlight
{
  if (state) {lcd.backlight();}
  else {lcd.noBacklight();}
}

void lcd_write(String row1, String row2, bool msg)          // write two rows on the display
{
  lcd.clear();                                              // clear the display
  lcd.setCursor(0, 0);                                      // set cursor to 0,0
  lcd.print(row1);                                          // write the first row
  lcd.setCursor(0, 1);                                      // set cursor to 0,1
  lcd.print(row2);                                          // write the second row
  if (msg) {delay(2000);}                                   // if the text is a message, wait 2 seconds to be able to read it
}

void write_line_to_csv(String line)
{
  if(myFile) {myFile.println(line);}
}

int set_value(String name, String unit, int min, int max, int defaultvalue, int step)
{
  int value = defaultvalue;
  lcd_write(name, String(value) + " " + unit, false);
  while (step >= 1)
  {
    while(digitalRead(C))
    {
      encoder_A = digitalRead(A);                           // read digital pin A
      encoder_B = digitalRead(B);                           // read digital pin B
      if (!encoder_A && encoder_A_prev)
      {                                            
        if (encoder_B)
        {
          value = value + step;
          if (value > max) {value = max;}
        }
        else
        {
          value = value - step;
          if (value < min) {value = min;}
        }
        lcd_write(name, String(value) + " " + unit, false);
      }
      encoder_A_prev = encoder_A;
    }
    while (!digitalRead(C)) {}
    step = step/10;                                         // divide settings step by 10
  }
  return value;
}

void decide()                                               // decide what to do nest (continue, new measurement, end measurement)
{
  int a = 0;
  lcd_backlight(true);
  lcd_write_desition(a);
  while (digitalRead(C))
  {
    encoder_A = digitalRead(A);                             // read digital pin A
    encoder_B = digitalRead(B);                             // read digital pin B
    if (!encoder_A && encoder_A_prev)
    {                                            
      if (encoder_B)
      {
        a += 1;
        if (a > 2) {a = 0;}
      }
      else
      {
        a -= 1;
        if (a < 0) {a = 2;}
      }
      lcd_write_desition(a);
    }
    encoder_A_prev = encoder_A;
  }
  while (!digitalRead(C)) {}
  if (a == 0) {}
  if (a == 1) {measurement_init();}
  if (a == 2) {finish(); sd_card_init();}
}

void lcd_write_desition(int state)                          // write current desition to display
{
  if (state == 0) {lcd_write("Action:", "continue", false);}
  if (state == 1) {lcd_write("Action:", "new measurement", false);}
  if (state == 2) {lcd_write("Action:", "end measurement", false);}
}

void timer()                                                // run datalog() function
{
  bit1 = true;
}

void datalog()
{
  pressure = bmp.readPressure();
  altitude = (pressure0 * log(pressure0/pressure)) / (12.01725) + altitude0;  // calculate altitude
  lcd_write("Altitude:", String(altitude) + " m ASL", false); // write altitude to LCD display
  write_line_to_csv(String(time) + "," + String(pressure) + "," + String(altitude)); // write data to csv file
  time += period;
  bit1 = false;
}

void finish()
{
  myFile.close();                                           // close csv file
  lcd_write("Meauserement", "ended", true);
  lcd_backlight(false);
  lcd.noDisplay();                                          // switch off the display
  while(1);
}

void setup()
{
  digpin_init();                                            // initialize digital pins
  lcd_init();                                               // initialize LCD display
  bmp280_init();                                            // initialize BMP280 sensor
  measurement_init();                                       // initialize measurement
  sd_card_init();                                           // initialize SD card
  lcd_write("Measurement", "started", true);
  Timer1.initialize(period);                                // set a period for TIMER1 (in microseconds)
  Timer1.attachInterrupt(timer);                            // run timer() function after one period
}

void loop()
{
  while (digitalRead(C))                                    // while the button is not pushed, measure
  {
    if (bit1) {datalog();}
  }
  time1 = millis();                                         // time, when the button was pressed
  while (!digitalRead(C))
  time2 = millis();                                         // time, when the button was released
  if (time2 - time1 > 3000) {decide();}                     // if the button has been pressed for more than 3 seconds, decide what to do
  else {backlight = !backlight; lcd_backlight(backlight);}  // if the button has been pressed for less than 3 seconds, change LCD display backlight
}
