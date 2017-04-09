#include <stdint.h>
#include <TFTv2.h>
#include <SPI.h>
#include <Wire.h>
#include <DS3231.h>
#include <SD.h>

const int chipSelect = 10;    // SD module CSS pin

const int button1 = 2;
const int button2 = 3;
const int button3 = 8;
const int LCD_LIGHT = 7;

int button1_counter = 0;
int button2_counter = 0;
int button3_counter = 0;
int changeClock_counter = 0;

int button1_state = 0;
int button2_state = 0;
int button3_state = 0;

int button1_last = 0;
int button2_last = 0;
int button3_last = 0;

int BPM_same = 0;

DS3231 Clock;
bool Century=false;
bool h12;
bool PM;

int year = 0;
int month = 0;
int date = 0;
int hour = 0;
int minute = 0;
int second = 0;

int year_DEL = 100;
int month_DEL = 100;
int date_DEL = 100;
int hour_DEL = 100;
int minute_DEL = 100;
int second_DEL = 100;

int MAX_BPM_GRAPH = 0;
boolean Record_ID = true;

int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int fadePin = 9;                  // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin
int TEMP = 0;
int TEMP_DEL = 0;
int BPM_color = 0xFFF0;   //COLOR RGB565 http://www.henningkarlsen.com/electronics/calc_rgb565.php
int BPM_DEL = 0;
int BPM_MIN = 300;
int BPM_MAX = 0;
int BPM_AVG = 0;
int BPM_AVG_DEL = 0;
int BPM_AVG_TOTAL = 0;
int count = 1;
boolean NONUM = true;
int x1=0;int x2=0;int x3=0;int x4=0;int x5=0;int x6=0;int x7=0;int x8=0;int x9=0;int x10=0;
int x11=0;

int m1=315;int m2=315;int m3=315;int m4=315;int m5=315;int m6=315;int m7=315;int m8=315;int m9=315;int m10=315;
int m11=315;
// these variables are volatile because they are used during the interrupt service routine!
volatile int BPM;                   // used to hold the pulse rate
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // holds the time between beats, must be seeded! 
volatile boolean Pulse = false;     // true when pulse wave is high, false when it's low
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.


void setup(){
  
  pinMode(fadePin,OUTPUT);          // pin that will fade to your heartbeat!
  Serial.begin(115200);             // we agree to talk fast!
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
  
  pinMode(10, OUTPUT);
  pinMode(LCD_LIGHT, OUTPUT);
  
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(button3, INPUT);
  
  Wire.begin();
  
  TFT_BL_ON;                                      // turn on the background light   
  Tft.TFTinit();                                  // init TFT library 
  Tft.drawString("ECG HEART RATE PULSE",0,10,2,GRAY1);
  
  if (!SD.begin(chipSelect)) {
    return;
  }
  delay(100);

  //draw heart sharp
  Tft.fillCircle(25,65,15,RED);  //center: (190, 65), r = 15 ,color : RED
  Tft.fillCircle(55,65,15,RED);
  Tft.fillCircle(40,80,18,RED);
  Tft.fillCircle(40,98,8,RED);
  for (int x=0; x<6; x++){
    int xl1=x+11;
    int xl2=x+32;
    int xr1=x+64;
    int xr2=x+43;
    Tft.drawLine(xl1,71,xl2,100,RED);  //start: (0, 0) end: (239, 315), color : RED
    Tft.drawLine(xr1,71,xr2,100,RED);
  }
  Tft.drawString("---", 75, 50, 8, 0xFFF0);
  Tft.drawNumber(20,4,180,2,0xCB20);
  Tft.drawString("/",52,180,2,0xCB20); 
  Tft.drawString("/",88,180,2,0xCB20);  
  Tft.drawString(":",160,180,2,0xCB20);
  Tft.drawString(":",196,180,2,0xCB20);
  
  Tft.drawString("TEP",19,125,2,GRAY1);
  Tft.drawString("MIN",74,125,2,GRAY1);
  Tft.drawString("MAX",129,125,2,GRAY1);
  Tft.drawString("AVG",184,125,2,GRAY1);
  
  Tft.drawRectangle(0,220,239,100,GRAY2);
  Tft.fillRectangle(0,220,21,100,GRAY2);
        
  Tft.drawString("15O",1,222,1,0xFFF0);
  Tft.drawString("1OO",1,251,1,0xFFF0);
  Tft.drawString("5O",7,280,1,0xFFF0);
  Tft.drawString("O",13,309,1,0xFFF0);
  
}



