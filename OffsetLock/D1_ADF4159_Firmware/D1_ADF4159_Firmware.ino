/****************************************************************************************
 *  *Note: MAY 31 2023, increase ramp array length to accomidate more ramp segments
 *      Also add ramp number check to make sure it doesn't exceed internal array size
 *      Max number of ramp segments is 512 for each channel
 * This is the firmware for the teensy 4.0 that takes command from serial interface 
 * and program the ADF4159 IC. 
 * The teensy micrcontroller also generates the necessary data for ramping the frequency.
 *  
 * The control command takes the following form. 
 * (ch, start freq, stop freq, ramp step number, ramp length)
 * 
 * range for input data:
 * Ch:0
 * Start freq: 500-8000(MHz)  If start freq = stop freq then the ramp is ignored.
 * Stop freq:500-8000(MHz)    If start freq = stop freq then the ramp is ignored.
 * Ramp step number: <2^32     Must be a positive integer
 * Ramp length: in ms.         Minimum step time is limited to 20us.
 *                             i.e. step length/step munber >20us
 *                             
 * Ch:1
 * Start freq: 6650-7500(MHz)  If start freq = stop freq then the ramp is ignored.
 * Stop freq:6650-7500(MHz)    If start freq = stop freq then the ramp is ignored.
 * Ramp step number: <2^32     Must be a positive integer
 * Ramp length: in ms.         Minimum step time is limited to 20us.
 *                             i.e. step length/step munber >20us
 *              
 * Anything out of range, the control will return an error.
 * Nothing will be updated. 
 * If the command is entered sucessfully, the control will give the echo of the command.
 * During power on, the controller programs the ADF4159 to the last defult frequency.
 ***************************************************************************************/

// #define DEBUG_

//Including necessary libraries 
#include <SPI.h>
#include <EEPROMex.h>

//Variable declaration
const int MAX_RAMP_NUM = 512;
const int cs0 = 14;  //slave select pin for ch0
const int cs1 = 15;  //slave select pin for ch1

const int Trig0 = 7;  //ch0 ramp trigger pin
const int Trig1 = 8;  //ch1 ramp trigger pin

const uint32_t fPFD0 = 20000;  //PFD frequency in kHz, for Rcounter=5
const uint32_t fPFD1 = 100000;  //PFD frequency in kHz, for Rcounter=1

int32_t ramp0[MAX_RAMP_NUM][6];  //ramp parameters for ch0 (Fs,Fe,R#,Rleng,dF,dt)
int32_t ramp1[MAX_RAMP_NUM][6];  //ramp parameters for ch1 (Fs,Fe,R#,Rleng,dF,dt)

const uint32_t address = 0;  //eeprom address for ch0,1 default frequencies

elapsedMicros rampDelay0;  //ramp delay timer0
elapsedMicros rampDelay1;  //ramp delay timer1

uint32_t dt0;  //ch0 ramp step delay interval
uint32_t dt1;  //ch1 ramp step delay interval

uint32_t stepCount0;  //ramp step counter for ch0
uint32_t stepCount1;  //ramp step counter for ch1

uint32_t rampflg0;  //ramp start/stop flag
uint32_t rampflg1;  //ramp start/stop flag

uint32_t rampCounter0;  //number of uploaded ramp in ch0
uint32_t rampCounter1;  //number of uploaded ramp in ch1

const char startMarker = '(';  //start marker for each set of data
const char endMarker = ')';  //end marker for each set of data
const char endendMarker = 'e';  //end marker for the whole frame of data
const char trigMarker = 't'; //software trigger marker type "te" to software trigger

void updatePFD(uint32_t FTW, int LE);
uint32_t calcFTW(uint32_t freq, uint32_t fPFD);
void processData();
void pfdInit();
void setRamp0();
void setRamp1();
bool dataCheck(uint32_t par[5]);
bool dataCheck1(uint32_t par[5]);


//initialization of hardware
void setup() {
  // Initializing USB serial to 12Mbit/sec. Teensy ignores the 9600 baud rate.
  Serial.begin(9600);
  //Initializing SPI to 1Mbit/sec
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  pinMode(cs0,OUTPUT);   //set cs pin as output
  pinMode(cs1,OUTPUT);
  digitalWrite(cs0,LOW);
  digitalWrite(cs1,LOW);
  //Initialize trigger pins and attach interrupts
  pinMode(Trig0,INPUT);
  pinMode(Trig1,INPUT);
  attachInterrupt(digitalPinToInterrupt(Trig0),setRamp0,RISING);
  attachInterrupt(digitalPinToInterrupt(Trig1),setRamp1,RISING);
  EEPROM.updateLong(address,624000);
  EEPROM.updateLong(address+5,6835000/2);
  //Initialize ADF4159 to default frequency from EEPROM
  delay(1000);
  pfdInit();
}

