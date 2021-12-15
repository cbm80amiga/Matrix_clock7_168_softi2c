/*
 CONNECTIONS:
 
 pin 12 is connected to MAX7219 DataIn
 pin 11 is connected to MAX7219 CLK
 pin 10 is connected to MAX7219 LOAD/CS
 pins A4/A5 are connected to RTC module (or D6/D5 via SoftI2C)
 pin8 - mode/debug button
 BT module to RX=D1
 pin 3 - audio to base via 220ohm, emiter to GND, speaker to emiter and resistor 10ohm to VCC
 pin A0 - thermistor to VCC, A0 via R=10k to GND
*/

/*
 Serial commands:
 |Thhmmss  - set time (|T113025)
 |Dddmmyy  - set date (|D180116)
 |Wd       - set day of week 1..7 (|D6) 
 |Ahhmm    - set alarm (|A0710) 
 @@        - play alarm
 @1..@5    - set scroll speed
 @A..@E    - set LEDs brightness
 :), :(, :o, :B - smileys
 add # at the end when no line ending
*/

// Sketch uses 15,634 bytes (98%) of program storage space. Maximum is 15,872 bytes.

// NOKIA tunes:
// Triple
const unsigned char mess[] PROGMEM = "T:d=8,o=5,b=635:c,e,g,c,e,g,c,e,g,c6,e6,g6,c6,e6,g6,c6,e6,g6,c7,e7,g7,c7,e7,g7,c7,e7,g7";
// Happy birthday
const unsigned char bday[] PROGMEM = "B:d=4,o=5,b=125:8d.,16d,e,d,g,2f#,8d.,16d,e,d,a,2g,8d.,16d,d6,b,g,f#,2e,8c.6,16c6,b,g,a,2g";
// Mission Impossible
const unsigned char alarm0[] PROGMEM = "M:d=32,o=5,b=125:d6,d#6,d6,d#6,d6,d#6,d6,d#6,d6,d#6,d6,d#6,d6,d#6,d6,d#6,d6,d#6,d6,d#6,e6,f#6,d#6,8g.6,8g.,8g.,8a#,8c6,8g.,8g.,8f,8f#,8g.,8g.,8a#,8c6,8g.,8g.,8f,8f#,16a#6,16g6,2d6,16a#6,16g6,2c#6,16a#6,16g6,2c6,16a#,8c.6,4p,16a#,16g,2f#6,16a#,16g,2f6,16a#,16g,2e6,16d#6,8d.6";
//const unsigned char alarm1[] PROGMEM = "Popcorn:d=4,o=5,b=160:8c6,8a#,8c6,8g,8d#,8g,c,8c6,8a#,8c6,8g,8d#,8g,c,8c6,8d6,8d#6,16c6,8d#6,16c6,8d#6,8d6,16a#,8d6,16a#,8d6,8c6,8a#,8g,8a#,c6";
// Smurfs
const unsigned char alarm1[] PROGMEM = "S:d=32,o=5,b=200:4c#6,16p,4f#6,p,16c#6,p,8d#6,p,8b,p,4g#,16p,4c#6,p,16a#,p,8f#,p,8a#,p,4g#,4p,g#,p,a#,p,b,p,c6,p,4c#6,16p,4f#6,p,16c#6,p,8d#6,p,8b,p,4g#,16p,4c#6,p,16a#,p,8b,p,8f,p,4f#";

#define LOW_MEM_DEBUG 0
#define DEBUG_RTC     0

#define USE_RTC       1  // 1 for RTC clock module (default)
#define USEHW         0  // 1 for hw I2C RTC on A4/A5 pins
#define NUM_MAX       4

#if USEHW==1
#include <Wire.h>
#else
//#define SDA_PORT PORTC
//#define SDA_PIN 4  // A4
//#define SCL_PORT PORTC
//#define SCL_PIN 5  // A5

#define SDA_PORT PORTD
#define SDA_PIN 6  // D6
#define SCL_PORT PORTD
#define SCL_PIN 5  // D5
#define I2C_FASTMODE 0
#include "SoftI2CMaster.h"
#endif  

// MAX7219 matrices pins
#define DIN_PIN 12
#define CS_PIN  11
#define CLK_PIN 10

#define BUTTON_MODE    8
#define AUDIO_PIN      3
#define THERMISTOR_PIN A0

#define CHIME_START   7
#define CHIME_END     23