void loop(){
  
  String dataString = "";
  
  getClock();
  
  // turn on or off the lcd backlite 
  button2_state = digitalRead(button2);
  if (button2_state != button2_last){
    if(button2_state == HIGH){
      button2_counter++;
    }
  }
  button2_last = button2_state;

  if (button2_counter % 2 == 0){
    digitalWrite(LCD_LIGHT, HIGH);
  }
  else{
    digitalWrite(LCD_LIGHT, LOW);
  }
  


  button3_state = digitalRead(button3);
  if(button3_state == HIGH){
    changeClock();
  }
  
  button1_state = digitalRead(button1);
  
  if (button1_state != button1_last){
    if (button1_state == HIGH){
      button1_counter++;
    }

  }
  button1_last = button1_state;


  if (QS == true){                       // Quantified Self flag is true when arduino finds a heartbeat
        fadeRate = 255;                  // Set 'fadeRate' Variable to 255 to fade LED with pulse
        QS = false;                      // reset the Quantified Self flag for next time  
        BPM = BPM/2; // BPM = Sensor connect 5V, BPM/2 = Sensor connect 3.3V
        
        if (NONUM == true){
          Tft.drawString("---", 75, 50, 8, BLACK);
          NONUM = false;
        }
        
        Tft.drawNumber(BPM_DEL, 75, 50, 8, BLACK);
        Tft.drawNumber(BPM, 75, 50, 8, 0xFFF0);
        BPM_DEL = BPM;
        
        
        if (button1_counter % 2 == 0){
          if (Record_ID == true){   
            Tft.drawHorizontalLine(0, 35, 240, 0xFFF0); // (x, y, length, color)
            Tft.drawHorizontalLine(0, 36, 240, 0xFFF0); // (x, y, length, color)
            Record_ID =! Record_ID;
            
          }

        }
        else{
          if (Record_ID == false){
            Tft.drawHorizontalLine(0, 35, 240, GREEN); // (x, y, length, color)
            Tft.drawHorizontalLine(0, 36, 240, GREEN); // (x, y, length, color)
            Record_ID =! Record_ID;
          }
          
          BPM_same++;
          
          if (BPM_same == 1){   // time to record heartbeat
            dataString = "20"+String(Clock.getYear())+"/"+String(Clock.getMonth(Century))+"/"+String(Clock.getDate())+", "+String(Clock.getHour(h12, PM))+":"+String(Clock.getMinute())+":"+String(Clock.getSecond())+", "+String(BPM)+", "+String(TEMP);
            File dataFile = SD.open("datalog.CSV", FILE_WRITE);
    
            if (dataFile) {
              dataFile.println(dataString);
              dataFile.close();
  
            }
            BPM_same = 0;
          }
    
        }
        
        TEMP = Clock.getTemperature();
        if(TEMP_DEL != TEMP){
        Tft.drawNumber(TEMP_DEL,19,143,2,BLACK);
        Tft.drawNumber(TEMP,19,143,2,0xFFF0);
        TEMP_DEL = TEMP;
        }

        if(BPM_MIN > BPM){
          Tft.drawNumber(BPM_MIN,74,143,2,BLACK);
          Tft.drawNumber(BPM,74,143,2,0xFFF0);
          BPM_MIN = BPM;
        }
        
        if(BPM_MAX < BPM){
          Tft.drawNumber(BPM_MAX,129,143,2,BLACK);
          Tft.drawNumber(BPM,129,143,2,0xFFF0);
          BPM_MAX = BPM;
        }
        
       BPM_AVG_TOTAL = BPM_AVG_TOTAL + BPM;
       BPM_AVG = BPM_AVG_TOTAL/count;
       Tft.drawNumber(BPM_AVG_DEL,184,143,2,BLACK);
       if (count > 101){ BPM_AVG_TOTAL = 0; count = 0;}  //reset the total bpm every 100 times, prevents it over 16bit max number.
       Tft.drawNumber(BPM_AVG,184,143,2,0xFFF0);
       BPM_AVG_DEL = BPM_AVG;
       
       count++;
       
       if (BPM > 145){
         MAX_BPM_GRAPH = 145;
       }
       else{
         MAX_BPM_GRAPH = BPM;
       }
 
        
        x11 = x10; x10 = x9; x9 = x8; x8 = x7; x7 = x6; x6 = x5; x5 = x4; x4 = x3; x3 = x2; x2 = x1; x1 = MAX_BPM_GRAPH;
        
        for (int bold=0; bold<3; bold++){
        Tft.drawLine(238,m1+bold,217,m2+bold,BLACK); Tft.drawLine(217,m2+bold,196,m3+bold,BLACK); Tft.drawLine(196,m3+bold,175,m4+bold,BLACK); Tft.drawLine(175,m4+bold,154,m5+bold,BLACK); Tft.drawLine(154,m5+bold,133,m6+bold,BLACK);
        Tft.drawLine(133,m6+bold,112,m7+bold,BLACK); Tft.drawLine(112,m7+bold,89,m8+bold,BLACK); Tft.drawLine(89,m8+bold,68,m9+bold,BLACK); Tft.drawLine(68,m9+bold,47,m10+bold,BLACK); Tft.drawLine(47,m10+bold,22,m11+bold,BLACK);
        
        
        }
        
        m1 = map(x1,0,150,315,218); m2 = map(x2,0,150,315,218); m3 = map(x3,0,150,315,218); m4 = map(x4,0,150,315,218); m5 = map(x5,0,150,315,218); 
        m6 = map(x6,0,150,315,218); m7 = map(x7,0,150,315,218); m8 = map(x8,0,150,315,218); m9 = map(x9,0,150,315,218); m10 = map(x10,0,150,315,218);
        m11 = map(x11,0,150,315,218);
        
        
        for (int bold=0; bold<3; bold++){
        Tft.drawLine(238,m1+bold,217,m2+bold,RED); Tft.drawLine(217,m2+bold,196,m3+bold,RED); Tft.drawLine(196,m3+bold,175,m4+bold,RED); Tft.drawLine(175,m4+bold,154,m5+bold,RED); Tft.drawLine(154,m5+bold,133,m6+bold,RED);
        Tft.drawLine(133,m6+bold,112,m7+bold,RED); Tft.drawLine(112,m7+bold,89,m8+bold,RED); Tft.drawLine(89,m8+bold,68,m9+bold,RED); Tft.drawLine(68,m9+bold,47,m10+bold,RED); Tft.drawLine(47,m10+bold,22,m11+bold,RED);
        
        
        }
  
  }
  
  ledFadeToBeat();
  
  delay(20);                             //  take a break
  
  
}