//main function
void loop() {
  // put your main code here, to run repeatedly:
  //Check for serial data
  if (Serial.available()){
    processData();
  }
  if (rampflg0%2 && rampflg0/2 < rampCounter0){
    if (stepCount0 == 0){
      dt0 = ramp0[rampflg0/2][5];
      updatePFD(calcFTW(ramp0[rampflg0/2][0], fPFD0),cs0);
      stepCount0++;
      #ifdef DEBUG_
      char buff[128];
      sprintf(buff, "ramping ch0: ramp step (%d/%d) in ramp# (%d/%d)", stepCount0, ramp0[rampflg0/2][2]+1, rampflg0/2+1, rampCounter0);
      Serial.println(buff);
      #endif
    }
    if (rampDelay0 >= dt0 && stepCount0 <= ramp0[rampflg0/2][2]){
      rampDelay0 = rampDelay0 - dt0;
      //update ch0 pfd function goes here
      updatePFD(calcFTW(ramp0[rampflg0/2][0]+(ramp0[rampflg0/2][4]*stepCount0), fPFD0),cs0);
      stepCount0++;
      #ifdef DEBUG_
      char buff[128];
      sprintf(buff, "ramping ch0: ramp step (%d/%d) in ramp# (%d/%d)", stepCount0, ramp0[rampflg0/2][2]+1, rampflg0/2+1, rampCounter0);
      Serial.println(buff);
      #endif
    }
    else if (stepCount0 > ramp0[rampflg0/2][2]){
      stepCount0 = 0;
      rampflg0++;
    }
  }
  if (rampflg1%2 && rampflg1/2 < rampCounter1){
    if (stepCount1 == 0){
      dt1 = ramp1[rampflg1/2][5];
      updatePFD(calcFTW(ramp1[rampflg1/2][0], fPFD1),cs1);
      stepCount1++;
      #ifdef DEBUG_
      char buff[128];
      sprintf(buff, "ramping ch1: ramp step (%d/%d) in ramp# (%d/%d)", stepCount1, ramp1[rampflg1/2][2]+1, rampflg1/2+1, rampCounter1);
      Serial.println(buff);
      #endif
    }
    if (rampDelay1 >= dt1 && stepCount1 <= ramp1[rampflg1/2][2]){
      rampDelay1 = rampDelay1 - dt1;
      //update ch1 pfd function goes here
      updatePFD(calcFTW(ramp1[rampflg1/2][0]+(ramp1[rampflg1/2][4]*stepCount1), fPFD1),cs1);
      stepCount1++;
      #ifdef DEBUG_
      char buff[128];
      sprintf(buff, "ramping ch1: ramp step (%d/%d) in ramp# (%d/%d)", stepCount1, ramp1[rampflg1/2][2]+1, rampflg1/2+1, rampCounter1);
      Serial.println(buff);
      #endif
    }
    else if (stepCount1 > ramp1[rampflg1/2][2]){
      stepCount1 = 0;
      rampflg1++;
    }
  }
}

//Initialize ADF4159 board and load default frequencies from EEPROM
//The ADF4159 is initialized to the following 
//ch0: R=5,cp=2.5mA, Prescaler = 4/5: NMIN = 23, at 500MHz, INT=25
//ch1: R=1, cp=2.1875mA, bleed=101-243uA
void pfdInit(){
  char INI[28] = {0,0,0,7,
                  0,0,0,6,
                  0,0x80,0,6,
                  0,0,0,5,
                  0,0x80,0,5,
                  0,0x18,0x01,4,
                  0,0x18,1,0x44};
  char ch0R32[8] = {0x01,0x83,0x00,0x83,0x07,0x02,0x80,0x0A};
  char ch1R32[8] = {0x01,0x63,0x00,0x83,0x06,0x00,0x80,0x0A};
  //ch0 R4,R5,R6,R7 Initialization
  for(int i=0;i<7;i++){
    for(int j=0;j<4;j++){
      SPI.transfer(INI[4*i+j]);
    }
    digitalWrite(cs0,HIGH);
    digitalWrite(cs0,LOW);
  }
  //write ch0 R3,R2
  for(int i=0;i<2;i++){
    for(int j=0; j<4;j++){
      SPI.transfer(ch0R32[4*i+j]);
    }
    digitalWrite(cs0,HIGH);
    digitalWrite(cs0,LOW);
  }
  //write ch0 R1,R0
  updatePFD(calcFTW(EEPROM.readLong(address), fPFD0),cs0);
  //ch1 R7,R6,R5,R4 Initialization
  for(int i=0;i<7;i++){
    for(int j=0;j<4;j++){
      SPI.transfer(INI[4*i+j]);
    }
    digitalWrite(cs1,HIGH);
    digitalWrite(cs1,LOW);
  }
  //write ch1 R3,R2
  for(int i=0;i<2;i++){
    for(int j=0; j<4;j++){
      SPI.transfer(ch1R32[4*i+j]);
    }
    digitalWrite(cs1,HIGH);
    digitalWrite(cs1,LOW);
  }
  //write ch1 R1,R0
  //Initialize CH1 to 6.835 GHz due to PFD frequency being 100 MHz and feedign back to Fout/2
  //The programmed frequency needs to be fout/2
  //updatePFD(calcFTW(EEPROM.readLong(address+5)),cs1);
  updatePFD(calcFTW(6835000/2, fPFD1),cs1);
}

