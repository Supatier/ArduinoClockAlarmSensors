/**
  ---------------------------------------------------------
  Originaly by:
     Nick Lamprianidis { paign10.ln [at] gmail [dot] com }
  ---------------------------------------------------------

  Modified By Supatier (supatier@gmail.com)

*/

#include <Wire.h>  // Required by RTClib
#include <LiquidCrystal.h>  // Required by LCDKeypad
#include <LCDKeypad.h>
#include <SoftwareSerial.h>
#include <RTClib.h>
#include <DHT.h>

#define TIME_OUT 5
#define ALARM_TIME_MET 6
#define BUZZER_PIN 3
#define SNOOZE 10
#define DHTPIN 2
#define DHTTYPE DHT22
#define RELAY_PIN 11

int DD, MM, YY, H, M, S, set_state, up_state, down_state;
int btnCount = 0;
int sensorPin = A3;
int sensorValue = 0;

unsigned long previousMillis = 0;
unsigned long currentMillis;

String sDD;
String sMM;
String sYY;
String sH;
String sM;
String sS;
String ssid = "null";
String password = "027412345678";
String data;
String server = "pensamig.000webhostapp.com"; // www.example.com
String uri = "/esp.php";// our example is /esppost.php

DHT dht(DHTPIN, DHTTYPE);
LCDKeypad lcd;
RTC_DS1307 RTC;
DateTime now;  // Holds the current date and time information
SoftwareSerial esp(12, 13);

int8_t button;  // Holds the current button pressed
uint8_t alarmHours = 0, alarmMinutes = 0;  // Holds the current alarm time
uint8_t tmpHours;
boolean alarm = false;  // Holds the current state of the alarm
unsigned long timeRef;

enum states
{
  SHOW_TIME,
  SHOW_TIME_ALARM_ON,
  SHOW_ALARM_TIME,
  SET_ALARM_HOUR,
  SET_ALARM_MINUTES,
  BUZZER_ON
};

states state;  // Holds the current state of the system


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

void setup()
{
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  esp.begin(115200);
  Serial.begin(115200);
  lcd.begin(16, 2);
  Wire.begin();
  RTC.begin();
  lcd.createChar(0, term);
  lcd.createChar(1, pic);
  reset();
  connectWifi();
  state = SHOW_TIME;
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  esp.begin(115200);
  Serial.begin(115200);
  //RTC.adjust(DateTime(__DATE__, __TIME__));
  delay(500);
  lcd.clear();
}

void loop()
{
  timeRef = millis();
  sensorValue = analogRead(sensorPin);
  if (sensorValue < 60) {
    while (sensorValue < 60) {
      sensorValue = analogRead(sensorPin);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("FIRE");
      analogWrite(BUZZER_PIN, 255);
      digitalWrite(RELAY_PIN, HIGH);
      sensorValue = analogRead(sensorPin);
      delay(5000);
      digitalWrite(RELAY_PIN, LOW);
      sensorValue = analogRead(sensorPin);
      delay(1000);
    }
    analogWrite(BUZZER_PIN, 0);
    digitalWrite(RELAY_PIN, LOW);
  }

  else {
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
}

void reset() {
  esp.println("AT+RST");
  delay(1000);
  if (esp.find("OK") )
    Serial.println("Module Reset");
  lcd.setCursor(0, 0);
  lcd.print("Module Reset");
}

void transition(uint8_t trigger) {
  switch (state)
  {
    case SHOW_TIME:
      if ( trigger == KEYPAD_LEFT ) state = SHOW_ALARM_TIME;
      else if ( trigger == KEYPAD_RIGHT ) {
        alarm = true;
        lcd.clear();
        state = SHOW_TIME_ALARM_ON;
      }
      else if ( trigger == KEYPAD_SELECT ) state = SET_ALARM_HOUR;
      break;
    case SHOW_TIME_ALARM_ON:
      if ( trigger == KEYPAD_LEFT ) state = SHOW_ALARM_TIME;
      else if ( trigger == KEYPAD_RIGHT ) {
        alarm = false;
        lcd.clear();
        state = SHOW_TIME;
      }
      else if ( trigger == KEYPAD_SELECT ) state = SET_ALARM_HOUR;
      else if ( trigger == ALARM_TIME_MET ) {
        analogWrite(BUZZER_PIN, 255);
        state = BUZZER_ON;
      }
      break;
    case SHOW_ALARM_TIME:
      if ( trigger == TIME_OUT ) {
        lcd.clear();
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
        lcd.clear();
        alarm = false; state = SHOW_TIME;
      }
      break;
  }
}

void showTime() {
  now = RTC.now();
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

void get_sens() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  lcd.setCursor(0, 1);
  lcd.write(byte(0));
  lcd.print(" ");
  lcd.print(t, 1);
  lcd.print((char)223); //degree sign
  lcd.print("C ");
  lcd.write(byte(1));
  lcd.print(" ");
  lcd.print(h, 0);
  lcd.print("%");
  lcd.print(alarm ? " #" : "");

  data = "temperature=" + String(t) + "&humidity=" + String(h);// data sent must be under this form //name1=value1&name2=value2.
  httppost();
}

void showAlarmTime() {
  lcd.clear();
  lcd.print("Alarm Time");
  lcd.setCursor(0, 1);
  lcd.print(String("HOUR: ") + ( alarmHours < 9 ? "0" : "" ) + alarmHours +
            " MIN: " + ( alarmMinutes < 9 ? "0" : "" ) + alarmMinutes);
  delay(2000);
  transition(TIME_OUT);
}

void checkAlarmTime() {
  if ( now.hour() == alarmHours && now.minute() == alarmMinutes ) transition(ALARM_TIME_MET);
}

void snooze() {
  alarmMinutes += SNOOZE;
  if ( alarmMinutes > 59 )
  {
    alarmHours += alarmMinutes / 60;
    alarmMinutes = alarmMinutes % 60;
  }
}

void setAlarmHours() {
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

void setAlarmMinutes() {


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

void httppost () {

  esp.println("AT+CIPSTART=\"TCP\",\"" + server + "\",80");//start a TCP connection.

  if ( esp.find("OK")) {

    Serial.println("TCP connection ready");

  }
  delay(200);

  String postRequest =
    "POST " + uri + " HTTP/1.0\r\n" +
    "Host: " + server + "\r\n" +
    "Accept: *" + "/" + "*\r\n" +
    "Content-Length: " + data.length() + "\r\n" +
    "Content-Type: application/x-www-form-urlencoded\r\n" +
    "\r\n" + data;

  String sendCmd = "AT+CIPSEND=";//determine the number of caracters to be sent.
  esp.print(sendCmd);
  esp.println(postRequest.length() );
  delay(200);

  if (esp.find(">")) {
    Serial.println("Sending.."); esp.print(postRequest);
    if ( esp.find("SEND OK")) {
      Serial.println("Packet sent");
      while (esp.available()) {
        String tmpResp = esp.readString();
        Serial.println(tmpResp);
      }
      // close the connection
      esp.println("AT+CIPCLOSE");
    }
  }
}

void connectWifi() {
  String cmd = "AT+CWJAP=\"" + ssid + "\",\"" + password + "\"";
  esp.println(cmd);
  delay(2000);
  if (esp.find("OK")) {
    Serial.println("Connected!");
    lcd.setCursor(0, 1);
    lcd.print("Connected!");
  }

  else {
    connectWifi();
    lcd.clear();
    Serial.println("Cannot connect to wifi");
    lcd.setCursor(0, 1);
    lcd.print("Cannot connect to wifi");
  }

}
