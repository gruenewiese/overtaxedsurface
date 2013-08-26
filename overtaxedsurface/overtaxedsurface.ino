//includes
#include <Servo.h>
#include <CapacitiveSensor.h>

//todo: maximalgeschwindigkeit in die ineinanderskalierung der verschiedenen funktionen einbauen 
boolean aktorOn = 1; //0 to turn machines off
boolean sensorOn = 1; //0 to turn sensor off and activate testwerte()
boolean startSequence = 0; //1 moves Servos to 0, 50 and 100% at the beginning

//###############vars aktor###############
//settings
int numberOfPresets = 6;
int presetConfig[6][3] = {
  {0,1,10} //0 idle
  ,{0,0,0} //1 goodmorning slow
  ,{3,50,80}  //2 goodmorning
  ,{70,90,95} //3 cracy
  ,{90,95,100} //4 shock 
  ,{0,0,0} //5 randommove
};
int servoCalib [4][2] = {{120,25},{170,15},{15,110},{10,160}}; //start and end points of servos
//---
Servo myServo0; 
Servo myServo1; 
Servo myServo2;
Servo myServo3;
Servo myServos[] = {myServo0,myServo1,myServo2,myServo3};
long ms; //milliseconds.
long tick; //each n milliseconds a tick is generated
float pos = 1.00; //variable for movement speed, which is independent from ticking speed
long prevAktor; //this is for timing
int moveAllToArr[] = {0,0,0,0};  /*this is the array for each preset function 
form: {pos servo1, ..., ... , importance/scale from 0-100(generates automatically)}*/
float allPresets[6][5] = {{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0}};
/*bottom limit, center, upper limit, :*/

int globalImportance = 0; /* this calculates how many functions are active and the global importance
form: {how many active, global imporance}*/

int testwert = 0;
int randommove[]={0,0,0,0};
//###############end vars aktor###############


//###############vars sensor###############
//settings
CapacitiveSensor cs = CapacitiveSensor(7,6);
int aufloesung = 2; //Ausgaben des Sensors pro Sekunde
long werte_range[] = {0, 1500}; //anfangswerte, die automatisch kalibriert werden
int counter_beruehrung_max = 5;
int schockschwelle = 50;
int beruehrungsschwelle = 30;
float ruecklauf = -1;
//---
long cs_data;
int counter = 0;
long werte = 0;
long durchschnitt;
int durchschnitt_gemappt;
float durchschnitt_alterwert = 0;
float ueberforderung = 0;
long veraenderung;
boolean kalibriert = false;
float beruehrung = 0;
int counter_beruehrung = 0;
int counter_veraenderung = 0;
float beruehrung_exp = 0;
float schock = false;
int counter_lang = 0;
long durchschnitt_lang;
float ueberforderung_exp;
float ueberforderung_exp_spitze;
int veraenderung_werte;
int veraenderung_counter = 0;
int veraenderung_durchschnitt;
//---
//###############end vars sensor###############

void setup() {
  //aktor
  prevAktor = millis ();
  if (aktorOn){
    myServo0.attach(5);
    myServo1.attach(4);
    myServo2.attach(3);
    myServo3.attach(2);
  }
  //sensor
  cs.set_CS_AutocaL_Millis(15000);
  
  //allgemein
  Serial.begin(9600);
} 

void loop() {
   if (startSequence) initSequence();
  //############### sensor - generiert die Variable ueberforderung ###############
  //Frage Sensordaten ab
  if (sensorOn) {
    cs_data =  cs.capacitiveSensor(30);
    if (cs_data > werte_range[1]) cs.reset_CS_AutoCal();
    werte += cs_data; //hochzählen für Berechnung von Durchschnittswerten
    counter ++;
    //--
  }
  //############### ende sensor ###############

  //############### aktor ###############
  ms = millis(); //1000ms = 1s
  if (ms-prevAktor >= 40){ //tick 25 times a second
    tick++; //necessarry for the ticking
    
    if (sensorOn) {
      if (tick%2 == 0) sensor();
      //if (tick%2 == 0) debug_sensor();
      //reset
      werte = 0;
      counter = 0;
      //--
    } else {
      testwerte();
    }
    calculateImportance();
    //Serial.println (ueberforderung);
    presets(tick);
    calculateMovAlltoArr();
    //zeroall();
    moveall();
    debugAktor();
    prevAktor = millis(); //ticking
  }
  //############### ende aktor###############
}

//############### aktor functions ###############
void initSequence(){
  int t = 2000;
  delay(t);
  for (int i=0; i<4; i++){
    moveAllTo(0);
    delay(t);
    moveTo(i,100);
    delay(t);
  }
  startSequence = 0;
  moveAllTo(0);
}

