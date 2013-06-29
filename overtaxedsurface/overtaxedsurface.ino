//includes
#include <Servo.h>
#include <CapacitiveSensor.h>

//vars aktor
boolean on = 1; //0 to turn machines off
Servo myservo1; 
Servo myservo2; 
Servo myservo3; 
Servo myservo4;
Servo myservos[] = {
  myservo1, myservo2, myservo3, myservo4 };
long ms;
long tick;
float pos;
long somevar;
long prev_aktor; //this is for timing
int movealltoarr[] = {
  0,0,0,0};  /*this is the array for each preset function 
 form: {pos servo1, ..., ... , importance/scale from 0-100(generates automatically)}*/
float allpresets[6][5] = {
  {
    0,0,0,0,0      }
  ,{
    0,0,0,0,0      } 
  ,{
    0,0,0,0,0      } 
  ,{
    0,0,0,0,0      }
  ,{
    0,0,0,0,0      }
  ,{
    0,0,0,0,0      }
};
/*bottom limit, center, upper limit, :*/
int presetconfig[6][3] = {
  {
    0,0,15   } //0 idle
  ,{
    0,0,0  } //1 goodmorning slow
  ,{
    3,20,60    }  //2 goodmorning
  ,{
    85,90,95    } //3 cracy
  ,{
    90,95,100    } //4 shock 
  ,{
    40,70,90    } //5 randommove
};
int numberofpresets = 6;
int globalimportance = 0; /* this calculates how many functions are active and what is the global importance
 form: {how many active, global imporance}*/

//vars sensor
//settings
CapacitiveSensor cs = CapacitiveSensor(12,10);
int aufloesung = 2; //Ausgaben des Sensors pro Sekunde
long werte_range[] = {
  0, 2000}; //anfangswerte, die automatisch kalibriert werden
int counter_beruehrung_max = 5;
int schockschwelle = 50;
int beruehrungsschwelle = 30;
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
int ueberforderung_exp;
int ueberforderung_exp_spitze;
int veraenderung_werte;
int veraenderung_counter = 0;
int veraenderung_durchschnitt;
//---

int testwert = 0;
int randommove[]={0,0,0,0};

void setup() {
  //aktor
  prev_aktor = millis ();
  if (on){
    myservo1.attach(7);
    myservo2.attach(6);
    myservo3.attach(5);
    myservo4.attach(4);
  }
  //sensor
  cs.set_CS_AutocaL_Millis(15000);
  //allgemein
  Serial.begin(9600);
} 

void loop() {
  //sensor
  //Frage Sensordaten ab
  cs_data =  cs.capacitiveSensor(30);
  //hochzählen für Berechnung von Durchschnittswerten
  werte += cs_data;
  counter ++;
  //ende sensor

  //aktor
  ms = millis(); //1000ms = 1s
  if (ms-prev_aktor >= 80){ //tick 12.5times a second
    tick++;
    //if ((tick%(12.5/aufloesung))==0){
    sensor();
    debug_sensor();
    //reset
    werte = 0;
    counter = 0;
    //}
    //testwerte ();
    calculateimportance();
    //testonepreset(5, tick);
    presets(tick);
    calculatemovalltoarr();
    moveall();
    //zeroall();
    //debug_aktor();
    prev_aktor = millis();
  }
  //ende aktor
}

void calculateimportance(){
  if (ueberforderung==100) ueberforderung=99;
  if (!ueberforderung) ueberforderung=1;
  for (int i=0; i<numberofpresets; i++){
    if ((ueberforderung >= presetconfig[i][0]) && (ueberforderung <= presetconfig[i][1])){
      //ganzer abstand: presetconfig[0][1]-presetconfig[0][0]
      //abstand von unten: presetconfig[0][1]-ueberforderung
      //abstand von unten / ganzer abstand * 100:
      allpresets[i][4] = 100.00*(ueberforderung-presetconfig[i][0])/(presetconfig[i][1]-presetconfig[i][0]);
      //if (allpresets[i][4]<4)allpresets[i][4]=0;
    }
    else if((ueberforderung >= presetconfig[i][1]) && (ueberforderung <= presetconfig[i][2])){
      //ganzer abstand: presetconfig[0][2]-presetconfig[0][1]
      //abstand von oben: presetconfig[0][2]-ueberforderung
      //abstand von oben / ganzer abstand * 100:
      allpresets[i][4] = 100.00*(presetconfig[i][2]-ueberforderung)/(presetconfig[i][2]-presetconfig[i][1]);
      //if (allpresets[i][4]<4)allpresets[i][4]=0;
    }
    else {
      allpresets[i][4]=0;
    }
  }
  globalimportance=0;
  for (int j=0; j<numberofpresets; j++)globalimportance+=allpresets[j][4];

}

