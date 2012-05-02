/*
  LiquidCrystal Library - Hello World
 
 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 
 This example code is in the public domain.
 
 http://www.arduino.cc/en/Tutorial/LiquidCrystal
 */

/*
  SD card datalogger
 
 created  24 Nov 2010
 updated 2 Dec 2010
 by Tom Igoe
 
 This example code is in the public domain.
 
 */

// include the library code:
#include <LiquidCrystal.h>
#include <stdarg.h>
#include <Wire.h>
#include "RTClib.h"
#include <SD.h>
#include <string.h>
#include "Adafruit_MCP23008.h"

// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidCrystal lcd(0);

/*
// initialize the LiquidCrystal library with the numbers of the interface pins
 LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
 */

// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
RTC_DS1307 RTC;

// Input and Output pins 
int VoltPin = A2;
int AmpPin = A1;
int BDIpin = A3;
int CENABLE = 9;
int CTHROTTLE = 2;
int StartButton = 7;
int IntervalButton = 6;
int DurationButton = 5;
int TypeButton = 4;
int LoadButton = 3;
int Throttlepin = A0;
const int chipSelect = 8;

// Constants
float vcons = 26.6819932293214;
float acons = 7.48318715903728;

// Time Variables
uint32_t current;  //Current time
uint32_t start;    //Start time 
uint32_t cycle;    //Time since last read
uint32_t duration;    //Total time since start
uint32_t elap;     //Elapsed seconds since start

// Variables
float Volt;
float Amp;
float AmpF;
float VoltAverage;
float AmpAverage;
float AmpHourTotal = 0;
float AmpHour = 0;
float AmpHourMilli = 0;
float VoltIn;
int Voltcount;
float AmpIn;
float watt;  // Watts
float BDIin;  // Curtis Battery Discharge Indicator input
float BDIaverage;
float BDIP; // Battery Discharge Indicator percentage
int CCon;
int CCoff;
int samples = 0;
int cells;
int cellscycle = 0;
float cutoffV; // Low Voltage cutoff total
float elapm = 0;       //Elapsed minutes since start
float elapmillis = 0;       //Elapsed hours since start
float elapc = 0;       //cycle time between samples
float elapi = 0;       //cycle time between samples
float ampneut;      //power-on 0A reading of the current sensor IC
float cutoff; // Low Voltage cutoff per cell. 1.65 for Acid, 1.8 for Gel or AGM
int interval;    //The time in seconds between log entries
int testduration;    // Total test duration, in minutes
int load;        // Desired discharge current
int Throttlein;  // Manual throttle input
int Throttleout = 0;    // Output throttle
float Throttlep;
int Startcount = 0;  // variable to record if the start button has been pressed
int StartButton1; // 1st reading of Start button
int StartButton2; // 2nd reading of Start button
int StartButtonState; // Start button state
int IntervalButton1; // 1st reading of Interval button
int IntervalButton2; // 2nd reading of Interval button
int IntervalButtonState; // Interval button state
int DurationButton1; // 1st reading of Duration button
int DurationButton2; // 2nd reading of Duration button
int DurationButtonState; // Duration button state
int TypeButton1; // 1st reading of Type button
int TypeButton2; // 2nd reading of Type button
int TypeButtonState; // Type button state
int LoadButton1; // 1st reading of Load button
int LoadButton2; // 2nd reading of Load button
int LoadButtonState; // Load button state
int BattType; // Battery Type


File logfile;
Adafruit_MCP23008 mcp;
// Custom Battery level indicator
byte full[8] = {
  0xe,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f};
byte five[8] = {
  0xe,0x11,0x1f,0x1f,0x1f,0x1f,0x1f};
byte four[8] = {
  0xe,0x11,0x11,0x1f,0x1f,0x1f,0x1f};
byte three[8] = {
  0xe,0x11,0x11,0x11,0x1f,0x1f,0x1f};
byte two[8] = {
  0xe,0x11,0x11,0x11,0x11,0x1f,0x1f};
