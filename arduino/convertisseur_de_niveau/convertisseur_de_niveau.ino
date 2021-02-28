/*
* Programme simple de convertisseur de niveau car la tension des GPIO du jetson s'écrase facilement...
*/

int IN_1=2;
int IN_2=3;
int IN_3=4;
int IN_4=5;
int IN_5=6;
int IN_6=7;
int IN_7=8;

int OUT_1=9;
int OUT_2=10;
int OUT_3=11;
int OUT_4=12;
int OUT_5=13;
int OUT_6=14; // A0
int OUT_7=15; // A1

void setup() {
  pinMode(IN_1, INPUT);
  pinMode(IN_2, INPUT);
  pinMode(IN_3, INPUT);
  pinMode(IN_4, INPUT);
  pinMode(IN_5, INPUT);
  pinMode(IN_6, INPUT);
  pinMode(IN_7, INPUT);

  pinMode(OUT_1, OUTPUT);
  pinMode(OUT_2, OUTPUT);
  pinMode(OUT_3, OUTPUT);
  pinMode(OUT_4, OUTPUT);
  pinMode(OUT_5, OUTPUT);
  pinMode(OUT_6, OUTPUT);
  pinMode(OUT_7, OUTPUT);
}

void loop() {
  digitalWrite(OUT_1, (digitalRead(IN_1)));
  digitalWrite(OUT_2, (digitalRead(IN_2)));
  digitalWrite(OUT_3, (digitalRead(IN_3)));
  digitalWrite(OUT_4, (digitalRead(IN_4)));
  digitalWrite(OUT_5, (digitalRead(IN_5)));
  digitalWrite(OUT_6, (digitalRead(IN_6)));
  digitalWrite(OUT_7, (digitalRead(IN_7)));
}