void calculateImportance(){
  if (ueberforderung==100) ueberforderung=99;
  if (!ueberforderung) ueberforderung=1;
  for (int i=0; i<numberOfPresets; i++){
    if ((ueberforderung >= presetConfig[i][0]) && (ueberforderung <= presetConfig[i][1])){
      //ganzer abstand: presetconfig[0][1]-presetconfig[0][0]
      //abstand von unten: presetconfig[0][1]-ueberforderung
      //abstand von unten / ganzer abstand * 100:
      allPresets[i][4] = 100.00*(ueberforderung-presetConfig[i][0])/(presetConfig[i][1]-presetConfig[i][0]);
      //if (allpresets[i][4]<4)allpresets[i][4]=0;
    }
    else if((ueberforderung >= presetConfig[i][1]) && (ueberforderung <= presetConfig[i][2])){
      //ganzer abstand: presetconfig[0][2]-presetconfig[0][1]
      //abstand von oben: presetconfig[0][2]-ueberforderung
      //abstand von oben / ganzer abstand * 100:
      allPresets[i][4] = 100.00*(presetConfig[i][2]-ueberforderung)/(presetConfig[i][2]-presetConfig[i][1]);
      //if (allpresets[i][4]<4)allpresets[i][4]=0;
    }
    else {
      allPresets[i][4]=0;
    }
  }
  globalImportance=0;
  for (int j=0; j<numberOfPresets; j++)globalImportance+=allPresets[j][4];

}

/*start preset functions */
void presets (long tick){
  pos = tick*0.30; //->20.000*2*PI; //1 360 rotation per second
  if (allPresets[0][4]){//###################### idle
    pos = pos/14;///random(1,5);
    if ((tick%random(5,25))==0){
      allPresets[0][random(0,4)] = random(0,8);
      //Serial.print("im idle!");
    }
  }
  if (allPresets[1][4]){//###################### goodmorningslow
    pos = pos/5;///random(1,5);
    for (int i=0; i<4; i++) allPresets[1][i] = 25*sin(pos+PI*i/2)+40;
  }
  if (allPresets[2][4]){//###################### goodmorningfaster
    pos = pos/7;///random(1,5);
    allPresets[2][0] = sin(pos)*25+40;
    allPresets[2][1] = sin(pos+PI*0.5)*25+40;
    allPresets[2][2] = sin(pos+PI)*25+40;
    allPresets[2][3] = sin(pos+PI*1.5)*25+40;
  }
  if (allPresets[3][4]){//###################### gocracy
    pos = pos/6;
    for (int i=0; i<4; i++) allPresets[3][i] = random(0, 80);
  }
  if (allPresets[4][4]){//###################### shock
    pos = pos*2;
    for (int i=0; i<4; i++) allPresets[4][i] = random (75, 85);
  }
  if (allPresets[5][4]){//###################### randommove
    pos = pos*10;
    //make it more likely that if you substracted something before, that you also do it after
    for (int i=0; i<4; i++) {
      if (!random(0,14)) randommove[i]=random(-1,1);
      if (allPresets[5][i]<15) randommove[i] = random(1,3);
      if (allPresets[5][i]>83) randommove[i] = random(-4,-1);
      allPresets[5][i] = allPresets[5][i]+randommove[i];
    }
  }
}

void calculateMovAlltoArr(){
  //reset array
  moveAllToArr [0] = 0; 
  moveAllToArr [1] = 0; 
  moveAllToArr [2] = 0; 
  moveAllToArr [3] = 0;
  for (int m=0; m<numberOfPresets; m++) {
    for (int n=0; n<4; n++){
      if (allPresets[m][4]!=0) {
        moveAllToArr[n] = moveAllToArr[n] + allPresets[m][n]*(allPresets[m][4]/globalImportance);
      }
    }
  }
};

/*some helping functions*/
//moveTo (servoNR[0-3], 0%-100%)
void moveTo (int s, int w) {
  int b = servoCalib[s][0];
  int e = servoCalib[s][1];
  myServos[s].write(map(w, 0, 100, b, e));
  //myServos[s].write(pow((map(w, 0, 100, b, e))/10.00, 2)*0.33+map(w, 0, 100, b, e)*0.66);
}
void moveAllTo(int p){
  for (int i=0; i<4; i++) moveTo(i, p);
}
void moveall() {
  for (int i=0; i<4; i++) moveTo(i, moveAllToArr[i]);
}
void zeroall (){
  for (int i=0; i<4; i++) moveTo(i, 0);
}
void showarr(int n){
  for (int k=0; k<5; k++) {
    Serial.print(allPresets[n][k]); 
    Serial.print("\t");
  }
}
void showmoveall(int n){
  for (int l=0; l<4; l++) {
    Serial.print(moveAllToArr[l]); 
    Serial.print("\t");
  }
  Serial.println();
}
void testonepreset(int m, int tick){
  for (int i=0; i<numberOfPresets; i++)allPresets[i][4]=0;
  allPresets[m][4] = 99;
  for (int n=0; n<4; n++){
    if (allPresets[m][4]!=0) {
      moveAllToArr[n] = allPresets[m][n];
    }
  }
}
void debugAktor () {
  Serial.print(ueberforderung);
  Serial.print(": ");
  for (int m=0; m<numberOfPresets; m++){
    Serial.print(allPresets[m][4]); //importance of each Preset
    Serial.print(" ");
  }
  Serial.println();
}

