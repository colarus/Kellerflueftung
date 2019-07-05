
// taken from https://carnotcycle.wordpress.com/2012/08/04/how-to-convert-relative-humidity-to-absolute-humidity/
// precision is about 0.1°C in range -30 to 35°C
// T=Lufttemperatur
// rh=relative Feuchte
// Abs. Feuchte (g/m³)=(6.112 * (2.71828 ** ((17.67 * T)/(T + 243.5)) * rh * 2.1674)/(273.15 + T)



#include "DHT.h"
#include <math.h>
#include <SPI.h> // needed in Arduino 0019 or later
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// für cpp nötig: Function Declaration 
float h2o(double relativefeuchte, double temperatur);
void luefterAN();
void luefterAUS();

// Set the LCD address to 0x27 or 0x3F for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x3F, 16, 2);
//LiquidCrystal_I2C lcd (0x3F, 20, 4); // for a 20 chars and 4 line display
// SDA... Verbunden mit Analog Pin 4
// SCL... Verbunden mit Analog Pin 5

#define DHTPIN1 7   // Sensor 1 indoor Sensor
#define DHTPIN2 8   // Sensor 2 outdoor Sensor

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dhtIndoor(DHTPIN1, DHTTYPE); // indoor Sensor
DHT dhtOutdoor(DHTPIN2, DHTTYPE); // outdoor Sensor

const int relaisPin = 6 ; // Relais

void setup()
{
  Serial.begin(9600);
  pinMode(relaisPin, OUTPUT);

  dhtIndoor.begin();
  dhtOutdoor.begin();

  lcd.init(); //Im Setup wird der LCD gestartet
  lcd.backlight(); //Hintergrundbeleuchtung einschalten (lcd.noBacklight(); schaltet die Beleuchtung aus).
}

void loop()
{
  int iMinuten = 0; //Minutenzähler

  while (iMinuten < 60) //Lüftung läuft maximal 60 Minuten
  {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    double hIndoor = dhtIndoor.readHumidity();
    double tIndoor = dhtIndoor.readTemperature(); // indoor
    double hOutdoor = dhtOutdoor.readHumidity();
    double tOutdoor = dhtOutdoor.readTemperature(); // outdoor
    float h2oIndoor = 0;
    float h2oOutdoor = 0;

    // check if returns are valid, if they are NaN (not a number) then something went wrong!
    if (isnan(tIndoor) || isnan(hIndoor))
    {
      Serial.println("Failed to read from DHT");
      //   lcd.begin();
      lcd.setCursor(0, 0);
      lcd.print("Keine Daten!");
    }
    else
    {
      h2oIndoor = h2o(hIndoor, tIndoor);

      // Serialmonitor Ausgabe
      Serial.print("Humidity1: ");
      Serial.print(hIndoor);
      Serial.print(" %\t");
      Serial.print("Temperature1: ");
      Serial.print(tIndoor);
      Serial.println(" *C");
      Serial.print("HumidityA1: ");
      Serial.print(h2oIndoor);
      Serial.println(" g");

      //LCD Ausgabe
      //      lcd.begin();
      lcd.setCursor(0, 0);
      lcd.print("I:");
      lcd.print((int)hIndoor);
      lcd.print("% ");
      lcd.print((int)(tIndoor + 0.5));
      lcd.print("\337C ");
      lcd.print(h2oIndoor);
      lcd.println("g");
      //int b=(int)(a+.5);
    }

    if (isnan(tOutdoor) || isnan(hOutdoor))
    {
      Serial.println("Failed to read from DHT");
      lcd.setCursor(0, 1);
      lcd.print("Keine Daten!");
    }
    else
    {
      h2oOutdoor = h2o(hOutdoor, tOutdoor);

      //Serialmonitor Ausgabe
      Serial.print("Humidity2: ");
      Serial.print(hOutdoor);
      Serial.print(" %\t");
      Serial.print("Temperature2: ");
      Serial.print(tOutdoor);
      Serial.println(" *C");
      Serial.print("HumidityA2: ");
      Serial.print(h2oOutdoor);
      Serial.println(" g");

      //LCD Ausgabe
      lcd.setCursor(0, 1);
      lcd.print("A:");
      lcd.print((int)hOutdoor);
      lcd.print("% ");
      lcd.print((int)(tOutdoor + .5));
      lcd.print("\337C ");
      lcd.print(h2oOutdoor);
      lcd.println("g");
    }

    if (isnan(tOutdoor) || isnan(hOutdoor) || isnan(tIndoor) || isnan(hIndoor))
    {
      // Lüftung ausschalten bei fehlerhaften Werten
      luefterAUS(); // Strom aus!
      delay(60000); // 1 min pause vor erneueter Prüfung
      iMinuten = 61;
    }
    else
    {
      if ((h2oIndoor - h2oOutdoor) > 3.5 ) // lüften wenn Aussenluft mind. 3,5g weniger Wasser/m³ enthält als Innenluft
      {
        // Innenentemperatur möglichst zwischen +5 und +20 Grad halten,
        // aber lüften wenn Aussenluft über 10g weniger Wasser/m³enthält als Innenluft
        if ((tIndoor < 5 && tOutdoor >= tIndoor )|| (tIndoor  > 20 && tOutdoor <= tIndoor  ) ||((h2oIndoor-h2oOutdoor)>10))     
        {
          Serial.print("Aussenluft ");
          Serial.print(h2oIndoor - h2oOutdoor);
          Serial.println("g trockener!");
          Serial.println("Lüftung an seit ");
          Serial.print(iMinuten);
          Serial.print(" Minuten");

          luefterAN(); // Strom an! Lüftung läuft!
          delay(60000); // 1 min lüften
          iMinuten = iMinuten + 1; //Minutenzähler +1 Minute
        }
        else
        {
          // Lüftung aus wenn Temperatur nicht stimmt
          luefterAUS();
          delay(600000); //10 min Pause
        }
      }
      else
      {
        //Lüftung aus wenn Luft nicht trocken genug
        luefterAUS();
        delay(600000); //10 min Pause
      }
    }
  } // Ende While

  if (iMinuten == 60) // Falls 60 Minuten gelüftet
  {
    // Lüftung ausschalten
    luefterAUS(); // Strom aus!
    delay(600000); // 10 min pause
  } else
  {
    // Ausstieg bei fehlerhaften Werten 10 sek. warten vor erneut Prüfung
    delay(10000);
  }
}// End loop


void luefterAN()
// Schaltet das Relais AN
{
  digitalWrite(relaisPin, LOW);
}

void luefterAUS()
// Schaltet das Relais AUS
{
  Serial.println("Lüftung aus!");
  digitalWrite(relaisPin, HIGH);
}

float h2o(double relativefeuchte, double temperatur)
{
  float h2o = 0;
  
// neue Formel als eine Zeile
  h2o = (6.112 * relativefeuchte * 2.1674 * pow (2.71828, ((17.67 * temperatur)/(temperatur + 243.5))))/(273.15 + temperatur);
  return h2o;
}