#define BDAY_START    15
#define BDAY_END      22

#define NIGHT_START   22
#define NIGHT_END     6

volatile uint8_t *buttonReg;
uint8_t buttonMask;

int hourAlarm=6, minuteAlarm=50;
int hour=19,minute=21,second=0;
int year=2016,month=8,day=20,dayOfWeek=6;
#include "rtc.h"

#include "audio.h"
#include "max7219.h"
#include "fonts.h"

// ----------------------------------
#define CLOCKBIG    1
#define CLOCKMED    2
#define CLOCK       3
#define DATE        4
#define DATE1       5
#define DATE2       6
#define TEMP        7
#define CLOCKBIGJMP 8
#define SPECIAL     9
#define ALARM      10
#define DISPMAX    10

int dx = 0;
int dy = 0;
int alarmCnt = 0;
int pos = 8;
int cnt = -1;
int h1, h0, m1, m0, s1, s0, secFr, lastSec=-1, lastDay=-1;
int d1,d0,mn1,mn0,y1,y0,dw;
int mode = 0, prevMode = 0;
int stx=1;
int sty=1;
int st = 1;
int disp = 1, prevDisp = 1;
int tr1 = 0, tr2 = 0;
int trdisp1 = 1, trdisp2 = 1;
int trans = 0, prevTrans = 0;
int dots = 0;
int del = 40;
int commandMode = 0;
float temp = 0;
uint32_t startTime, diffTime, zeroTime;
int charCnt=0;
char charBuf[7];

// ----------------------------------
void setup() 
{
  Serial.begin (9600);
  //Serial.print("AT+NAMELED_CLOCK");
  //delay(1000);
  //Serial.print("AT+PIN4321");
  //delay(1000);
  //Serial.print("AT+BAUD7");
  pinMode(BUTTON_MODE, INPUT_PULLUP);
  pinMode(AUDIO_PIN, OUTPUT);
  buttonMask = digitalPinToBitMask(BUTTON_MODE);
  buttonReg = portInputRegister(digitalPinToPort(BUTTON_MODE));
  initMAX7219();
  clr();
  refreshAll();
  sendCmdAll(CMD_SHUTDOWN,1);
  sendCmdAll(CMD_INTENSITY,0);
  //Serial.println("Init");
#if USE_RTC==1

#if USEHW==1
  Wire.begin();
#else

#if DEBUG_RTC==1
  Serial.println(i2c_init()?"I2C OK" : "I2C ERR");
#else
  i2c_init();
#endif

#endif

  hourAlarm   = readRTCMem(0);
  minuteAlarm = readRTCMem(1);
#endif

#if DEBUG_RTC==1
  Serial.print("Alarm: ");Serial.print(hourAlarm);Serial.print(":");Serial.println(minuteAlarm);
#endif
  temp = readTherm();
  playChime();
  zeroTime = millis();
//  delay(1000);
//  playMessage();
//  delay(1000);
//  alarmCnt = 1; playAlarm();
}

// ----------------------------------
//long readIntTemp() {
//  long result;
//  // Read temperature sensor against 1.1V reference
//  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
//  delay(2); // Wait for Vref to settle
//  ADCSRA |= _BV(ADSC); // Convert
//  while (bit_is_set(ADCSRA, ADSC));
//  result = ADCL;
//  result |= ADCH << 8;
//  result = (result - 125) * 1075;
//  return result;
//}
// ----------------------------------

float readTherm() 
{
  float pad = 9960;          // balance/pad resistor value - 10k
  float thermr = 10000;      // thermistor nominal resistance - 10k
  long resist = pad*((1024.0 / analogRead(THERMISTOR_PIN)) - 1);
  float temp = log(resist);
  temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
  temp -= 273.15;
  return temp;
}

// ----------------------------------
int oldState = HIGH;
long debounce = 30;
long b1Debounce = 0;
long b1LongPress = 0;
int checkModeBt()
{
  if (millis() - b1Debounce < debounce)
    return 0;
  int state = digitalRead(BUTTON_MODE);
  if (state == oldState)
  {
    if(state == LOW && (millis() - b1LongPress > 500) )
      return -1;
    return 0;
  }
  oldState = state;
  b1Debounce = millis();
  if(state == LOW)
    b1LongPress = millis();
  return state == LOW ? 1 : 0;
}
// ----------------------------------
void updateTime()
{
#if USE_RTC==1
  getRTCDateTime();
#else
  diffTime = (millis() - zeroTime)/1000;
  if(diffTime<=0) return;
  zeroTime += diffTime*1000;
  for(int i=0;i<diffTime;i++) {
    second++;
    if(second>=60) {
      second=0;
      minute++;
      if(minute>=60) {
        minute=0;
        hour++;
        if(hour>=24) {
          hour=0;
          dayOfWeek++;
          if(dayOfWeek>7)
            dayOfWeek=1;
          day++;
          if(day>31) {
            day=1;
            month++;
            if(month>12) {
              month=1;
              year++;
            }
          }
        }
      }
    }
  }
#endif
}