void ledFadeToBeat(){
    fadeRate -= 50;                         //  set LED fade value
    fadeRate = constrain(fadeRate,0,255);   //  keep LED fade value from going into negative numbers!
    analogWrite(fadePin,fadeRate);          //  fade LED
}

void getClock(){
  int second,minute,hour,date,month,year,temperature;
  year = Clock.getYear();
  month = Clock.getMonth(Century);
  date = Clock.getDate();
  hour = Clock.getHour(h12, PM);
  minute = Clock.getMinute();
  second = Clock.getSecond();
  
  if (year != year_DEL){
    Tft.fillRectangle(28,180,24,14,BLACK);
    //Tft.drawNumber(year_DEL,28,180,2,BLACK);
    Tft.drawNumber(year,28,180,2,0xCB20);
    year_DEL = year;
  }
  
  if (month != month_DEL){
    Tft.fillRectangle(64,180,24,14,BLACK);
    if (month >= 0 && month < 10){
      Tft.drawNumber(0,64,180,2,0xCB20);
      Tft.drawNumber(month,76,180,2,0xCB20);
    }
    else{
      Tft.drawNumber(month,64,180,2,0xCB20);
    }
    month_DEL = month;
  }
  
  if (date != date_DEL){
    Tft.fillRectangle(100,180,24,14,BLACK);
    if (date >= 0 && date < 10){
      Tft.drawNumber(0,100,180,2,0xCB20);
      Tft.drawNumber(date,112,180,2,0xCB20);
    }
    else{
      Tft.drawNumber(date,100,180,2,0xCB20);
    }
    date_DEL = date;
  }
  
  if (hour != hour_DEL){
    Tft.fillRectangle(136,180,24,14,BLACK);
    if (hour >= 0 && hour < 10){
      Tft.drawNumber(0,136,180,2,0xCB20);
      Tft.drawNumber(hour,148,180,2,0xCB20);
    }
    else{
      Tft.drawNumber(hour,136,180,2,0xCB20);
    }
    hour_DEL = hour;
  }
  
  if (minute != minute_DEL){
    Tft.fillRectangle(172,180,24,14,BLACK);
    if (minute >= 0 && minute < 10){
      Tft.drawNumber(0,172,180,2,0xCB20);
      Tft.drawNumber(minute,184,180,2,0xCB20);
    }
    else{
      Tft.drawNumber(minute,172,180,2,0xCB20);
    }
    minute_DEL = minute;
  } 
  
  if (second != second_DEL){
    Tft.fillRectangle(208,180,24,14,BLACK);
    if (second >= 0 && second < 10){
      Tft.drawNumber(0,208,180,2,0xCB20);
      Tft.drawNumber(second,220,180,2,0xCB20);
    }
    else{
      Tft.drawNumber(second,208,180,2,0xCB20);
    }
    second_DEL = second;
  }
}


                                                            //after system restart, the 0 time will blackout!!!!