//Calculate the FTW according to the input frequency in kHz
uint32_t calcFTW(uint32_t freq, uint32_t fPFD){
  uint32_t N;
  float Frac;
  uint32_t FTW;
  N = freq/fPFD;
  Frac = (float)(freq%fPFD)/fPFD*0x2000000; //2**25//33554432;
  FTW = (N << 25)+ (uint32_t)Frac;
  return FTW;
}

//send data to ADF4159, FTW is in the format of (N7..N1,F25..F1)
void updatePFD(uint32_t FTW, int LE){
  char R0[4];
  char R1[4];
  
  R0[0] = 0x00;
  R0[1] = (uint32_t)((FTW >> 26) & 0x3F);
  R0[2] = (uint32_t)((FTW >> 18) & 0xFF);
  R0[3] = (uint32_t)((FTW >> 10) & 0xF8);

  R1[0] = (uint32_t)((FTW >> 9) & 0x0F);
  R1[1] = (uint32_t)((FTW >> 1) & 0xFF);
  R1[2] = (uint32_t)((FTW << 7) & 0x80);
  R1[3] = 0x01;

  //write to R1 first
  for (int n=0; n<4; n++){
    SPI.transfer(R1[n]);
  }
  digitalWrite(LE,HIGH);
  digitalWrite(LE,LOW);
  //write to R0
  for (int n=0; n<4; n++){
    SPI.transfer(R0[n]);
  }
  digitalWrite(LE,HIGH);
  digitalWrite(LE,LOW);
}