byte empty[8] = {
  0xe,0x11,0x11,0x11,0x11,0x11,0x1f};


void setup() {
  // Change to External if you desire to read from the 5v rail. Note, must connect jumper on PCB      
  analogReference(DEFAULT);  
  //Create battery icons
  lcd.createChar(0, full);
  lcd.createChar(1, five);
  lcd.createChar(2, four);
  lcd.createChar(3, three);
  lcd.createChar(4, two);
  lcd.createChar(5, empty);
  // set up the LCD's number of columns and rows: 
  lcd.begin(20, 4);
  Wire.begin(); //Important for RTClib.h
  mcp.begin();
  RTC.begin();
  //  Serial.begin(9600);
  //RTC.adjust(DateTime(__DATE__, __TIME__));    //sets RTC time to PC time at compiling.

  pinMode(CENABLE, OUTPUT);
  pinMode(CTHROTTLE, OUTPUT);
  pinMode(StartButton, INPUT);
  pinMode(IntervalButton, INPUT);
  pinMode(DurationButton, INPUT);
  pinMode(TypeButton, INPUT);
  pinMode(LoadButton, INPUT); 
  pinMode(Throttlepin, INPUT);  
  pinMode(10, OUTPUT); //LadyaADA says to put this in to make the SD work




  // This bit of code is here to center the amp meter IC's output. 
  // Takes an average reading of the output, and stores that into its "center" variable
  delay (1000);
  int count = 0;
  float totalA = 0;
  while (count < 512){
    AmpIn = analogRead(AmpPin);
    totalA = AmpIn + totalA;
    count++;
  }
  ampneut = totalA / (float)count;

  while (Startcount == 0){  
    IntervalButton1 = digitalRead(IntervalButton); // read Start Button and store it in StartButton1
    delay(10);                               // 10 milliseconds is a good amount of time
    IntervalButton2 = digitalRead(IntervalButton); // read the input again to check for bounces
    if (IntervalButton1 == IntervalButton2) {      // make sure we got 2 consistant readings!
      if (IntervalButton1 != IntervalButtonState) { // the button state has changed!
        if (IntervalButton1 == HIGH) {
          if (interval == 5) {
            interval = 10;
          } 
          else {
            if (interval == 10) {
              interval = 15;
            } 
            else {
              if (interval == 15) {
                interval = 30;
              } 
              else {
                if (interval == 30) {
                  interval = 60;
                } 
                else {
                  if (interval == 60) {
                    interval = 150;
                  } 
                  else {
                    if (interval == 150) {
                      interval = 300;
                    } 
                    else {
                      if (interval == 300) {
                        interval = 5;
                      } 

                    }
                  }
                }
              }
            }
          }
        }
      }
      DurationButton1 = digitalRead(DurationButton); // read Start Button and store it in StartButton1
      delay(10);                               // 10 milliseconds is a good amount of time
      DurationButton2 = digitalRead(DurationButton); // read the input again to check for bounces
      if (DurationButton1 == DurationButton2) {      // make sure we got 2 consistant readings!
        if (DurationButton1 != DurationButtonState) { // the button state has changed!
          if (DurationButton1 == HIGH) {
            if (testduration == 0) {
              testduration = 5;
            } 
            else {
              if (testduration == 5) {
                testduration = 10;
              } 
              else {
                if (testduration == 10) {
                  testduration = 15;
                } 
                else {
                  if (testduration == 15) {
                    testduration = 30;
                  } 
                  else {
                    if (testduration == 30) {
                      testduration = 60;
                    } 
                    else {
                      if (testduration == 60) {
                        testduration = 150;
                      } 
                      else {
                        if (testduration == 150) {
                          testduration = 300;
                        } 
                        else {
                          if (testduration == 300) {
                            testduration = 0;
                          } 

                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    TypeButton1 = digitalRead(TypeButton); // read Start Button and store it in StartButton1
    delay(10);                               // 10 milliseconds is a good amount of time
    TypeButton2 = digitalRead(TypeButton); // read the input again to check for bounces
    if (TypeButton1 == TypeButton2) {      // make sure we got 2 consistant readings!
      if (TypeButton1 != TypeButtonState) { // the button state has changed!
        if (TypeButton1 == HIGH) {
          if (BattType == 1) {
            BattType = 2;
          } 
          else {
            if (BattType == 2) {
              BattType = 3;
            } 
            else {
              if (BattType == 3) {
                BattType = 1;
              } 
            }
          }
        }
      }
    }
    LoadButton1 = digitalRead(LoadButton); // read Start Button and store it in StartButton1
    delay(10);                               // 10 milliseconds is a good amount of time
    LoadButton2 = digitalRead(LoadButton); // read the input again to check for bounces
    if (LoadButton1 == LoadButton2) {      // make sure we got 2 consistant readings!
      if (LoadButton1 != LoadButtonState) { // the button state has changed!
        if (LoadButton1 == HIGH) {
          if (load == 0) {
            load = -5;
          } 
          else {
            if (load == -5) {
              load = -10;
            } 
            else {
              if (load == -10) {
                load = -15;
              } 
              else {
                if (load == -15) {
                  load = -20;
                } 
                else {
                  if (load == -20) {
                    load = -40;
                  } 
                  else {
                    if (load == -40) {
                      load = 0;
                    } 
                  }
                }
              }
            }
          }
        }
      }
    }
    StartButton1 = digitalRead(StartButton); // read Start Button and store it in StartButton1
    delay(10);                               // 10 milliseconds is a good amount of time
    StartButton2 = digitalRead(StartButton); // read the input again to check for bounces
    if (StartButton1 == StartButton2) {      // make sure we got 2 consistant readings!
      if (StartButton1 != StartButtonState) { // the button state has changed!
        if (StartButton1 == HIGH) {
          Startcount ++;
        }
      }
    }
    lcd.setCursor(0, 0);
    lcd.print("Sample Period");
    lcd.setCursor(0, 1);
    lcd.print("Test Duration");
    lcd.setCursor(0, 2);
    lcd.print("Battery Type");
    lcd.setCursor(0, 3);
    lcd.print("Test Load");
    lcd.setCursor(15, 0);
    lcd.print(interval);
    lcd.print("s ");
    lcd.setCursor(15, 1);
    if (testduration == 0){          
      lcd.print("UNL");
    } 
    else {
      lcd.print(testduration);
      lcd.print("m ");
    }
    lcd.setCursor(15, 2);
    if (BattType == 1){          
      lcd.print("AGM ");
      cutoff = 1.8;
    }
    if (BattType == 2){
      lcd.print("GEL");
      cutoff = 1.8;
    }
    if (BattType == 3){
      lcd.print("Acid");
      cutoff = 1.65;
    }
    lcd.setCursor(15, 3);
    if (load == 0){          
      lcd.print("MAN ");
    } 
    else {
      lcd.print(load);
      lcd.print("A ");
    }    
  }// below this ends the button loops
  
  lcd.clear(); //clears the LCD
  // Set start and cycle timers to the time the system is started
  start = RTC.now().unixtime();
  cycle = RTC.now().unixtime();
  DateTime now = RTC.now();
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    lcd.setCursor(10, 2);
    lcd.print("Card failed");
    return;
  }  
  
  //SD.mkdir(filedate)
  char monthfolder[] = "00";
  monthfolder[0] = now.month()/10 + '0'; //To get 1st digit from month()
  monthfolder[1] = now.month()%10 + '0'; //To get 2nd digit from month()
  char dayfolder[] = "00";
  dayfolder[0] = now.day()/10 + '0'; //To get 1st digit from day()
  dayfolder[1] = now.day()%10 + '0'; //To get 2nd digit from day()
  /*
  filedate[4] = now.hour()/10 + '0'; //To get 1st digit from hour()
  filedate[5] = now.hour()%10 + '0'; //To get 2nd digit from hour()
  filedate[6] = now.minute()/10 + '0'; //To get 1st digit from minute()
  filedate[7] = now.minute()%10 + '0'; //To get 2nd digit from minute()
  */
  // create a new file
  
  if (BattType == 1){          
      char filename[] = "0000AGM.CSV";
  filename[0] = now.hour()/10 + '0'; //To get 1st digit from hour()
  filename[1] = now.hour()%10 + '0'; //To get 2nd digit from hour()
  filename[2] = now.minute()/10 + '0'; //To get 1st digit from minute()
  filename[3] = now.minute()%10 + '0'; //To get 2nd digit from minute()
  }
  if (BattType == 2){          
      char filename[] = "0000GEL.CSV";
  filename[0] = now.hour()/10 + '0'; //To get 1st digit from hour()
  filename[1] = now.hour()%10 + '0'; //To get 2nd digit from hour()
  filename[2] = now.minute()/10 + '0'; //To get 1st digit from minute()
  filename[3] = now.minute()%10 + '0'; //To get 2nd digit from minute()
  }
  if (BattType == 3){          
      char filename[] = "0000ACD.CSV";
  filename[0] = now.hour()/10 + '0'; //To get 1st digit from hour()
  filename[1] = now.hour()%10 + '0'; //To get 2nd digit from hour()
  filename[2] = now.minute()/10 + '0'; //To get 1st digit from minute()
  filename[3] = now.minute()%10 + '0'; //To get 2nd digit from minute()
  }
  
  
  if (! SD.exists("monthfolder/dayfolder/filename")) {
    // only open a new file if it doesn't exist
    SD.mkdir("monthfolder/dayfolder");
    logfile = SD.open("monthfolder/dayfolder/filename", FILE_WRITE); 
    // break;  // leave the loop!
  }

  logfile.print("Count,Seconds,Minutes,Curtis State,Volts,Amps,Watts,BDI,AH,Throtte,,");  //Write the data header
  // Calcuate current date and time, and print to the SD card
  //DateTime now = RTC.now();
  logfile.print(now.month(), DEC);
  logfile.print('/');
  logfile.print(now.day(), DEC);
  logfile.print('/');
  logfile.print(now.year(), DEC);
  logfile.print(',');
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print(",");
  logfile.print(interval);
  logfile.print("sec interval,");
  if (testduration == 0){          
    logfile.print("Unlimited duration,");
  } 
  else {
    logfile.print(testduration);
    logfile.print("min duration,");
  }
  if (BattType == 1){          
    logfile.print("AGM,");
  }
  if (BattType == 2){          
    logfile.print("GEL,");
  }
  if (BattType == 3){          
    logfile.print("Acid,");
  }
  if (load == 0){          
    logfile.println("Manual Throttle,");
  } 
  else {
    logfile.print(load);
    logfile.println("A Current Hold,");
  }    
  int CCon = 0;    //Curtis has not been turned on
  int CCoff = 0;   //Curtis has not been turned off
  int cells = 0;   //Have not yet detected cell count
  int Voltcount = 0;  // Have not yet measuered voltage

}
void loop(){
  int count = 0;
  float totalV = 0;
  float totalA = 0;
  float totalB = 0;

  //  Read input values 512 times, and average the values
  while (count < 512){
    float VoltIn = analogRead(VoltPin);
    float AmpIn = analogRead(AmpPin);
    float BDIin = analogRead(BDIpin);
    totalV += VoltIn;
    totalA += AmpIn;
    totalB += BDIin;
    count++;
  }
  VoltAverage = totalV / (float)count;
  AmpAverage = totalA / (float)count;
  BDIaverage = totalB / (float)count;

  Volt = VoltAverage / vcons; //convert 1024 to 38
  AmpF = AmpAverage - ampneut; // 2.5v is 0A, adjusting range
  Amp = AmpF / acons; // convert +- 512 to 50 from center
  watt = Volt * Amp;
  BDIP = BDIaverage / 10.24; // convert to Percent

  if (Volt > 0.5){
    Voltcount ++;    //Starts adding to this number once batteries are connected
  }

  // This section automatically detects the number of battery cells attached to the machine.
  // Determining the number of cells allows the program to appropriately set the low voltage cutoff
  if(cells == 0){
    if (Voltcount > 10){  //once the batteries have been connected for a few cycles, then take a cell count.
      if (Volt >= 4.8){
        cells = cells + 3;
        if (Volt >= 9.6){
          cells = cells + 3;
          if (Volt >= 19.2){
            cells = cells + 6;
            if (Volt >= 28.8){
              cells = cells + 6;
            }
          }
        }
      }
    }
    cutoffV = cells * cutoff; //Sets the low voltage cut off value
  } 

  if (cellscycle == 0){    //If it has not yet been here...
    if (cells != 0){       //And we have previously detected cell count
      //Write Ammeter center value, and Cell count to the SD card.
      logfile.print(ampneut);
      logfile.print(" Ammeter Center,");
      logfile.print(cells);
      logfile.print(" Cells,");
      logfile.print(cutoffV);
      logfile.println(" V Cutoff,");


      logfile.flush();
      cellscycle++;    //make sure it doesn't come through here again.
    }
  }

  if (load == 0){
    Throttlein = analogRead(Throttlepin);
    Throttleout = Throttlein;
  } 
  else{
    if ( digitalRead( CENABLE ) == HIGH){
      if (Amp>load){
        if(Amp>0.9*load){
          Throttleout+=10;
        }
        else{
          Throttleout++;
        }
      }
      else if(Amp==load){
        Throttleout;
      }
      else{
        if(Amp<1.1*load){
          Throttleout-=10;
        }
        else{
          Throttleout--;
        }
      }
    }
  }
  analogWrite(CTHROTTLE, Throttleout);
  Throttlep = Throttleout / 1024;




  elapc = elapmillis;    //Sets cycle time to elap from the last time around.
  // calculate Current Time
  current = RTC.now().unixtime();
  // Calculate time since start
  duration = current - cycle;
  elap = current - start;
  elapm=float(elap)/60.0;
  elapmillis = millis();
  elapi = elapmillis - elapc;
  /*
        New Amp Hour Calculation Section
   */
  AmpHourMilli = Amp * elapi;
  AmpHour = AmpHourMilli / 3600000;
  AmpHourTotal += AmpHour;

  /*
       Verifies that Cell number and cutoff have been determined
   then checks to verify that the batteries are not being charged.
   Checks if the Curtis has been turned off. If not, checks if the Curtis has been turned on.
   If not, checks Voltage. If high enough, enables Curtis. Once on, checks to see if voltage falls below cutoff.
   If it has Curtis is disabled until reset.
   */
  if (cells != 0){ // Only turn on Curtis if we have determined Cell count, and therefore cutoff voltage.
    if(CCoff==0){
      if(CCon==0){
        if (Volt > cutoffV + 0.5){    // If Voltage is 0.5V above cutoff
          digitalWrite(CENABLE, HIGH);
          CCon++;
        }
      }


      else if (Volt <= cutoffV){
        digitalWrite(CENABLE, LOW);
        ;
        CCoff++;                          // Turns off the Curtis, and then writes the current values to the log
        logfile.print(samples);           // Sample Count
        logfile.print(",");
        logfile.print(elap);           // Elapsed Seconds since start
        logfile.print(",");
        logfile.print(elapm);           // Elapsed Minutes since start
        logfile.print(",");
        logfile.print("0");           // Curtis Off
        logfile.print(",");
        logfile.print(Volt);           // Volt
        logfile.print(",");
        logfile.print(Amp);             // Amp
        logfile.print(",");
        logfile.print(watt);           // Watt
        logfile.print(",");
        logfile.print(BDIP);           // BDI
        logfile.print(",");
        logfile.println(AmpHourTotal);           // AH
        logfile.print(",");
        logfile.println(Throttlep);           // Throttle
      }
      if (Amp > 0.5){
        digitalWrite(CENABLE, LOW);
        ;
        CCoff++;
      }
    }
  }

  // Run load for test duration
  if (testduration != 0){ //Only disable if test duration is fixed
    if (cells != 0){ // Only turn on Curtis if we have determined Cell count, and therefore cutoff voltage.
      if(CCoff==0){
        if(CCon==0){

          if (elapm < testduration){    // If elapsed is less than test duration
            digitalWrite(CENABLE, HIGH);
            CCon++;
          }
        }


        else if (elapm >= testduration){
          digitalWrite(CENABLE, LOW);
          CCoff++;
          logfile.print(samples);           // Sample Count
          logfile.print(",");
          logfile.print(elap);           // Elapsed Seconds since start
          logfile.print(",");
          logfile.print(elapm);           // Elapsed Minutes since start
          logfile.print(",");
          logfile.print("0");           // Curtis Off
          logfile.print(",");
          logfile.print(Volt);           // Volt
          logfile.print(",");
          logfile.print(Amp);             // Amp
          logfile.print(",");
          logfile.print(watt);           // Watt
          logfile.print(",");
          logfile.print(BDIP);           // BDI
          logfile.print(",");
          logfile.println(AmpHourTotal);           // AH
          logfile.print(",");
          logfile.println(Throttlep);           // Throttle
        }
        if (Amp > 0.5){
          digitalWrite(CENABLE, LOW);
          ;
          CCoff++;
        }
      }
    }
  }

  DateTime now = RTC.now();
  // LCD Display Section
  lcd.setCursor (0, 0);
  lcd.print(Volt, 2);
  lcd.print("V ");
  lcd.print(Amp, 2);  
  lcd.print("A ");
  lcd.print(Throttlep);
  lcd.print("%  ");


  lcd.setCursor(0, 1);
  if (BDIP >= 95 ){
    lcd.write(0);
  }
  else{
    if (BDIP >= 80 ){
      lcd.write(1);
    }
    else{
      if (BDIP >= 60 ){
        lcd.write(2);
      }
      else{
        if (BDIP >= 40 ){
          lcd.write(3);
        }
        else{
          if (BDIP >= 20 ){
            lcd.write(4);
          }
          else{
            lcd.write(5);
          }
        }
      }
    }
  }
  lcd.print(AmpHourTotal);
  lcd.print(" AH ");
  lcd.print(cells);
  lcd.print(" Cells");

  lcd.setCursor(0, 2);
  lcd.print(" CC ");
  if ( digitalRead( CENABLE ) == HIGH){
    lcd.print("ON ");           // Curtis On
  } 
  else {
    lcd.print("OFF ");           // Curtis Off
  }


  lcd.setCursor(0, 3);
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  lcd.print(now.second(), DEC);
  lcd.print(" ");
  lcd.print(duration);
  lcd.print(" ");



  // Print to SD card

  if (duration >= interval){
    cycle = RTC.now().unixtime();
    if (cells != 0){
      samples++;
      lcd.setCursor(13, 3);
      lcd.print(samples, DEC);
      lcd.print(" Samp ");
      //logfile.print("Count,Seconds Elapsed,Minutes Elapsed,Curtis State,Volts,Amps,Watts,BDI,AH");
      logfile.print(samples);           // Sample Count
      logfile.print(",");
      logfile.print(elap);           // Elapsed Seconds since start
      logfile.print(",");
      logfile.print(elapm);           // Elapsed Minutes since start
      logfile.print(",");
      if ( digitalRead( CENABLE ) == HIGH){
        logfile.print("1");           // Curtis On
      } 
      else {
        logfile.print("0");           // Curtis Off
      }

      logfile.print(",");
      logfile.print(Volt);           // Volt
      logfile.print(",");
      logfile.print(Amp);             // Amp
      logfile.print(",");
      logfile.print(watt);           // Watt
      logfile.print(",");
      logfile.print(BDIP);           // BDI
      logfile.print(",");
      logfile.println(AmpHourTotal);           // AH
      logfile.print(",");
      logfile.println(Throttlep);           // Throttle
      logfile.flush();

    }
  }

} // end of loop



