// ----------------------------------
int ascii2int(char *buf) { return (buf[0]&0xf)*10+(buf[1]&0x0f); }

void setClock()
{
  int m = toupper(charBuf[0]);
  if(m!='T' && m!='D' && m!='W' && m!='A') return;
  switch(m) {
    case 'T':
      hour   = ascii2int(charBuf+1);
      minute = ascii2int(charBuf+3);
      second = ascii2int(charBuf+5);
#if USE_RTC==1
      setRTCTime();
#endif
      break;
    case 'D':
      day   = ascii2int(charBuf+1);
      month = ascii2int(charBuf+3);
      year  = ascii2int(charBuf+5)+2000;
#if USE_RTC==1
      setRTCDate();
#endif
      break;
    case 'W':
      dayOfWeek   = charBuf[1]&0xf;
#if USE_RTC==1
      setRTCDoW();
#endif
      break;
    case 'A':
      hourAlarm   = ascii2int(charBuf+1);
      minuteAlarm = (charBuf[3]&0xf)*10+(charBuf[4]&0x0f);
      //minuteAlarm   = ascii2int(charBuf+3);
#if USE_RTC==1
      writeRTCMem(0,hourAlarm);
      writeRTCMem(1,minuteAlarm);
#endif
#if DEBUG_RTC==1
      Serial.print("SetAlarm: ");Serial.print(hourAlarm);Serial.print(":");Serial.println(minuteAlarm);
#endif
      break;
  }
  playClockSet();
}

// ----------------------------------