//process incoming serial data and check if data are valid and write default into EEPROM
//test(0,1300,1300,1,1)(1,5100,5100,1,1)(1,5100,5150,10,1000)(1,5150,5100,10,1000)(0,1300,1350,10,1000)(0,1350,1300,10,1000)e
//test2 (0,1340,1340,1,1)(1,6700,6700,1,1)e
//(0,1340,1340,1,1)(1,6650,7500,25,5000)(1,7500,6700,25,5000)e
//(0,1340,1500,1,1)(0,1500,1200,25,1000)(1,6650,7500,25,5000)(1,7500,6700,25,5000)e
//(0,1150,1150,1,1)(1,1175,1175,1,1)e
//test3 (0,1340,1340,1,1)(0,1340,1300,1000,2000)(0,1300,1340,1000,2000)e
void processData(){
  float para[5];
  uint32_t paraint0[MAX_RAMP_NUM][5];
  uint32_t paraint1[MAX_RAMP_NUM][5];
  uint32_t rampCount0 = 0;
  uint32_t rampCount1 = 0;

  char rc = 0;
  bool error = false;   //error flag for input parameter
  bool triggered = false; //software triggered?
  elapsedMillis serialTimeout;  //timeout for serial receive in case of faliur
  
  //process all serial data until the end marker is received
  serialTimeout = 0;
  while (rc != endendMarker && serialTimeout != 10000){
    if (Serial.available()){
      rc = Serial.read();
    }
    if (rc == startMarker){
      //Take the first 5 float from serial command
      for (int i=0; i<5; i++){
        para[i] = Serial.parseFloat();
      }
      //convert float into int and transfer to parameter buffer also check the data validity
      if ((int)para[0] == 0){
        paraint0[rampCount0][0] = para[0];    //ch
        paraint0[rampCount0][1] = para[1]*1000;    //freq start in kHz
        paraint0[rampCount0][2] = para[2]*1000;    //freq stop in kHz
        paraint0[rampCount0][3] = para[3];    //step number
        paraint0[rampCount0][4] = para[4]*1000;    //step length time in us
        error |= dataCheck(paraint0[rampCount0]);
        rampCount0++;
      }
      else if ((int)para[0] == 1){
        paraint1[rampCount1][0] = para[0];    //ch
        paraint1[rampCount1][1] = para[1]*1000/2;    //freq start in kHz, /2 because the VCO is divided by 2 feeded to PLL
        paraint1[rampCount1][2] = para[2]*1000/2;    //freq stop in kHz
        paraint1[rampCount1][3] = para[3];    //step number
        paraint1[rampCount1][4] = para[4]*1000;    //step length time in us
        error |= dataCheck1(paraint1[rampCount1]);
        rampCount1++;
      }
    }
    else if (rc == trigMarker){
      triggered = true;
    }
  }
  //check if there are errors in the data or the serial communication timeout is reached
  if (error == false && serialTimeout < 10000 && triggered == false && rampCount0 < MAX_RAMP_NUM && rampCount1 < MAX_RAMP_NUM ){
    //transfer local buffered parameter to global parameter in the main program
    for (int i=0; i<rampCount0; i++){
      for(int j=0; j<4; j++){
        ramp0[i][j] = paraint0[i][j+1];
      }
      ramp0[i][4] = (ramp0[i][1]-ramp0[i][0])/ramp0[i][2];
      ramp0[i][5] = ramp0[i][3]/ramp0[i][2];
    }
    for (int i=0; i<rampCount1; i++){
      for(int j=0; j<4; j++){
        ramp1[i][j] = paraint1[i][j+1];
      }
      ramp1[i][4] = (ramp1[i][1]-ramp1[i][0])/ramp1[i][2];
      ramp1[i][5] = ramp1[i][3]/ramp1[i][2];
    }
    rampCounter0 = rampCount0;
    rampCounter1 = rampCount1;
    rampflg0 = 0;
    rampflg1 = 0;
    // EEPROM.updateLong(address,ramp0[0][0]);
    // EEPROM.updateLong(address+5,ramp1[0][0]);
    //write ch0 R1,R0
    updatePFD(calcFTW(EEPROM.readLong(address), fPFD0),cs0);
    //write ch1 R1,R0
    updatePFD(calcFTW(EEPROM.readLong(address+5), fPFD1),cs1);
    Serial.print("Parameter programmed in :");
    Serial.print(serialTimeout);
    Serial.println(" ms");
    // Serial.print("f");
  }
  else if (triggered == true){
    //trigger ch0 ramp
    rampDelay0 = 0;
    rampflg0++;
    stepCount0 = 0;
    //trigger ch1 ramp
    rampDelay1 = 0;
    rampflg1++;
    stepCount1 = 0;
    Serial.println("Software triggered!!");
    // Serial.print("f");
    triggered = false;
  }
  else{
    Serial.println("Error in paramter!! Nothing changed.");
    // Serial.print("f");
  }
  while (Serial.available()){
    Serial.read();    
  }
}

//check if input data are in range and valid for ch0
bool dataCheck(uint32_t par[5]){
  bool errorflg = true;
  if (par[0] == 0 || par[0] == 1 ){
    if (par[1] >= 500000 && par[1] <= 8000000 && par[2] >= 500000 && par[2] <= 8000000){ /*check if the freq is within range of 500MHz to 8GHz*/
      if (par[3] > 0 && par[4]/par[3] >= 20){
        errorflg = false;
      }
      else{
        errorflg = true;
      }
    }
    else{
      errorflg = true;
    }
  }
  else{
    errorflg = true;
  }
  return errorflg;
}

//check if input data are in range and valid for channel 1
bool dataCheck1(uint32_t par[5]){
  bool errorflg = true;
  if (par[0] == 0 || par[0] == 1 ){
    if (par[1] >= 6650*1000/2 && par[1] <= 7500*1000/2 && par[2] >= 6650*1000/2 && par[2] <= 7500*1000/2){
      if (par[3] > 0 && par[4]/par[3] >= 20){
        errorflg = false;
      }
      else{
        errorflg = true;
      }
    }
    else{
      errorflg = true;
    }
  }
  else{
    errorflg = true;
  }
  return errorflg;
}

//ISR for ch0 ramp when the trigger is detected
void setRamp0(){
  rampDelay0 = 0;
  rampflg0++;
  stepCount0 = 0;
  #ifdef DEBUG_
  Serial.println("Trigger for ch0 triggered");
  #endif
}

//ISR for ch1 ramp when the trigger is detected
void setRamp1(){
  rampDelay1 = 0;
  rampflg1++;
  stepCount1 = 0;
  #ifdef DEBUG_
  Serial.println("Trigger for ch1 triggered");
  #endif
}
