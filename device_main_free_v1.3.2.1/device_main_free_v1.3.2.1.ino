/*
    Coinapp Pisonet Timer System
    CoinappTechnology
    ========================================================
    Version: 1.3.2.1
    Date: May 11, 2020
    Description:
      - Fix: Fail safe mode will ON even if there's a time.
      - Added: Determine failed (unknown) coin read for Multi coin-slot type.
      
    Facebook: https://www.facebook.com/CoinappTechnology
    Email: coinapptechnology@gmail.com
    ========================================================
    Author: Darvin G. Mamangon
    Facebook: https://www.facebook.com/darvgm
    Email: mamangondarvin@gmail.com
*/

//#include <EEPROM.h>
#include "SevenSegmentTM1637.h"

#define PIN_CLK 7
#define PIN_DIO 8

#define VERSION "1.3.2.1"
#define INTERVAL_COIN 150
#define PIN_COIN 2
#define PIN_COIN_2 3
#define PIN_SIGNAL 4
#define PIN_SPK 5
#define PIN_RELAY 6
#define PIN_LED 13
#define FAIL_SAFE 330000 //5 minutes and 30 seconds
#define EOF '\n'

SevenSegmentTM1637 timer(PIN_CLK, PIN_DIO);

long minutes = 0, credits = 0;
int seconds = 0, TIME_INTERVAL = 1000;
int PULSE1 = -1, PULSE2 = -1, PULSE3 = -1, PULSE4 = -1, PULSE5 = -1, PULSE6 = -1;
bool counting, coinInserted, READY, INIT, FAIL, WAIT, ADMIN, sound, display;
unsigned long creditsPreviousMillis = 0, coinsPreviousMillis = 0, failSafeMillis = 0;
unsigned int pulse = 0, pulse_2 = 0, COINSLOT = 0;
unsigned int COIN1 = 0, COIN2 = 0, COIN3 = 0, COIN4 = 0, COIN5 = 0, COIN6 = 0;
String string = "";

void setup() {
  pinMode(PIN_COIN, INPUT_PULLUP);
  pinMode(PIN_COIN_2, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_SIGNAL, OUTPUT);
  
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_SIGNAL, LOW);
  digitalWrite(PIN_LED, LOW);
  
  Serial.begin(19200);
  
  timer.init();
  timer.clear();
}

void loop() {
  unsigned long currentMillis = millis();

  //At start of windows if this device is not initialize after 2 minutes (WAIT_INIT) fail safe will activate.
  if (currentMillis >= FAIL_SAFE && !INIT && !FAIL) { //wait for 2 minutes before initialize on startup.
    FAIL = true;
    Pin(PIN_RELAY, HIGH);
    timer.print("FAIL");
  }
  
  if (WAIT && !FAIL && (currentMillis - failSafeMillis) >= FAIL_SAFE )  {
    WAIT = false;
    FAIL = true;
    Pin(PIN_RELAY, HIGH);
    timer.print("FAIL");
  }

  if (currentMillis - creditsPreviousMillis >= TIME_INTERVAL)  {
    creditsPreviousMillis = currentMillis;

    if (counting && !ADMIN) {
      Send(credits);                                //Send time to app.

      if (display) {
        seconds = credits % 60;
        minutes = ((credits - seconds) / 60);
        
        timer.setColonOn( minutes == 0 || minutes > 99 ? false : true );
        timer.clear();

        if (minutes <= 0) {
          timer.setCursor(0, credits > 9 ? 2 : 3 );
          timer.print(credits);
        } else {
          if (minutes > 9) {
            if (minutes > 99) {
              timer.setCursor(0, (minutes > 999 ? 0 : 1)); 
              timer.print(minutes);
              timer.blink(1000, 1);
            } else {
              timer.setCursor(0, 0);
              timer.print(String(minutes) + (seconds > 9 ? "" : "0" ) + String(seconds));
            }
          } else {
            timer.setCursor(0, 1);
            timer.print(String(minutes) + (seconds > 9 ? "" : "0" ) + String(seconds));
          }
        }
      }

      if (sound) {
        if (credits == 60) {
          tone(PIN_SPK, 2100, 5000);
        }
        
        if (credits <= 10) {
          tone(PIN_SPK, 2100, credits != 0 ? 650 : 2000);
        }
        
      }

      if (credits <= 0) {
        counting = false;
        credits = 0;
        Send("ZERO");
        failSafeMillis = currentMillis;
        WAIT = true;
        if (FAIL) {
          Pin(PIN_RELAY, HIGH);
          timer.clear();
          timer.print("  00");
        }
      }
      else {
        credits -= 1;
      }
    }
  }

  if (currentMillis - coinsPreviousMillis >= INTERVAL_COIN) {
    coinsPreviousMillis = currentMillis;
    
    if ( !coinInserted && !ADMIN ) {
      if ( ( COINSLOT == 0 || COINSLOT == 1 ) && pulse > 0) {
        if ( pulse > 1) {
          Send("COIN1");
          credits += pulse * COIN1;
        } else {
          Send("COIN1");
          credits += COIN1;
        }
      }
      else if (COINSLOT == 2) {
        if (pulse == 1) {
          Send("COIN1");
          credits += COIN1;
        }
        
        if (pulse_2 == 1) {
          Send("COIN2");
          credits += COIN2;
          if (FAIL) {
            Pin(PIN_RELAY, LOW);
            counting = true;
          }
          WAIT = false;
        }
        pulse_2 = 0;
      }
      else if (COINSLOT == 3) {
        if ( pulse == PULSE1) {
          Send("COIN1");
          credits += COIN1;
        } else if (pulse == PULSE2) {
          Send("COIN2");
          credits += COIN2;
        } else if (pulse == PULSE3) {
          Send("COIN3");
          credits += COIN3;
        } else if (pulse == PULSE4) {
          Send("COIN4");
          credits += COIN4;
        } else if (pulse == PULSE5) {
          Send("COIN5");
          credits += COIN5;
        } else if (pulse == PULSE6) {
          Send("COIN6");
          credits += COIN6;
        } else {
          if (pulse > 0) {
            Send("COIN");
            credits += pulse * COIN1;
          }
        }
      }

      if (pulse > 0) {
        if (FAIL) {
          Pin(PIN_RELAY, LOW);
          counting = true;
        }
        WAIT = false;
      }
      
      pulse = 0;
    }
    
    coinInserted = false;
  }
}