void loop()
{
  startTime = millis();
  updateTime();
//  hour = 21; minute = 11;
  h1 = hour / 10;
  h0 = hour % 10;
  m1 = minute / 10;
  m0 = minute % 10;
  s1 = second / 10;
  s0 = second % 10;
  d1 = day / 10;
  d0 = day % 10;
  mn1 = month / 10;
  mn0 = month % 10;
  y1 = (year - 2000)/10;
  y0 = (year - 2000)%10;
  dw = dayOfWeek; // dw=0..6, dayOfWeek=1..7
  if(dw>6) dw -= 7;

  if(second!=lastSec) {
    lastSec = second;
    secFr = 0;
  } else
    secFr++;

  // correct time of my RTC module - every midnight -5 secs
  if(hour==0 && minute==0 && second==5 && lastDay!=day) {
    lastDay = day;
    second = 0;
    setRTCTime();
  }
  
  if (cnt < 0) cnt = second * 10;
  if (secFr == 0) cnt = 0;
  dots = (cnt % 40 < 20) ? 1 : 0;

  while (Serial.available() > 0) {
    byte c = Serial.read();
    int par = c;
    if(commandMode=='|') { // setClock mode
      if(par!=10 && par!=13 && par!='#' && charCnt<7) 
        charBuf[charCnt++] = par; 
      else {
        setClock();
        commandMode = 0;
        charCnt = 0;
      }
    } else
    if(commandMode=='@') {
      if(par=='@') alarmCnt = 1; else
      if(par>='0' && par<='9') { // @1 to @5 sets scroll speed
        par = c-'0';
        switch(par) {
          case 1:  del = 10; break;
          case 2:  del = 20; break;
          case 3:  del = 30; break;
          case 4:  del = 40; break;
          case 5:  del = 50; break;
          default: del = 30;
        }
      } else
      if(par>='A' && par<='F') { // @A to @E sets matrix brightness
        c -= 'A';
        switch(c) {
          case 0:  par = 0; break;
          case 1:  par = 1; break;
          case 2:  par = 3; break;
          case 3:  par = 5; break;
          case 4:  par = 10; break;
          default: par = 1;
        }
        sendCmdAll(CMD_INTENSITY, par);
      }
      commandMode = 0;
    } else if(commandMode==':') { // convert smileys
      commandMode = 0;
      switch(c) {
        case ')': c = 19+'~'; break; // ':)'
        case '(': c = 20+'~'; break; // ':('
        case 'o': c = 21+'~'; break; // ':o'
        case 'B': c = 22+'~'; break; // 'heart <3'
        default: scrollChar(':', del);
      }
      scrollChar(c, del);
    }
    else if(c=='@' || c==':' || c=='|') {
       commandMode = c;
    } else {
      //Serial.println(c, DEC);
      if(alarmCnt==0 && !(charCnt==0 && (c==10 || c==13 || c=='#'))) {
        if(charCnt==0) {
          playMessage();
          scrollChar(' ', del);
        }
        if(c==10 || c==13 || c=='#') { 
          charCnt = 0;
          startTime = millis();
          while (millis() - startTime < 10000) if(Serial.available()) break;
        } else
        scrollChar(c, del);
        charCnt++;
      }
    }
  }
  int but = checkModeBt();
  if(but<0) {
    mode=0;
  } else
  if(but>0) {
    if(++mode>DISPMAX) mode=0;
  }
  prevDisp = disp;
  switch(mode) {
    case 0: autoDisp(); break;
    default: disp = mode; break;
  }
  
  // night mode - only big clock, low intensity
  if(hour==NIGHT_START && minute==0 && second==0 && secFr==0) {
    prevMode = mode;
    mode = CLOCKBIG;
    sendCmdAll(CMD_INTENSITY, 0);
  } else  
  if(hour==NIGHT_END && minute==0 && second==0 && secFr==0)
    mode = prevMode;

  clr();
  if(disp!=prevDisp) {
    trans = 1 + (prevTrans % 4);
    prevTrans = trans;
    switch(trans) {
      case 1:  tr1 = 0; tr2 = -38; st = +1; break;
      case 2:  tr1 = 0; tr2 =  38; st = -1; break;
      case 3:  tr1 = 0; tr2 = -11<<1; st = +1; break;
      case 4:  tr1 = 0; tr2 =  11<<1; st = -1; break;
    }
    trdisp1 = prevDisp;
    trdisp2 = disp;
    if(prevDisp==CLOCKBIGJMP || disp==CLOCKBIGJMP) { trans=dx=dy=0; }
  }

  if(!trans) {
    render(disp);
  } else {
    if(trans==1 || trans==2) dx = tr1; else dy = tr1>>1;
    render(trdisp1);
    if(trans==1 || trans==2) dx = tr2; else dy = tr2>>1;
    render(trdisp2);
    tr1 += st;
    tr2 += st;
    if(tr2==0) trans = dx = dy = 0;
  }

  refreshAll();

  // play chime every full hour
  if(hour>=CHIME_START && hour<CHIME_END && minute==0 && second==0 && secFr==0)
    playChime();

  // play bday tune 5 minutes after full hour
  if(hour>=BDAY_START && hour<BDAY_END && minute==5 && disp==SPECIAL && trans==0 && isBDay()>=0)
    playBirthday();

  if(hour==hourAlarm && minute==minuteAlarm && second==0 && secFr==0 && isAlarmDay()) {
      alarmCnt = 5;
      disp = CLOCKBIG;
      trans=dx=dy=0;
  }

  playAlarm();

  cnt++;
  while (millis() - startTime < 25);
}

// ----------------------------------

const byte noAlarmDays[] PROGMEM = {
  1,1, 6,1,
  1,2, 2,2, 3,2, 4,2, 5,2, 
  8,2, 9,2, 10,2, 11,2, 12,2,
  29,3,
  6,4,
  1,5, 2,5, 3,5, 4,6, 1,11, 11,11, 23,12, 24,12, 25,12, 26,12
};

bool isAlarmDay()
{
  if(dw==6 || dw==0 || month==7 || month==8) return false; // no alarm on Sat, Sun and July, August
//  if(dw==4 || dw==5) return false; // no alarm on Thu, Fri
  for(int i=0;i<sizeof(noAlarmDays)/2;i++)
    if(day==pgm_read_byte(noAlarmDays+i*2) && month==pgm_read_byte(noAlarmDays+i*2+1)) return false;
  return true;
}

// ----------------------------------


