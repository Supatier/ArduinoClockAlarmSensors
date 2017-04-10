/**
  ---------------------------------------------------------
  Originaly by:
     Nick Lamprianidis { paign10.ln [at] gmail [dot] com }
  ---------------------------------------------------------

*/

#include <Wire.h>  // Required by RTClib
#include <LiquidCrystal.h>  // Required by LCDKeypad
#include <LCDKeypad.h>
#include "RTClib.h"
#include "DHT.h"

#define TIME_OUT 5
#define ALARM_TIME_MET 6
#define BUZZER_PIN 3
#define SNOOZE 10

#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

enum states
{
  SHOW_TIME,
  SHOW_TIME_ALARM_ON,
  SHOW_ALARM_TIME,
  SET_ALARM_HOUR,
  SET_ALARM_MINUTES,
  BUZZER_ON
};
int DD, MM, YY, H, M, S, set_state, up_state, down_state;
int btnCount = 0;
unsigned long previousMillis = 0;
unsigned long currentMillis;
String sDD;
String sMM;
String sYY;
String sH;
String sM;
String sS;


byte term[8] = //icon for termometer
{
  B00100,
  B01010,
  B01010,
  B01110,
  B01110,
  B11111,
  B11111,
  B01110
};

byte pic[8] = //icon for water droplet
{
  B00100,
  B00100,
  B01010,
  B01010,
  B10001,
  B10001,
  B10001,
  B01110,
};

DHT dht(DHTPIN, DHTTYPE);
LCDKeypad lcd;

RTC_DS1307 RTC;

states state;  // Holds the current state of the system
int8_t button;  // Holds the current button pressed
uint8_t alarmHours = 0, alarmMinutes = 0;  // Holds the current alarm time
uint8_t tmpHours;
boolean alarm = false;  // Holds the current state of the alarm
unsigned long timeRef;
DateTime now;  // Holds the current date and time information


void setup()
{
  pinMode(BUZZER_PIN, OUTPUT);  // Buzzer pin

  // Initializes the LCD and RTC instances
  lcd.begin(16, 2);
  Wire.begin();
  RTC.begin();

  state = SHOW_TIME;  // Initial state of the FSM
  RTC.adjust(DateTime(__DATE__, __TIME__));
}

void loop()
{
  timeRef = millis();

  switch (state)
  {
    case SHOW_TIME:
      showTime();
      break;
    case SHOW_TIME_ALARM_ON:
      showTime();
      checkAlarmTime();
      break;
    case SHOW_ALARM_TIME:
      showAlarmTime();
      break;
    case SET_ALARM_HOUR:
      setAlarmHours();
      if ( state != SET_ALARM_MINUTES ) break;
    case SET_ALARM_MINUTES:
      setAlarmMinutes();
      break;
    case BUZZER_ON:
      showTime();
      break;
  }

  while ( (unsigned long)(millis() - timeRef) < 970 )
  {
    if ( (button = lcd.button()) != KEYPAD_NONE )
    {
      while ( lcd.button() != KEYPAD_NONE ) ;
      transition(button);
      break;
    }
  }
}

void transition(uint8_t trigger)
{
  switch (state)
  {
    case SHOW_TIME:
      if ( trigger == KEYPAD_LEFT ) state = SHOW_ALARM_TIME;
      else if ( trigger == KEYPAD_RIGHT ) {
        alarm = true;
        state = SHOW_TIME_ALARM_ON;
      }
      else if ( trigger == KEYPAD_SELECT ) state = SET_ALARM_HOUR;
      break;
    case SHOW_TIME_ALARM_ON:
      if ( trigger == KEYPAD_LEFT ) state = SHOW_ALARM_TIME;
      else if ( trigger == KEYPAD_RIGHT ) {
        alarm = false;
        state = SHOW_TIME;
      }
      else if ( trigger == KEYPAD_SELECT ) state = SET_ALARM_HOUR;
      else if ( trigger == ALARM_TIME_MET ) {
        analogWrite(BUZZER_PIN, 220);
        state = BUZZER_ON;
      }
      break;
    case SHOW_ALARM_TIME:
      if ( trigger == TIME_OUT ) {
        if ( !alarm ) state = SHOW_TIME;
        else state = SHOW_TIME_ALARM_ON;
      }
      break;
    case SET_ALARM_HOUR:
      if ( trigger == KEYPAD_SELECT ) state = SET_ALARM_MINUTES;
      else if ( trigger == TIME_OUT ) {
        if ( !alarm ) state = SHOW_TIME;
        else state = SHOW_TIME_ALARM_ON;
      }
      break;
    case SET_ALARM_MINUTES:
      if ( trigger == KEYPAD_SELECT ) {
        alarm = true;
        state = SHOW_TIME_ALARM_ON;
      }
      else if ( trigger == TIME_OUT ) {
        if ( !alarm ) state = SHOW_TIME;
        else state = SHOW_TIME_ALARM_ON;
      }
      break;
    case BUZZER_ON:
      if ( trigger == KEYPAD_UP || trigger == KEYPAD_DOWN ) {
        analogWrite(BUZZER_PIN, 0);
        snooze(); state = SHOW_TIME_ALARM_ON;
      }
      if ( trigger == KEYPAD_SELECT || trigger == KEYPAD_LEFT ) {
        analogWrite(BUZZER_PIN, 0);
        alarm = false; state = SHOW_TIME;
      }
      break;
  }
}

