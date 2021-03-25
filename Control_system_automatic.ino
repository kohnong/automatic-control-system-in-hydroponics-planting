#define SensorPin A0 //pH meter Analog output to Arduino Analog Input 0
unsigned long int avgValue; //Store the average value of the sensor feedback
float b;
int buf[10],temp;
#include "DFRobot_EC.h"
#include <EEPROM.h>
#define EC_PIN A1
float voltage,ecValue,temperature = 25;
DFRobot_EC ec;
#define PUMPWATER1 6// water
#define PUMPWATER2 7// drain water
#define PUMPWATER3 8// fertilizer B
#define PUMPWATER4 9// fertilizer A
#define PUMPWATER5 10// ph Down
#define PUMPWATER6 11// ph Up
#define PUMPWATER7 12// dosing in
#define PUMPWATER8 13// dosing ext
#include <SoftwareSerial.h>
#define RX 2 //10
#define TX 3 //11 // near VCC 3.3 V Green wire
String AP = "Kohnong 2G!"; // Name AP
String PASS = "0850315416K"; // Pwd
String API = "0QWTGEAVOIXWYA0O"; // API of Channel
String HOST = "api.thingspeak.com";
String PORT = "80";
String f1 = "field1";
String f2 = "field2";
int countTrueCommand;
int countTimeCommand;
boolean found = false;
//int valSensor = 1;
SoftwareSerial esp8266(RX,TX);
unsigned long previousMillis = 0; // will store last time LED was updated
const long interval = 1200000; // every 20 minutes or 1200 seconds
void setup() {
Serial.begin(9600);
pins_init();
ec.begin();
esp8266.begin(115200);
}
void loop() {
unsigned long currentMillis = millis(); // time count ++
if (currentMillis - previousMillis >= interval) {
previousMillis = currentMillis;
//----------------------------------------------------------------------------------------------
water_abs_pH();
delay(5000);
//----------------------------------------------------------------------------------------------
// find pH values
for(int i=0;i<10;i++) //Get 10 sample value from the sensor for smooth the value
{
buf[i]=analogRead(SensorPin);
delay(10);
}
for(int i=0;i<9;i++) //sort the analog from small to large
{
for(int j=i+1;j<10;j++)
{
if(buf[i]>buf[j])
{
temp=buf[i];
buf[i]=buf[j];
buf[j]=temp;
}
}
}
avgValue=0;
for(int i=2;i<8;i++) //take the average value of 6 center sample
avgValue+=buf[i];
float phValue=(float)avgValue*5.0/1024/6; //convert the analog into millivolt
phValue=3.5*phValue+0.5; //convert the millivolt into pH value
Serial.print(" pH: ");
Serial.print(phValue,2);
Serial.println(" ");
// digitalWrite(13, HIGH);
// delay(2000);
// digitalWrite(13, LOW);
/* end pH value detect */
//----------------------------------------------------------------------------------------------
water_abs_EC();
delay(5000);
//----------------------------------------------------------------------------------------------
//find ec values
static unsigned long timepoint = millis();
if(millis()-timepoint>1000U) //time interval: 1s
{
timepoint = millis();
voltage = analogRead(EC_PIN)/1024.0*5000; // read the voltage
//temperature = readTemperature(); // read your temperature sensor to execute
temperature compensation
ecValue = ec.readEC(voltage,temperature); // convert voltage to EC with temperature
compensation
}
float RecValue = ecValue*1000;
// Serial.print("temperature:");
// Serial.print(temperature,1);
// Serial.print("^C EC:");
Serial.print(" EC: ");
Serial.print(RecValue);
Serial.println(" microsiemens");
/* end of find ec value */
/* send data to cloud */
sendCommand("AT",5,"OK");
sendCommand("AT+CWMODE=1",5,"OK");
sendCommand("AT+CWJAP=\""+ AP +"\",\""+ PASS +"\"",20,"OK");
String getData = "GET /update?api_key="+ API +"&"+ f1 +"="+ String(RecValue) +"&"+ f2
+"="+ String(phValue);
sendCommand("AT+CIPMUX=1",5,"OK");
sendCommand("AT+CIPSTART=0,\"TCP\",\""+ HOST +"\","+ PORT,15,"OK");
sendCommand("AT+CIPSEND=0," +String(getData.length()+4),4,">");
esp8266.println(getData);
delay(1500);
countTrueCommand++;
sendCommand("AT+CIPCLOSE=0",5,"OK");
/* end of send data to cloud */
// check value EC and pH for change crisp data to ** VL/L/M/S/VS // **(very
large/large/medium/small/very small)
// Check ec Mode ----- //// --------------------
String ecMode;
if(RecValue>=1600){
ecMode = "VL"; // Very large Val.
}
else if((RecValue>=1500)and(RecValue<1600)){
ecMode = "L"; // Large Val.
}
else if((RecValue>=1300)and(RecValue<1500)){
ecMode = "M"; // Medium Val.
}
else if((RecValue>=1200)and(RecValue<1300)){
ecMode = "S"; // Small Val.
}
else if((RecValue>=500)and(RecValue<1200)){
ecMode = "VS"; // Very small Val.
}
else {
ecMode = "ER"; // Error Val.
}
// --- Check pH Mode /// --------------------
String pHMode;
if(phValue>=6.85){
pHMode = "VL"; // Very large Val.
}
else if((phValue>=6.35)and(phValue<6.85)){
pHMode = "L"; // Large Val.
}
else if((phValue>5.75)and(phValue<6.35)){
pHMode = "M"; // Medium Val.
}
else if((phValue>=5.25)and(phValue<=5.75)){
pHMode = "S"; // Small Val.
}
else{
pHMode = "VS"; // Very small Val.
}
//--------------------------------------------------------
int fuzzy_rule;
if(ecMode=="ER"){
fuzzy_rule = 0;
}
else if((ecMode=="VL")and(pHMode=="VL")){
fuzzy_rule = 1; // ec = VL and pH = VL critical situate
}
else if((ecMode=="VL")and(pHMode=="L")){
fuzzy_rule = 2; // ec = VL and pH = L
}
else if((ecMode=="VL")and(pHMode=="M")){
fuzzy_rule = 3; // ec = VL and pH = M
}
else if((ecMode=="VL")and(pHMode=="S")){
fuzzy_rule = 4; // ec = VL and pH = S
}
else if((ecMode=="VL")and(pHMode=="VS")){
fuzzy_rule = 5; // ec = VL and pH = VS critical situate
}
// --------------------end state 5 of 1 ///////////////////////////////////////////////////////////
else if((ecMode=="L")and(pHMode=="VL")){
fuzzy_rule = 6; // ec = L and pH = VL
}
else if((ecMode=="L")and(pHMode=="L")){
fuzzy_rule = 7; // ec = L and pH = L
}
else if((ecMode=="L")and(pHMode=="M")){
fuzzy_rule = 8; // ec = L and pH = M
}
else if((ecMode=="L")and(pHMode=="S")){
fuzzy_rule = 9; // ec = L and pH = S
}
else if((ecMode=="L")and(pHMode=="VS")){
fuzzy_rule = 10; // ec = L and pH = VS
}
// --------------------end state 5 of 2 ///////////////////////////////////////////////////////////
else if((ecMode=="M")and(pHMode=="VL")){
fuzzy_rule = 11; // ec = M and pH = VL
}
else if((ecMode=="M")and(pHMode=="L")){
fuzzy_rule = 12; // ec = M and pH = L
}
else if((ecMode=="M")and(pHMode=="M")){
fuzzy_rule = 13; // ec = M and pH = M // ----- BEST CASE ---------
*********************************************
}
else if((ecMode=="M")and(pHMode=="S")){
fuzzy_rule = 14; // ec = M and pH = S
}
else if((ecMode=="M")and(pHMode=="VS")){
fuzzy_rule = 15; // ec = M and pH = VS
}
// --------------------end state 5 of 3 ///////////////////////////////////////////////////////////
else if((ecMode=="S")and(pHMode=="VL")){
fuzzy_rule = 16; // ec = S and pH = VL
}
else if((ecMode=="S")and(pHMode=="L")){
fuzzy_rule = 17; // ec = S and pH = L
}
else if((ecMode=="S")and(pHMode=="M")){
fuzzy_rule = 18; // ec = S and pH = M
}
else if((ecMode=="S")and(pHMode=="S")){
fuzzy_rule = 19; // ec = S and pH = S
}
else if((ecMode=="S")and(pHMode=="VS")){
fuzzy_rule = 20; // ec = S and pH = VS
}
// --------------------end state 5 of 4 ///////////////////////////////////////////////////////////
else if((ecMode=="VS")and(pHMode=="VL")){
fuzzy_rule = 21; // ec = VS and pH = VL critical situate
}
else if((ecMode=="VS")and(pHMode=="L")){
fuzzy_rule = 22; // ec = VS and pH = L
}
else if((ecMode=="VS")and(pHMode=="M")){
fuzzy_rule = 23; // ec = VS and pH = M
}
else if((ecMode=="VS")and(pHMode=="S")){
fuzzy_rule = 24; // ec = VS and pH = S
}
else{
fuzzy_rule = 25; // ec = VS and pH = VS critical situate
}
// --------------------end state 5 of 5 ///////////////////////////////////////////////////////////
switch(fuzzy_rule){
case 0: // When EC = M and pH = M // Best Case
Serial.println("-- Case 0 // do nothing --");
//Serial.println("-- This is BEST CASE !! no PUMPWATER is working When EC = M and pH =
M --"); // do nothing
break;
case 1: // When EC = VL and pH = VL // Worst case 1
Serial.println("-- Case 1 --");
fill_Water_L();
fill_pHDown_L();
break;
case 2: // When EC = VL and pH = L
Serial.println("-- Case 2 --");
fill_Water_L();
fill_pHDown_S();
break;
case 3: // When EC = VL and pH = M
Serial.println("-- Case 3 --");
fill_Water_L();
break;
case 4: // When EC = VL and pH = S
Serial.println("-- Case 4 --");
fill_Water_L();
break;
case 5: // When EC = VL and pH = VS
Serial.println("-- Case 5 --");
fill_Water_L();
//Serial.println("-- PUMPWATER No.5 is working When pH = VS --");
fill_pHUp_S();
break;
// --------------------end state 5 of 1 ///////////////////////////////////////////////////////////
case 6: // When EC = L and pH = VL
Serial.println("-- Case 6 --");
fill_Water_S();
fill_pHDown_L();
break;
case 7: // When EC = L and pH = L
Serial.println("-- Case 7 --");
fill_Water_S();
fill_pHDown_S();
break;
case 8: // When EC = L and pH = M
Serial.println("-- Case 8 --");
fill_Water_S();
break;
case 9: // When EC = L and pH = S
Serial.println("-- Case 9 --");
fill_Water_S();
break;
case 10: // When EC = L and pH = VS
Serial.println("-- Case 10 --");
fill_Water_S();
fill_pHUp_S();
break;
// --------------------end state 5 of 2 /////////////////// When EC = L ///////////////////////////
case 11: // When EC = M and pH = VL
Serial.println("-- Case 11 --");
fill_pHDown_L();
break;
case 12: // When EC = M and pH = L
Serial.println("-- Case 12 --");
fill_pHDown_S();
break;
case 13: // When EC = M and pH = M // Best Case
Serial.println("-- Case 13 // Best Case --");
break;
case 14: // When EC = M and pH = S
Serial.println("-- Case 14 --");
fill_pHUp_S();
break;
case 15: // When EC = M and pH = VS
Serial.println("-- Case 15 --");
fill_pHUp_L();
break;
// --------------------end state 5 of 3 /////////////////// When EC = M ///////////////////////////
case 16: // When EC = S and pH = VL
Serial.println("-- Case 16 --");
fill_EC_S();
fill_pHDown_L();
break;
case 17: // When EC = S and pH = L
Serial.println("-- Case 17 --");
fill_EC_S();
fill_pHDown_S();
break;
case 18: // When EC = S and pH = M
Serial.println("-- Case 18 --");
fill_EC_S();
break;
case 19: // When EC = S and pH = S
Serial.println("-- Case 19 --");
fill_EC_S();
fill_pHUp_S();
break;
case 20: // When EC = S and pH = VS
Serial.println("-- Case 20 --");
fill_EC_S();
fill_pHUp_L();
break;
// --------------------end state 5 of 4 /////////////////// When EC = S ///////////////////////////
case 21: // When EC = VS and pH = VL
Serial.println("-- Case 21 --");
fill_EC_L();
fill_pHDown_L();
break;
case 22: // When EC = VS and pH = L
Serial.println("-- Case 22 --");
fill_EC_L();
fill_pHDown_S();
break;
case 23: // When EC = VS and pH = M
Serial.println("-- Case 23 --");
fill_EC_L();
break;
case 24: // When EC = VS and pH = S
Serial.println("-- Case 24 --");
fill_EC_L();
fill_pHUp_S();
break;
case 25: // When EC = VS and pH = VS
Serial.println("-- Case 25 --"); // Worst case 2
fill_EC_L();
fill_pHUp_L();
break;
// --------------------end state 5 of 5 /////////////////// When EC = VS
///////////////////////////
} // end switch
//----------------------------------------------------------------------------------------------
delay(5000);
drain_water(); // drain water back to system
//----------------------------------------------------------------------------------------------
} // time interval
} // end loop
void fill_pHUp_S()
{
digitalWrite(PUMPWATER6, LOW); // fill pH Up 2 CC
delay(1640); // x 1.22cc ps
digitalWrite(PUMPWATER6, HIGH);
}
void fill_pHUp_L()
{
digitalWrite(PUMPWATER6, LOW); // fill pH Up 4 CC
delay(3280); // x 1.22cc ps
digitalWrite(PUMPWATER6, HIGH);
}
void fill_pHDown_S()
{
digitalWrite(PUMPWATER5, LOW); // fill pH Down 2 CC
delay(1640); // x 1.22cc ps
digitalWrite(PUMPWATER5, HIGH);
}
void fill_pHDown_L()
{
digitalWrite(PUMPWATER5, LOW); // fill pH Down 4 CC
delay(3280); // x 1.22cc ps
digitalWrite(PUMPWATER5, HIGH);
}
void fill_EC_S()
{
digitalWrite(PUMPWATER3, LOW); // fill fertilizer A
digitalWrite(PUMPWATER4, LOW); // fill fertilizer B
delay(14820); // fill fertilizer AB 20 CC x 1.35cc ps
digitalWrite(PUMPWATER3, HIGH);
digitalWrite(PUMPWATER4, HIGH);
}
void fill_EC_L()
{
digitalWrite(PUMPWATER3, LOW); // fill fertilizer A
digitalWrite(PUMPWATER4, LOW); // fill fertilizer B
delay(22250); // fill fertilizer AB 30 CC x 1.35cc ps
digitalWrite(PUMPWATER3, HIGH);
digitalWrite(PUMPWATER4, HIGH);
}
void fill_Water_S()
{
digitalWrite(PUMPWATER1, LOW);
delay(5000);
digitalWrite(PUMPWATER1, HIGH);
}
void fill_Water_L()
{
digitalWrite(PUMPWATER1, LOW);
delay(10000);
digitalWrite(PUMPWATER1, HIGH);
}
void water_abs_pH()
{
digitalWrite(PUMPWATER7, LOW);
delay(80000);
digitalWrite(PUMPWATER7, HIGH);
}
void water_abs_EC()
{
digitalWrite(PUMPWATER8, LOW);
delay(85000);
digitalWrite(PUMPWATER8, HIGH);
}
void drain_water(){
digitalWrite(PUMPWATER2, LOW);
delay(7000);
digitalWrite(PUMPWATER2, HIGH);
}
void pins_init()
{
pinMode(SensorPin, INPUT);
pinMode(EC_PIN, INPUT);
pinMode(PUMPWATER1, OUTPUT);
pinMode(PUMPWATER2, OUTPUT);
pinMode(PUMPWATER3, OUTPUT);
pinMode(PUMPWATER4, OUTPUT);
pinMode(PUMPWATER5, OUTPUT);
pinMode(PUMPWATER6, OUTPUT);
pinMode(PUMPWATER7, OUTPUT);
pinMode(PUMPWATER8, OUTPUT);
digitalWrite(PUMPWATER1, HIGH);
digitalWrite(PUMPWATER2, HIGH);
digitalWrite(PUMPWATER3, HIGH);
digitalWrite(PUMPWATER4, HIGH);
digitalWrite(PUMPWATER5, HIGH);
digitalWrite(PUMPWATER6, HIGH);
digitalWrite(PUMPWATER7, HIGH);
digitalWrite(PUMPWATER8, HIGH);
}
// function for sendCommand esp8266 to cloud
void sendCommand(String command, int maxTime, char readReplay[]) {
Serial.print(countTrueCommand);
Serial.print(". at command => ");
Serial.print(command);
Serial.print(" ");
while(countTimeCommand < (maxTime*1))
{
esp8266.println(command);//at+cipsend
if(esp8266.find(readReplay))//ok
{
found = true;
break;
}
countTimeCommand++;
}
if(found == true)
{
Serial.println("SOK");
countTrueCommand++;
countTimeCommand = 0;
}
if(found == false)
{
Serial.println("Fail");
countTrueCommand = 0;
countTimeCommand = 0;
}
found = false;
}