void playAlarm()
{
  while(alarmCnt>0) {
    alarmCnt--;
    if(checkModeBt())  { alarmCnt=0; return; }
    dots = 1;
    trans=dx=dy=0;
    clr();
    render(CLOCKBIG);
    invert(); refreshAll();
#if LOW_MEM_DEBUG==0
    playRTTTL((day & 1) ? alarm1 : alarm0 );
#endif
    invert(); refreshAll();
    if(sleep(3000)) return;
  }
}

//void playAlarm()
//{
//  while(alarmCnt>0) {
//    alarmCnt--;
//    if(checkModeBt())  { alarmCnt=0; return; }
//    dots = 1;
//    trans=dx=dy=0;
//    clr();
//    render(CLOCKBIG);
//    invert(); refreshAll();
//    playPCM(horn4000, sizeof(horn4000), 4000);
//    invert(); refreshAll();
//    if(sleep(500)) return;
//
//    invert(); refreshAll();
//    playPCM(horn4000, sizeof(horn4000), 4000);
//    invert();  refreshAll();
//    if(sleep(500)) return;
//
//    invert(); refreshAll();
//    playPCM(pobudka6000, sizeof(pobudka6000), 6000);
//    invert(); refreshAll();
//    if(sleep(2000)) return;
//  }
//}
// ----------------------------------

void playMessage()
{
#if LOW_MEM_DEBUG==0
  playRTTTL(mess);
#endif
}

// ----------------------------------

void playBirthday()
{
#if LOW_MEM_DEBUG==0
  playRTTTL(bday);
#endif
}

// ----------------------------------

void playChime()
{
  playSound(AUDIO_PIN,2000,40);
  delay(200);
  playSound(AUDIO_PIN,2000,40);
}

// ----------------------------------

void playClockSet()
{
  playSound(AUDIO_PIN,1500,60);
  delay(100);
  playSound(AUDIO_PIN,1500,60);
  delay(100);
  playSound(AUDIO_PIN,1500,60);
}

// ----------------------------------

int sleep(int del)
{
  unsigned int st = millis();
  while(millis()-st < del) if(checkModeBt())  { alarmCnt=0; return 1; }
  return 0;
}

// ----------------------------------

byte dispTab[20] = {
  CLOCKMED, CLOCKMED, CLOCKMED, CLOCKMED, CLOCKMED, CLOCKMED, CLOCKMED,  // 7/20
  SPECIAL, DATE1, // 2/20
  CLOCKBIG,CLOCKBIG,CLOCKBIG,CLOCKBIG,CLOCKBIG,CLOCKBIG,CLOCKBIG,CLOCKBIG, // 8/20
  DATE2,TEMP,CLOCKMED // 3/20
};

void autoDisp()
{
  // seconds 0-59 -> 0-19, 3s steps
  disp = dispTab[second/3];
}

// ----------------------------------
void showTemp()
{
//  int t = (readIntTemp() / 10000) - 2;
//  if(t/10) showDigit(t / 10, 6, dig5x8sq);
//  showDigit(t % 10, 12, dig5x8sq);
//  showDigit(7, 18, dweek_pl);
  if(secFr==0) temp = readTherm() + 0.05;
//  Serial.println(temp);
  if(temp>0 && temp<99) {
    int t1=(int)temp/10;
    int t0=(int)temp%10;
    int tf=(temp-int(temp))*10.0;
    if(t1) showDigit(t1, 2, dig5x8sq);
    showDigit(t0, 8, dig5x8sq);
    showDigit(tf, 16, dig5x8sq);
  }
  setCol(14, 0x80);
  showDigit(7, 22, dweek_pl);
}

// 4+1+4+2+4+1+4+2+4+1+4 = 31
//  [ 4. 1.16]
void showDate()
{
  if(d1) showDigit(d1, 0, dig4x8);
  showDigit(d0, 5, dig4x8);
  if(mn1) showDigit(mn1, 11, dig4x8);
  showDigit(mn0, 16, dig4x8);
  showDigit(y1, 22, dig4x8);
  showDigit(y0, 27, dig4x8);
  setCol(10, 0x80);
  setCol(21, 0x80);
}

//  [ 4. 1.2016]
void showDate1()
{
  if(d1) showDigit(d1, 0, dig3x8);
  showDigit(d0, 4, dig3x8);
  if(mn1) showDigit(mn1, 9, dig3x8);
  showDigit(mn0, 12, dig3x8);
  showDigit(2, 18, dig3x8);
  showDigit(0, 22, dig3x8);
  showDigit(y1, 26, dig3x8);
  showDigit(y0, 29, dig3x8);
  setCol(8, 0x80);
  setCol(16,0x80);
}