void changeClock(){
  while (button3_state == HIGH){
    
    button1_state = digitalRead(button1);
    if (button1_state != button1_last){
      if (button1_state == HIGH){
        changeClock_counter++;
      }
    }
    button1_last = button1_state;
    
    if (changeClock_counter == 1){
      if (digitalRead(button2) == HIGH){
        year++;
        delay(300);
        Clock.setYear(year);
        getClock();
        if (year > 40){
          year = 10;
        }
      }  
  }
  if (changeClock_counter == 2){
      if (digitalRead(button2) == HIGH){
        month++;
        delay(300);
        Clock.setMonth(month);
        getClock();
        if (month > 11){
          month = 0;
        }
      }  
  }
  if (changeClock_counter == 3){
      if (digitalRead(button2) == HIGH){
        date++;
        delay(300);
        Clock.setDate(date);
        getClock();
        if (date > 30){
          date = 0;
        }
      }  
  }
  if (changeClock_counter == 4){
      if (digitalRead(button2) == HIGH){
        delay(300);
        if (hour > 23){
          hour = 0;
        }
        Clock.setHour(hour);
        getClock();
        hour++; 
      }  
  }
  if (changeClock_counter == 5){
      if (digitalRead(button2) == HIGH){
        delay(300);
        if (minute > 59){
          minute = 0;
        }
        Clock.setMinute(minute);
        getClock();  
        minute++;
      }  
  }
  if (changeClock_counter == 6){
    changeClock_counter = 0;
  }
    button3_state = digitalRead(button3);

  }
}