void testwerte (){
 //überschreibt die überforderung und liefert werte von 0 bis 100 auf und abtsteigend
 ueberforderung = tick%20;
 if (ueberforderung <= 100) testwert++;
 if (ueberforderung > 100) testwert--;
 ueberforderung = testwert;
}

//############### end aktor functions ###############


//############### sensor functions ###############
void sensor() {
  durchschnitt = werte / counter;
  //Kalibrierung auf 0
  aufnullkalibrieren();
  //mappe durchschnitt auf 0 bis 100
  durchschnitt_gemappt = map(durchschnitt, werte_range[0], werte_range[1], 0, 100);
  //Kalibrierung auf 100
  //aufhundertkalibrieren(cs_data);

  if (kalibriert) {

    //Wert Veränderung
    veraenderung = durchschnitt_gemappt - durchschnitt_alterwert;
    if (veraenderung == 0) counter_veraenderung++;
    else counter_veraenderung = 0;

    //Wert Berührung
    if (durchschnitt_gemappt > beruehrungsschwelle && beruehrung_exp <= 100) {
      beruehrung++;
      beruehrung_exp = pow(beruehrung/2.00, 2);
      if (beruehrung_exp > 100) beruehrung_exp = 101;
      counter_beruehrung = 0;
    } 
    else if (beruehrung > 0 || beruehrung_exp > 100) {
      if (veraenderung > -2 && veraenderung < 2) counter_beruehrung++;
      if (counter_beruehrung >= counter_beruehrung_max) {
        beruehrung--;
        beruehrung_exp = pow(beruehrung/2.00, 2);

      }
    } 
    else {
      counter_beruehrung = 0;
    }

    //Wert Überforderung
    if (veraenderung >= schockschwelle) schock = true;

    if (ueberforderung >= 0 && ueberforderung < 101) {
      if (durchschnitt_gemappt > 5) {
        if (veraenderung+ueberforderung > 0) {
          if (beruehrung_exp == 0) ueberforderung += veraenderung/2;
          else ueberforderung += (veraenderung+beruehrung_exp)/3;
        }
        ueberforderung_exp = 0;
        ueberforderung_exp_spitze = ueberforderung;
      } 
      else if (ueberforderung != 0) {
        ueberforderung_exp++;
        ueberforderung = ruecklauf*ueberforderung_exp+ueberforderung_exp_spitze*1.000;
      }
      if (ueberforderung < 0) ueberforderung = 0;
      if (ueberforderung >= 100) ueberforderung = 99; //verhindert werte über 100
    }
    ueberforderung = round(ueberforderung);
    if (schock) {
      ueberforderung = 99;
      if ((tick%11) == 0) schock = false;
    }

  } // Ende if kalibriert

  //reset
  durchschnitt_alterwert = durchschnitt_gemappt;
}


void aufnullkalibrieren() {
  counter_lang++;
  veraenderung_counter++;
  veraenderung_werte += veraenderung;
  if (kalibriert) {
    durchschnitt_lang += durchschnitt;
    veraenderung_durchschnitt = veraenderung_werte / veraenderung_counter;
    if (
    durchschnitt_gemappt < -1
      || (durchschnitt_gemappt > 1 && durchschnitt_gemappt < 10 && counter_veraenderung >= 10)
      || (durchschnitt_gemappt >= 10 && veraenderung_counter > 20 &&  veraenderung_durchschnitt > -2 && veraenderung_durchschnitt < 2)
      )
    {
      werte_range[0] = durchschnitt_lang / counter_lang;
      durchschnitt_lang = 0;
      counter_lang = 0;
      veraenderung_counter = 0; //---
      veraenderung_werte = 0; //---
      Serial.print("#### kalibriert bei ");
      Serial.print(werte_range[0]);
      Serial.println(" ####");

    }
  } 
  else if (counter_lang > 5) {
    durchschnitt_lang += durchschnitt;
    if (counter_lang > 20) {
      werte_range[0] = durchschnitt_lang / (counter_lang-5);      
      durchschnitt_lang = 0;
      counter_lang = 0;
      kalibriert = true;
      Serial.print("#### Erste Kalibrierung bei ");
      Serial.print(werte_range[0]);
      Serial.println(" ####");
    }
  }
}

void aufhundertkalibrieren(long data) {
  if (kalibriert) {
    if (durchschnitt_gemappt > 100) {
      werte_range[1] = data;
      Serial.print("#### Obergrenze kalibriert bei ");
      Serial.print(werte_range[1]);
      Serial.println(" ####");
    }
  }
}

void debug_sensor() {
  Serial.print(cs_data);
  Serial.print("\t");
  Serial.print(durchschnitt);
  Serial.print("\t");
  Serial.print(durchschnitt_gemappt);
  Serial.print("\t");
  Serial.print(veraenderung);
  Serial.print("\t");
  Serial.print(beruehrung_exp);
  Serial.print("\t");
  Serial.print(ueberforderung);
  //  Serial.print("\t");
  //  Serial.print(ueberforderung_exp);
  //  Serial.print("\t");
  //  Serial.print(ueberforderung_exp_spitze);
  Serial.println("");
}