void serialEvent() {
  while (Serial.available() > 0) {
    char data = (char)Serial.read();
    
    if (data == '=' && READY) {
      if (string.equalsIgnoreCase("C1")){
        COIN1 = Serial.parseInt();
      }
      
      if (string.equalsIgnoreCase("C2")) {
        COIN2 = Serial.parseInt();
      }
      
      if (string.equalsIgnoreCase("C3")) {
        COIN3 = Serial.parseInt();
      }

      if (string.equalsIgnoreCase("C4")) {
        COIN4 = Serial.parseInt();
      }

      if (string.equalsIgnoreCase("C5")) {
        COIN5 = Serial.parseInt();
      }

      if (string.equalsIgnoreCase("C6")) {
        COIN6 = Serial.parseInt();
      }
      
      if (string.equalsIgnoreCase("P1")) {
        PULSE1 = Serial.parseInt();
      }
      
      if (string.equalsIgnoreCase("P2")) {
        PULSE2 = Serial.parseInt();
      }
      
      if (string.equalsIgnoreCase("P3")) {
       PULSE3 = Serial.parseInt();
      }

      if (string.equalsIgnoreCase("P4")) {
       PULSE4 = Serial.parseInt();
      }

      if (string.equalsIgnoreCase("P5")) {
       PULSE5 = Serial.parseInt();
      }

      if (string.equalsIgnoreCase("P6")) {
       PULSE6 = Serial.parseInt();
      }
      
      if (string.equalsIgnoreCase("CS")) {
        COINSLOT = Serial.parseInt();
        
        switch (COINSLOT) {
          case 0: {
            Send("L");
            break;
          }
          case 1: {
            Send("S");
            break;
          }
              
          case 2: {
            Send("D");
            attachInterrupt(digitalPinToInterrupt(PIN_COIN_2), coinISR_dual, FALLING);
            pulse_2 = 0;
            break;
          }
              
          case 3: {
            Send("M");
            break;
          }
            
        }
        attachInterrupt(digitalPinToInterrupt(PIN_COIN), coinISR, FALLING);
        credits = 0;
        pulse = 0;
        failSafeMillis = millis();
        INIT = true;
        WAIT = true;
        Pin(PIN_SIGNAL, HIGH);        
      }
      
      if (string.equalsIgnoreCase("CD")) {
        credits = Serial.parseInt();
        if (display && credits == 0){
          timer.setColonOn(false);
          timer.clear();
          timer.print("   0");
        }
      }
      
      if (string.equalsIgnoreCase("CD+")){
        credits += Serial.parseInt();
      }
      
      if (string.equalsIgnoreCase("CT")) {
        counting = boolean(Serial.parseInt());
      }
      
      if (string.equalsIgnoreCase("TI")) {
        TIME_INTERVAL = Serial.parseInt();
      }
      
      if (string.equalsIgnoreCase("AD")) {
        ADMIN  = boolean(Serial.parseInt());
        Pin(PIN_SIGNAL, ADMIN ? LOW : HIGH);
        failSafeMillis = ADMIN ? 0 : millis();
        WAIT = ADMIN ? false : true;
        
        if (display) {
            timer.setColonOn(false);
            timer.clear();
            timer.print( ADMIN ? "8888" : "   0" );
        }
      }
      
      if (string.equalsIgnoreCase("S")) {
        sound = boolean(Serial.parseInt()) ? false : true;
      }
      
      if (string.equalsIgnoreCase("D")) {
        display = boolean(Serial.parseInt()) ? false : true;
        if (display)
        {
          timer.clear();
          timer.print("   0");
        }
      }

      string = "";

    }
    else if (data == '?' && READY) {
      
      if (string.equalsIgnoreCase("C1")) {
        Send("C1", COIN1);
      }
      if (string.equalsIgnoreCase("C2")) {
        Send("C2", COIN2);
      }
      if (string.equalsIgnoreCase("C3")) {
        Send ("C3", COIN3);
      }
      if (string.equalsIgnoreCase("P1")) {
        Send("P1", PULSE1);
      }
      if (string.equalsIgnoreCase("P2")) {
       Send("P2", PULSE2);
      }
      if (string.equalsIgnoreCase("P3")) {
       Send("P3", PULSE3);
      }
      
      if (string.equalsIgnoreCase("CS")) {
       Send("CS", COINSLOT);
      }
      
      if (string.equalsIgnoreCase("AD")) {
        Send("AD", ADMIN);
      }
      
      if (string.equalsIgnoreCase("CT")) {
        Send("CT", counting);
      }
      
      if (string.equalsIgnoreCase("TI")) {
        Send ("TI", TIME_INTERVAL);
      }
      
      if (string.equalsIgnoreCase("CD")) {
        Send("CD", credits);
      }
      
      if (string.equalsIgnoreCase("VERSION")) {
        Send("VERSION", VERSION);
      }
      
      string = ""; //Clear data string after received.
      
    }
    else if (data == ';') {
      
      if (string.equalsIgnoreCase("COINAPP") && !READY) {
        Send("READY");
        READY = true;
      }

      if (string.equalsIgnoreCase("LIVE") && READY) {
        Send("ALIVE");
      }
      
      if (string.equalsIgnoreCase("DC") && READY) {
        Pin(PIN_SIGNAL, LOW);
        Pin(PIN_RELAY, LOW);
        detachInterrupt(digitalPinToInterrupt(PIN_COIN));
        detachInterrupt(digitalPinToInterrupt(PIN_COIN_2));
        counting = false;
        if (display) {
          timer.setColonOn(false);
          timer.clear();
        }
        READY = false;
        WAIT = false;
        
        credits = 0;
      }
      
      string = "";
      
    }
    else {
      string += data; //Combine all the characters.
    }
  }
}

void coinISR()  {
  delay(1);
  if (READY && !ADMIN && digitalRead(PIN_COIN) == LOW) {
    pulse++;
    coinInserted = true;
  } else  {
    pulse = 0;
  }
}

void coinISR_dual() {
  delay(1);
  if (READY && !ADMIN && digitalRead(PIN_COIN_2) == LOW) {
    pulse_2++;
    coinInserted = true;
  } else {
    pulse_2 = 0;
  }
}

void Pin(int pin, int state) {
  digitalWrite(pin, state);
}

void Send(String com, String data) {
  Serial.print(com);
  Serial.print(EOF);
  Serial.print(data);
  Serial.print(EOF);
}

void Send(String com, int data) {
  Serial.print(com);
  Serial.print(EOF);
  Serial.print(data);
  Serial.print(EOF);
}

void Send(String com) {
  Serial.print(com);
  Serial.print(EOF);
}

void Send (long int data) {
  Serial.print(data);
  Serial.print(EOF);
}