void showTime()
{
  now = RTC.now();
  lcdPrint();
  DateTime now = RTC.now();
  DD = now.day();
  MM = now.month();
  YY = now.year();
  H = now.hour();
  M = now.minute();
  S = now.second();

  if (DD < 10) {
    sDD = '0' + String(DD);
  } else {
    sDD = DD;
  }
  if (MM < 10) {
    sMM = '0' + String(MM);
  } else {
    sMM = MM;
  }
  sYY = YY;
  if (H < 10) {
    sH = '0' + String(H);
  } else {
    sH = H;
  }
  if (M < 10) {
    sM = '0' + String(M);
  } else {
    sM = M;
  }
  if (S < 10) {
    sS = '0' + String(S);
  } else {
    sS = S;
  }
}

void lcdPrint() {
  getTime();
  get_sens();
}
void getTime() {
  lcd.setCursor(0, 0);
  lcd.print(sH);
  lcd.print(":");
  lcd.print(sM);
  lcd.print(" ");
  lcd.print(sDD);
  lcd.print("/");
  lcd.print(sMM);
  lcd.print("/");
  lcd.print(sYY);
}

void get_sens()
{
  delay(2000);

  int h = dht.readHumidity();
  int t = dht.readTemperature();

  lcd.setCursor(0, 1);
  lcd.write(1);
  lcd.print(":");
  lcd.print(t);
  lcd.print((char)223); //degree sign
  lcd.print("C ");
  lcd.write(2);
  lcd.print(":");
  lcd.print(h);
  lcd.print("%");
}

void showAlarmTime()
{
  lcd.clear();
  lcd.print("Alarm Time");
  lcd.setCursor(0, 1);
  lcd.print(String("HOUR: ") + ( alarmHours < 9 ? "0" : "" ) + alarmHours +
            " MIN: " + ( alarmMinutes < 9 ? "0" : "" ) + alarmMinutes);
  delay(2000);
  transition(TIME_OUT);
}

void checkAlarmTime()
{
  if ( now.hour() == alarmHours && now.minute() == alarmMinutes ) transition(ALARM_TIME_MET);
}

void snooze()
{
  alarmMinutes += SNOOZE;
  if ( alarmMinutes > 59 )
  {
    alarmHours += alarmMinutes / 60;
    alarmMinutes = alarmMinutes % 60;
  }
}

void setAlarmHours()
{
  unsigned long timeRef;
  boolean timeOut = true;

  lcd.clear();
  lcd.print("Alarm Time");

  tmpHours = 0;
  timeRef = millis();
  lcd.setCursor(0, 1);
  lcd.print("Set hours:  0");

  while ( (unsigned long)(millis() - timeRef) < 5000 )
  {
    uint8_t button = lcd.button();

    if ( button == KEYPAD_UP )
    {
      tmpHours = tmpHours < 23 ? tmpHours + 1 : tmpHours;
      lcd.setCursor(11, 1);
      lcd.print("  ");
      lcd.setCursor(11, 1);
      if ( tmpHours < 10 ) lcd.print(" ");
      lcd.print(tmpHours);
      timeRef = millis();
    }
    else if ( button == KEYPAD_DOWN )
    {
      tmpHours = tmpHours > 0 ? tmpHours - 1 : tmpHours;
      lcd.setCursor(11, 1);
      lcd.print("  ");
      lcd.setCursor(11, 1);
      if ( tmpHours < 10 ) lcd.print(" ");
      lcd.print(tmpHours);
      timeRef = millis();
    }
    else if ( button == KEYPAD_SELECT )
    {
      while ( lcd.button() != KEYPAD_NONE ) ;
      timeOut = false;
      break;
    }
    delay(150);
  }

  if ( !timeOut ) transition(KEYPAD_SELECT);
  else transition(TIME_OUT);
}

void setAlarmMinutes()
{
  unsigned long timeRef;
  boolean timeOut = true;
  uint8_t tmpMinutes = 0;

  lcd.clear();
  lcd.print("Alarm Time");

  timeRef = millis();
  lcd.setCursor(0, 1);
  lcd.print("Set minutes:  0");

  while ( (unsigned long)(millis() - timeRef) < 5000 )
  {
    uint8_t button = lcd.button();

    if ( button == KEYPAD_UP )
    {
      tmpMinutes = tmpMinutes < 55 ? tmpMinutes + 5 : tmpMinutes;
      lcd.setCursor(13, 1);
      lcd.print("  ");
      lcd.setCursor(13, 1);
      if ( tmpMinutes < 10 ) lcd.print(" ");
      lcd.print(tmpMinutes);
      timeRef = millis();
    }
    else if ( button == KEYPAD_DOWN )
    {
      tmpMinutes = tmpMinutes > 0 ? tmpMinutes - 5 : tmpMinutes;
      lcd.setCursor(13, 1);
      lcd.print("  ");
      lcd.setCursor(13, 1);
      if ( tmpMinutes < 10 ) lcd.print(" ");
      lcd.print(tmpMinutes);
      timeRef = millis();
    }
    else if ( button == KEYPAD_SELECT )
    {
      while ( lcd.button() != KEYPAD_NONE ) ;
      timeOut = false;
      break;
    }
    delay(150);
  }

  if ( !timeOut )
  {
    alarmHours = tmpHours;
    alarmMinutes = tmpMinutes;
    transition(KEYPAD_SELECT);
  }
  else transition(TIME_OUT);
}