// 4+1+4+ 3 +4+1+4 = 21 +1 +10 =32
//  [ 4. 1 MO]
void showDate2()
{
  if(d1) showDigit(d1, 0, dig4x8);
  showDigit(d0, 5, dig4x8);
  if(mn1) showDigit(mn1, 11, dig4x8);
  showDigit(mn0, 16, dig4x8);
//  showDigit(dw, 22, dweek_en);
  showDigit(dw, 22, dweek_pl);
  setCol(10, 0x80);
}

// 4+1+4+3+4+1+4=21 + 3+1+3
void showClock()
{
  if (h1 > 0) showDigit(h1, h1 == 2 ? 0 : 1, dig4x8);
  showDigit(h0, 5, dig4x8);
  showDigit(m1, 12, dig4x8);
  showDigit(m0, 17, dig4x8);
  showDigit(s1, 24, dig3x7);
  showDigit(s0, 28, dig3x7);
  setCol(10, dots ? 0x24 : 0);
}

//6+2+6+3+6+2+6 = 31
void showClockBig(int jump=0)
{
  if(jump && !trans) {
    dx+=stx; if(dx>25 || dx<-25) stx=-stx;
    dy+=sty; if(dy>6 || dy<-6) sty=-sty;
    delay(40); // ugly!
  }
  if (h1 > 0) showDigit(h1, h1 == 2 ? 1 : 2, dig6x8);
  showDigit(h0, 8, dig6x8);
  showDigit(m1, 17, dig6x8);
  showDigit(m0, 24, dig6x8);
  setCol(15, dots ? 0x24 : 0);
}

// 5+1+5+3+5+1+5+ 1+3+1+3=33
void showClockMed()
{
  if (h1 > 0) showDigit(h1, 0, dig5x8rn);
  showDigit(h0, h1==2 ? 6 : 5, dig5x8rn); // <20h display 1 pixel earlier for better looking dots
  showDigit(m1, 13, dig5x8rn);
  showDigit(m0, 19, dig5x8rn);
  showDigit(s1, 25, dig3x6);
  showDigit(s0, 29, dig3x6);
  setCol((hour==20) ? 12 : 11, dots ? 0x24 : 0); // 20:xx - dots 1 pixel later
}

void showAlarm()
{
  if (hourAlarm/10 > 0) showDigit(hourAlarm/10, 3, dig4x8);
  showDigit(hourAlarm%10, 8, dig4x8);
  showDigit(minuteAlarm/10, 15, dig4x8);
  showDigit(minuteAlarm%10, 20, dig4x8);
  setCol(13, 0x24);
}
// ----------------------------------

// put below more special days, holidays, birthdays, etc.

const byte specialDays[] PROGMEM = {
  1,1,  // NewYr
  26,5, // DzMamy
  23,6, // DzTaty
};

// this way it takes less memory than in PROGMEM
char specialText[][8] = {
  "!NewYr!",
  "\224DzMamy",
  "\224DzTaty",
};

int isBDay()
{
  for(int i=0;i<sizeof(specialDays)/2;i++)
    if(day==pgm_read_byte(specialDays+i*2) && month==pgm_read_byte(specialDays+i*2+1)) return i;
  return -1;
}

void showSpecial()
{
//day=14; month=2;
  int bd = isBDay();
  if(bd<0)
    showClockBig();
  else
    showString(0, specialText[bd]);
}

// ----------------------------------

void render(int d)
{
  switch(d) {
    case CLOCKBIG:    showClockBig(); break;
    case CLOCKBIGJMP: showClockBig(1); break;
    case CLOCKMED:    showClockMed(); break;
    case CLOCK:       showClock(); break;
    case DATE:        showDate(); break;
    case DATE1:       showDate1(); break;
    case DATE2:       showDate2(); break;
    case TEMP:        showTemp(); break;
    case SPECIAL:     showSpecial(); break;
    case ALARM:       showAlarm(); break;
//    case EMPTY:    break;
    default:          showClockMed(); break;
  }
}

// ----------------------------------

void showDigit(char ch, int col, const uint8_t *data)
{
  if(dy<-8 | dy>8) return;
  int len = pgm_read_byte(data);
  int w = pgm_read_byte(data + 1 + ch * len);
  col += dx;
  for (int i = 0; i < w; i++)
    if(col+i>=0 && col+i<NUM_MAX*8) {
      byte v = pgm_read_byte(data + 1 + ch * len + 1 + i);
      if(!dy) scr[col + i] = v; else scr[col + i] |= dy>0 ? v>>dy : v<<-dy;
    }
}

