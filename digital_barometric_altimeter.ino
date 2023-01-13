#include <Adafruit_BMP280.h>                               // knihovna pro BMP280
#include <LiquidCrystal_I2C.h>                             // knihovna pro LCD displej verze I2C
#include <SD.h>                                            // knihovna pro kominukaci s SD kartou
#include <SPI.h>                                           // knihovna pro komunikaci SPI
#include <TimerOne.h>                                      // knihovna pro čítač TIMER1

LiquidCrystal_I2C lcd(0x27, 20, 4);

Adafruit_BMP280 bmp;                                       // pro komunikaci displeje pomocí I2C

File myFile;

volatile bool bit1 = false;                                // bit, kterým se spouští funkce datalog()
volatile bool bit2 = false;                                // bit, kterým se spouští funkce finish()
volatile bool bit3 = true;                                 // 
volatile bool encoder_A;                                   // hodnota pinu pinA
volatile bool encoder_B;                                   // hodnota pinu pinB
volatile bool encoder_A_prev = 0;                          // předchozí hodnota pinu A
unsigned long period;                                      // perioda měření
unsigned long cas = 0;                                     // čas od začátku měření
double atm0;                                               // počáteční atmosférický tlak (na hladině moře)
double altitude;                                           // aktuální nadmořská výška
double altitude0;                                          // počáteční nadmořská výška                                    

int value(String velicina, String jednotka, int minh, int maxh, int prom, int krok)
{
  int value = prom;
  while (krok >= 1)
  {
    while((digitalRead(2)) == HIGH)
    {
      encoder_A = digitalRead(3);                          // přečte hodnotu pinu 3
      encoder_B = digitalRead(5);                          // přečte hodnotu pinu 5
      if((!encoder_A) && (encoder_A_prev))                 // zjišťuje, zda se změnil stav na pinu 3
      {                                            
        bit3 = true;
        if(encoder_B)                                      // B je 1, takže value roste
        {
          value = value + krok;
          if(value > maxh)
          {
            value = maxh;       
          }
        }
        else                                               // B je 0, takže value klesá
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
        lcd.clear();                                       // vymazání disleje
        lcd.setCursor(0, 0);                               // kurzor na 0,0
        lcd.print(velicina);                               // napíše hodnotu čísla velicina
        lcd.setCursor(0, 1);                               // kurzor na 0,1
        lcd.print(String(value) + " " + jednotka);         // na displej napíše hodnotu veličiny a její jednotku
        bit3 = false;
      }
      encoder_A_prev = encoder_A;                          // uloží hodnotu A do encoder_A_prev
    }
    while((digitalRead(2)) == LOW)
    {
      
    }
    krok = krok/10;
  }
  bit3 = true;
  return value;
}

void timer()
{
  bit1 = true;
}

void ending()
{
  bit2 = true;
}

void datalog()
{
  altitude = (atm0 * log( atm0 / bmp.readPressure() )) / ( 12.01725 ) + altitude0;  // do proměnné altitude zapíše aktuální nadmořskou výšku
  lcd.clear();                                                                      // vymazání disleje
  lcd.setCursor(0, 0);                                                              // kurzor na 0,0
  lcd.print("Nadmorska vyska:");                                                    // napíše do prvního řádku Nadmorska vyska:
  lcd.setCursor(0, 1);                                                              // kurzor na 0,1
  lcd.print(String(altitude) + " m n.m."); 
  if (myFile) 
  {
    myFile.println(String(cas) + "," + String(altitude));                            // na nový řádek zapíše čas a nadmořskou výšku
  }
  cas = cas + period;
  bit1 = false;
}

void finish()
{
  myFile.close();                                          // zavře soubor mereni.txt
  lcd.clear();                                             // vymaže LCD displej
  lcd.print("Mereni ukonceno");                            // na LCD displej napíše Mereni ukonceno
  delay(2000);                                             // počká 2 sekundy
  lcd.noBacklight();                                       // vypne podsvícení
  lcd.noDisplay();                                         // vypne LCD displej
  while(1);
}

void setup()
{
  pinMode (2, INPUT_PULLUP);                               // nastavení pinu pinA na vstup
  pinMode (3, INPUT_PULLUP);                               // nastavení pinu pinB na vstup
  pinMode (5, INPUT_PULLUP);                               // nastavení pinu pinC na vstup
  lcd.init();                                              // inicializace LCD displeje
  lcd.backlight();                                         // podsvícení displeje
  if (!bmp.begin(0x76))                                    // zjišťuje, zda je připojen senzor BMP280
  {
    lcd.print("Nepripojen BMP280");
    while (1);
  }
  if (!SD.begin(4))                                        // zjišťuje, zda je vložena SD karta
  {
    lcd.print("Chybi SD karta");
    delay(2000);
  }
  myFile = SD.open("data.csv", FILE_WRITE);                // otevře soubor data.csv k zápisu
  altitude0 = double(value("Nadmorska vyska:", "m n.m.", 0, 8000, 300, 100));
  period = (unsigned long)(value("Perioda:", "ms", 100, 30000, 1000, 1000));
  lcd.clear();                                             // vymaže displej
  lcd.print("Kalibrace...");                               // napíše na displej Kalibrace
  for(int i=1; i<=1000; i++)
  {
    atm0 = atm0 + bmp.readPressure();
  }
  atm0 = atm0 / 1000;
  if (myFile) 
  {
    myFile.println("MERENI NADMORSKE VYSKY POMOCI ATMOSFERICKEHO TLAKU");
    myFile.println("Atmosfericky tlak v nadmorske vysce"  + String(altitude0) + " m n.m. je: " + String(atm0/100) + " hPa");
    myFile.println("Cas [ms],Nadmorska vyska [m n.m.]");
  }
  lcd.clear();                                             // vymaže displej
  lcd.print("Mereni zahajeno");                            // napíše na displej Mereni zahajeno
  delay(2000);                                             // počká 2 sekundy
  Timer1.initialize(period * 1000);                        // nastavení maxima, do kterého Timer1 počítá (v mikrosekundách)
  Timer1.attachInterrupt(timer);                           // při interruptu spustí funkci timer
  attachInterrupt(digitalPinToInterrupt(2), ending, LOW);  // nastavení externího interruptu na pin D2, při interruptu spustí funkci ending
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
