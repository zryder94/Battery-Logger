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
//#include <stdarg.h>
#include <Wire.h>
#include "RTClib.h"
#include <SD.h>
//#include <string.h>
//#include "Adafruit_MCP23008.h"

// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidCrystal lcd(0);

// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
RTC_DS1307 RTC;

// Input and Output pins 
int VoltPin = A2;
int AmpPin = A1;
int BDIpin = A3;
int CENABLE = 9;
int CTHROTTLE = 3;
int StartButton = 7;
int IntervalButton = 6;
int DurationButton = 5;
int TypeButton = 4;
int LoadButton = 2;
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
//int AmpF;
int AmpAverage;
int AmpHourTotal = 0;
int AmpHour = 0;
int AmpHourMilli = 0;
int Voltcount;
int AmpIn;
float watt;  // Watts
int BDIP; // Battery Discharge Indicator percentage

int CCon;
int CCoff;
int samples = 0;
int cells;
int cellscycle = 0;
float cutoffV; // Low Voltage cutoff total
int elapm = 0;       //Elapsed minutes since start
int elapmillis = 0;       //Elapsed hours since start
int elapc = 0;       //cycle time between samples
int elapi = 0;       //interval time between samples
float ampneut;      //power-on 0A reading of the current sensor IC
float cutoff; // Low Voltage cutoff per cell. 1.65 for Acid, 1.8 for Gel or AGM
int interval = 30;    //The time in seconds between log entries
int testduration = 0;    // Total test duration, in minutes
int load = 0;        // Desired discharge current
int Throttlein;  // Manual throttle input
int Throttleout = 0;    // Output throttle
int Throttlep;
int Startcount = 0;  // variable to record if the start button has been pressed

int IntervalButton0 = 0; // Send Interval button
int DurationButton0 = 0; // Send Duration button
int TypeButton0 = 0; // Send Type button
int LoadButton0 = 0; // Send Load button
int BattType = 1; // Battery Type

File logfile;

// Custom Battery level indicator
byte full[8] = {
  14,31,31,31,31,31,31};
byte five[8] = {
  14,17,31,31,31,31,31};
byte four[8] = {
  14,17,17,31,31,31,31};
byte three[8] = {
  14,17,17,17,31,31,31};
byte two[8] = {
  14,17,17,17,17,31,31};