/*start preset functions */
//there will only be one preset funciton in the end
void presets (long tick){
  pos = tick/20.000*2*PI; //1 360 rotation per second
  if (allpresets[0][4]){//###################### idle
    pos = pos/7;///random(1,5);
    if ((tick%random(5,25))==0){
      allpresets[0][random(0,4)] = random(0,8);
      Serial.print("im idle!");
    }
  }
  if (allpresets[1][4]){//###################### goodmorningslow
    pos = pos/7;///random(1,5);
    allpresets[1][0] = sin(pos)*25+40;
    allpresets[1][1] = sin(pos+PI*0.5)*25+40;
    allpresets[1][2] = sin(pos+PI)*25+40;
    allpresets[1][3] = sin(pos+PI*1.5)*25+40;
  }
  if (allpresets[2][4]){//###################### goodmorningfaster
    pos = pos/5;///random(1,5);
    allpresets[2][0] = sin(pos)*25+40;
    allpresets[2][1] = sin(pos+PI*0.5)*25+40;
    allpresets[2][2] = sin(pos+PI)*25+40;
    allpresets[2][3] = sin(pos+PI*1.5)*25+40;
  }
  if (allpresets[3][4]){//###################### gocracy
    pos = pos/3;
    for (int i=0; i<4; i++) allpresets[3][i] = random(0, 80);
  }
  if (allpresets[4][4]){//###################### shock
    pos = pos*3;
    for (int i=0; i<4; i++) allpresets[4][i] = random (75, 85);
  }
  if (allpresets[5][4]){//###################### randommove
    pos = pos*10;
    //make it more likely that if you substracted something before, that you also do it after
    for (int i=0; i<4; i++) {
      if (!random(0,14)) randommove[i]=random(-1,1);
      if (allpresets[5][i]<15) randommove[i] = random(1,3);
      if (allpresets[5][i]>83) randommove[i] = random(-4,-1);
      allpresets[5][i] = allpresets[5][i]+randommove[i];
    }
  }

}

void calculatemovalltoarr(){
  //reset array
  movealltoarr [0] = 0; 
  movealltoarr [1] = 0; 
  movealltoarr [2] = 0; 
  movealltoarr [3] = 0;
  for (int m=0; m<numberofpresets; m++) {
    for (int n=0; n<4; n++){
      if (allpresets[m][4]!=0) {
        movealltoarr[n] = movealltoarr[n] + allpresets[m][n]*(allpresets[m][4]/globalimportance);
      }
    }
  }
};

/*some helping functions*/
//move (servoNR[0-3], 0%-100%)
void moveto (int s, int w) {
  if (s == 0 || s == 2) myservos[s].write(pow((map(w, 0, 100, 20, 170))/10.00,2)*0.33+map(w, 0, 100, 20, 170)*0.66);
  else if (s == 1 || s == 3) myservos[s].write(pow(map(w, 0, 100, 20, 120)/10.00, 2)*0.33+map(w, 0, 100, 20, 120)*0.66);
}
void moveallto(int p){
  for (int i=0; i<4; i++) moveto(i, p);
}
void moveall() {
  for (int i=0; i<4; i++) moveto(i, movealltoarr[i]);
}
void blend (){
}
void zeroall (){
  for (int i=0; i<4; i++) moveto(i, 0);
}
void showarr(int n){
  for (int k=0; k<5; k++) {
    Serial.print(allpresets[n][k]); 
    Serial.print("\t");
  }
}
void showmoveall(int n){
  for (int l=0; l<4; l++) {
    Serial.print(movealltoarr[l]); 
    Serial.print("\t");
  }
  Serial.println();
}
void testonepreset(int m, int tick){
  for (int i=0; i<numberofpresets; i++)allpresets[i][4]=0;
  allpresets[m][4] = 99;
  for (int n=0; n<4; n++){
    if (allpresets[m][4]!=0) {
      movealltoarr[n] = allpresets[m][n];
    }
  }
  //moveall();
}
void debug_aktor () {
  Serial.print(ueberforderung);
  Serial.print(": ");
  for (int m=0; m<numberofpresets; m++){
    Serial.print(allpresets[m][4]);
    Serial.print(" ");
  }
  Serial.println();
}

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
        ueberforderung = -1.2*ueberforderung_exp+ueberforderung_exp_spitze;
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

/*void testwerte (){ //überschreibt die überforderung und liefert werte von 0 bis 100 auf und abtsteigend
 ueberforderung = tick%200;
 if (ueberforderung <= 100) testwert++;
 if (ueberforderung > 100) testwert--;
 ueberforderung = testwert;
 }*/