// ---------------------------------------------

int showChar(char ch, int col, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  int i,w = pgm_read_byte(data + 1 + ch * len);
  if(dy<-8 | dy>8) return w;
  col += dx;
  for (i = 0; i < w; i++)
    if(col+i>=0 && col+i<NUM_MAX*8) {
      byte v = pgm_read_byte(data + 1 + ch * len + 1 + i);
      if(!dy) scr[col + i] = v; else scr[col + i] |= dy>0 ? v>>dy : v<<-dy;
    }
  return w;
}

// ---------------------------------------------

void showString(int x, char *s)
{
  while(*s) {
    unsigned char c = convertPolish(*s++);
    if (c < ' ' || c > '~'+22) continue;
    c -= 32;
    int w = showChar(c, x, font);
    x += w+1;
  }
}

// ---------------------------------------------
void setCol(int col, byte v)
{
  if(dy<-8 | dy>8) return;
  col += dx;
  if(col>=0 && col<32)
    if(!dy) scr[col] = v; else scr[col] |= dy>0 ? v>>dy : v<<-dy;
}

// ---------------------------------------------

int printChar(unsigned char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  int i,w = pgm_read_byte(data + 1 + ch * len);
  for (i = 0; i < w; i++)
    scr[NUM_MAX*8 + i] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  scr[NUM_MAX*8 + i] = 0;
  return w;
}

// ---------------------------------------------

int dualChar = 0;

unsigned char convertPolish(unsigned char _c)
{
  unsigned char c = _c;
  if(c==196 || c==197 || c==195) {
    dualChar = c;
    return 0;
  }
  if(dualChar) {
    switch(_c) {
      case 133: c = 1+'~'; break; // 'ą'
      case 135: c = 2+'~'; break; // 'ć'
      case 153: c = 3+'~'; break; // 'ę'
      case 130: c = 4+'~'; break; // 'ł'
      case 132: c = dualChar==197 ? 5+'~' : 10+'~'; break; // 'ń' and 'Ą'
      case 179: c = 6+'~'; break; // 'ó'
      case 155: c = 7+'~'; break; // 'ś'
      case 186: c = 8+'~'; break; // 'ź'
      case 188: c = 9+'~'; break; // 'ż'
      //case 132: c = 10+'~'; break; // 'Ą'
      case 134: c = 11+'~'; break; // 'Ć'
      case 152: c = 12+'~'; break; // 'Ę'
      case 129: c = 13+'~'; break; // 'Ł'
      case 131: c = 14+'~'; break; // 'Ń'
      case 147: c = 15+'~'; break; // 'Ó'
      case 154: c = 16+'~'; break; // 'Ś'
      case 185: c = 17+'~'; break; // 'Ź'
      case 187: c = 18+'~'; break; // 'Ż'
      default:  break;
    }
    dualChar = 0;
    return c;
  }    
  switch(_c) {
    case 185: c = 1+'~'; break;
    case 230: c = 2+'~'; break;
    case 234: c = 3+'~'; break;
    case 179: c = 4+'~'; break;
    case 241: c = 5+'~'; break;
    case 243: c = 6+'~'; break;
    case 156: c = 7+'~'; break;
    case 159: c = 8+'~'; break;
    case 191: c = 9+'~'; break;
    case 165: c = 10+'~'; break;
    case 198: c = 11+'~'; break;
    case 202: c = 12+'~'; break;
    case 163: c = 13+'~'; break;
    case 209: c = 14+'~'; break;
    case 211: c = 15+'~'; break;
    case 140: c = 16+'~'; break;
    case 143: c = 17+'~'; break;
    case 175: c = 18+'~'; break;
    default:  break;
  }
  return c;
}

// ---------------------------------------------
void scrollChar(unsigned char c, int del) {
  c = convertPolish(c);
  if (c < ' ' || c > '~'+22) return;
  c -= 32;
  int w = printChar(c, font);
  for (int i=0; i<w+1; i++) {
    delay(del);
    scrollLeft();
    refreshAll();
  }
}

// ---------------------------------------------
void scrollString(char *s, int del)
{
  while(*s) scrollChar(*s++, del);
}