byte empty[8] = {
  14,17,17,17,17,17,31};


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


  Wire.begin(); //Important for RTClib.h
  //mcp.begin();
  RTC.begin();
  lcd.begin(20, 4); // set up the LCD's number of columns and rows:
  Serial.begin(9600);
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

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    lcd.setCursor(10, 2);
    lcd.print("Card failed");
    //Serial.print("Card failed");
    return;
  }  

  // This bit of code is here to center the amp meter IC's output. 
  // Takes an average reading of the output, and stores that into its "center" variable
  delay (1000);
  int count = 0;
  float totalA = 0;
  while (count < 512){
    AmpIn = analogRead(AmpPin);
    totalA += AmpIn;
    count++;
  }

  ampneut = totalA / count;
  ampneut *= 5;  //Convert to 5v
  ampneut /= 1024;
  Serial.println(ampneut);

  while (Startcount == 0){  
    readbuttons();

    switch (IntervalButton0){
    case 1:
      interval = 5;
      break;
    case 2:
      interval = 10;
      break;
    case 3:
      interval = 15;
      break;
    case 4:
      interval = 30;
      break;
    case 5:
      interval = 60;
      break;
    case 6:
      interval = 150;
      break;
    case 7:
      interval = 300;
      break;
    }
    switch (DurationButton0){
    case 1:
      testduration = 0;
      break;
    case 2:
      testduration = 5;
      break;
    case 3:
      testduration = 10;
      break;
    case 4:
      testduration = 15;
      break;
    case 5:
      testduration = 30;
      break;
    case 6:
      testduration = 60;
      break;
    case 7:
      testduration = 150;
      break;
    case 8:
      testduration = 300;
      break;
    }
    switch (TypeButton0){
    case 1:
      BattType = 1;
      break;
    case 2:
      BattType = 2;
      break;
    case 3:
      BattType = 3;
      break;
    }
    switch (LoadButton0){
    case 1:
      load = 0;
      break;
    case 2:
      load = -5;
      break;
    case 3:
      load = -10;
      break;
    case 4:
      load = -15;
      break;
    case 5:
      load = -20;
      break;
    case 6:
      load = -40;
      break;
    }



    lcd.setBacklight(HIGH);
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
  //Serial.println("End of Button Loop");

  /* // see if the card is present and can be initialized:
   if (!SD.begin(chipSelect)) {
   lcd.clear(); //clears the LCD
   lcd.setCursor(10, 2);
   lcd.print("Card failed");
   //Serial.println("Card failed");
   return;
   }*/


  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connect Batteries");
  lcd.setCursor(0, 1);
  lcd.print("Now");
  //Serial.println("Connect Batteries Now");
  delay(1000);

  /*
   // see if the card is present and can be initialized:
   if (!SD.begin(chipSelect)) {
   lcd.setCursor(10, 2);
   lcd.print("Card failed");
   return;
   }*/

  DateTime now = RTC.now();
  // create a new file
  char foldername[] = "00/00";
  foldername[0] = now.month()/10 + '0'; //To get 1st digit from month()
  foldername[1] = now.month()%10 + '0'; //To get 2nd digit from month()
  foldername[3] = now.day()/10 + '0'; //To get 1st digit from day()
  foldername[4] = now.day()%10 + '0'; //To get 2nd digit from day()

  char filename[] = "00/00/00-00AGM.CSV";
  filename[0] = now.month()/10 + '0'; //To get 1st digit from month()
  filename[1] = now.month()%10 + '0'; //To get 2nd digit from month()
  filename[3] = now.day()/10 + '0'; //To get 1st digit from day()
  filename[4] = now.day()%10 + '0'; //To get 2nd digit from day()

  filename[6] = now.hour()/10 + '0'; //To get 1st digit from hour()
  filename[7] = now.hour()%10 + '0'; //To get 2nd digit from hour()
  filename[9] = now.minute()/10 + '0'; //To get 1st digit from minute()
  filename[10] = now.minute()%10 + '0'; //To get 2nd digit from minute()
  if (BattType == 1){          
    filename[11] = 'A';
    filename[12] = 'G';
    filename[13] = 'M';
  }
  else if (BattType == 2){          
    filename[11] = 'G';
    filename[12] = 'E';
    filename[13] = 'L';
  }
  else {          
    filename[11] = 'A';
    filename[12] = 'C';
    filename[13] = 'D';
  }

  SD.mkdir(foldername);
  if (! SD.exists(filename)) {
    // only open a new file if it doesn't exist
    logfile = SD.open(filename, FILE_WRITE);
    // break;  // leave the loop!
  }

  logfile.print("Count,Seconds,Minutes,Curtis State,Volts,Amps,Watts,BDI,AH,");  //Write the data header
  // Calcuate current date and time, and print to the SD card
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
  logfile.flush();
  /*
  
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
   
   logfile.flush();
   //Serial.println("Beginning of Header Printed");
   */


  if (testduration == 0){          
    logfile.print("Unlimited duration,");
    //Serial.println("Unlimited duration");
  } 

  else {
    logfile.print(testduration);
    logfile.print("min duration,");
    //Serial.print(testduration);
    //Serial.println("min duration,");
  }
  if (BattType == 1){          
    logfile.print("AGM,");
    //Serial.print("AGM,");
  }
  else if (BattType == 2){          
    logfile.print("GEL,");
    //Serial.print("GEL,");
  }
  else {          
    logfile.print("Acid,");
    //Serial.print("Acid,");
  }

  if (load == 0){          
    logfile.println("Manual Throttle,");
    //Serial.println("Manual Throttle,");
  } 
  else {
    logfile.print(load);
    logfile.println("A Current Hold,");
    //Serial.print(load);
    //Serial.println("A Current Hold,");
  }   
  logfile.flush();
  int CCon = 0;    //Curtis has not been turned on
  int CCoff = 0;   //Curtis has not been turned off
  int cells = 0;   //Have not yet detected cell count
  int Voltcount = 0;  // Have not yet measuered voltage
  // Set start and cycle timers to power on time

  lcd.clear(); //clears the LCD

  //Serial.println("Leaving Setup");
}

void loop(){

  readsensors ();

  if (Volt > 0.5){
    Voltcount ++;    //Starts adding to this number once batteries are connected
  }

  // This section automatically detects the number of battery cells attached to the machine.
  // Determining the number of cells allows the program to appropriately set the low voltage cutoff
  if(cells == 0){
    if (Voltcount > 10){  //once the batteries have been connected for a few cycles, then take a cell count.
      start = RTC.now().unixtime();
      cycle = RTC.now().unixtime();
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
    Throttleout = Throttlein / 4; //PWM is 0-256, analog in is 0-1024. Adjusting range.
  } 
  else{
    if ( digitalRead( CENABLE ) == HIGH){
      if (Amp>load){
        if(Throttleout < 256){
          //if(Amp>0.9*load){
          // Throttleout+=5;
          //}
          //else{
          Throttleout++;
          //}
        }
      }
      else if(Amp==load){
        Throttleout;
      }
      //else{
      //if(Amp<1.1*load){
      //  Throttleout-=5;
      //}
      else{
        Throttleout--;
      }
      //}
    }
  }

  analogWrite(CTHROTTLE, Throttleout);
  float Throttlep = Throttleout / 2.56;


  elapc = elapmillis;    //Sets cycle time to elapmillis from the last time around.
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
  lcd.print(Throttlep, 0);
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
      logfile.print(AmpHourTotal);           // AH
      logfile.print(",");
      logfile.println(Throttlep);           // Throttle
      logfile.flush();

    }
  }

} // end of loop

int readbuttons(){
  int StartButton1; // 1st reading of Start button
  int StartButton2; // 2nd reading of Start button
  int StartButtonState;// = LOW; // Start button state
  int IntervalButton1; // 1st reading of Interval button
  int IntervalButton2; // 2nd reading of Interval button
  int IntervalButtonState; //  = LOW Interval button state
  int DurationButton1; // 1st reading of Duration button
  int DurationButton2; // 2nd reading of Duration button
  int DurationButtonState; //  = LOW Duration button state
  int TypeButton1; // 1st reading of Type button
  int TypeButton2; // 2nd reading of Type button
  int TypeButtonState; //  = LOW Type button state
  int LoadButton1; // 1st reading of Load button
  int LoadButton2; // 2nd reading of Load button
  int LoadButtonState ; // = LOW Load button state

    StartButton1 = digitalRead(StartButton); // read Start Button and store it in StartButton1
  delay(10);                               // 10 milliseconds is a good amount of time
  StartButton2 = digitalRead(StartButton); // read the input again to check for bounces
  if (StartButton1 == StartButton2) {      // make sure we got 2 consistant readings!
    if (StartButton1 != StartButtonState) { // the button state has changed!
      if (StartButton1 == HIGH) {
        Startcount++;
        //Serial.println("StartButton");

      }
    }
  }
  IntervalButton1 = digitalRead(IntervalButton); // read Interval Button and store it in IntervalButton1
  delay(10);                               // 10 milliseconds is a good amount of time
  IntervalButton2 = digitalRead(IntervalButton); // read the input again to check for bounces
  if (IntervalButton1 == IntervalButton2) {      // make sure we got 2 consistant readings!
    if (IntervalButton1 != IntervalButtonState) { // the button state has changed!
      if (IntervalButton1 == HIGH) {
        IntervalButton0++;
        //Serial.println("IntervalButton");
        if (IntervalButton0 == 8) {
          IntervalButton0 = 1;
        }
      }
    }
  }
  DurationButton1 = digitalRead(DurationButton); // read Duration Button and store it in DurationButton1
  delay(10);                               // 10 milliseconds is a good amount of time
  DurationButton2 = digitalRead(DurationButton); // read the input again to check for bounces
  if (DurationButton1 == DurationButton2) {      // make sure we got 2 consistant readings!
    if (DurationButton1 != DurationButtonState) { // the button state has changed!
      if (DurationButton1 == HIGH) {
        DurationButton0++;
        //Serial.println("DurationButton");
        if (DurationButton0 == 9) {
          DurationButton0 = 0;
        }
      }
    }
  }
  TypeButton1 = digitalRead(TypeButton); // read Type Button and store it in TypeButton1
  delay(10);                               // 10 milliseconds is a good amount of time
  TypeButton2 = digitalRead(TypeButton); // read the input again to check for bounces
  if (TypeButton1 == TypeButton2) {      // make sure we got 2 consistant readings!
    if (TypeButton1 != TypeButtonState) { // the button state has changed!
      if (TypeButton1 == HIGH) {
        TypeButton0++;
        //Serial.println("TypeButton");
        if (TypeButton0 == 4) {
          TypeButton0 = 1;
        }
      }
    }
  }
  LoadButton1 = digitalRead(LoadButton); // read Load Button and store it in LoadButton1
  delay(10);                               // 10 milliseconds is a good amount of time
  LoadButton2 = digitalRead(LoadButton); // read the input again to check for bounces
  if (LoadButton1 == LoadButton2) {      // make sure we got 2 consistant readings!
    if (LoadButton1 != LoadButtonState) { // the button state has changed!
      if (LoadButton1 == HIGH) {
        LoadButton0++;
        //Serial.println("LoadButton");
        if (LoadButton0 == 7) {
          LoadButton0 = 1;
        }
      }
    }
  }
  return Startcount;
  return IntervalButton0;
  return DurationButton0;
  return TypeButton0;
  return LoadButton0;

}

int readsensors(){
  int count = 0;
  float totalV = 0;
  float totalA = 0;
  float totalB = 0;
  int VoltIn;
  int VoltAverage;
  int BDIaverage;
  int BDIin;


  //  Read input values 512 times, and average the values
  while (count < 512){
    VoltIn = analogRead(VoltPin);
    AmpIn = analogRead(AmpPin);
    BDIin = analogRead(BDIpin);
    totalV += VoltIn;
    totalA += AmpIn;
    totalB += BDIin;
    count++;
  }
  VoltAverage = totalV / count;
  AmpAverage = totalA / count;
  BDIaverage = totalB / count;

  Volt = VoltAverage * 5;
  Volt /= 1024;
  Volt *= 7.666947; //convert from 130.43mv/V from voltage divider 

  //Volt = VoltAverage / vcons; //convert 1024 to 38
  Amp = AmpAverage * 5;
  Amp /= 1024;
  Amp = (Amp - ampneut ) * 25; // convert from 40mv/A from ammeter sensor

  //AmpF = AmpAverage - ampneut; // 2.5v is 0A, adjusting range
  //Amp = AmpF / acons; // convert +- 512 to 50 from center
  watt = Volt * Amp;
  BDIP = BDIaverage / 10.24; // convert to Percent
  return Volt;
  return Amp;
  return watt;
  return BDIP;

}











