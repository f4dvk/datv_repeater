// Mémoire tampon commande via série
String cmd = String("");

// Entrées
int OUTA_1=2;
int OUTA_2=4;
int OUTA_3=6;
int OUTA_4=8;
int OUTA_5=10;
int OUTA_6=13;

int OUTB_1=3;
int OUTB_2=5;
int OUTB_3=7;
int OUTB_4=9;
int OUTB_5=12;
int OUTB_6=14;

// pulse parameters in usec paramètres code infrarouge
#define NEC_HDR_MARK  9000
#define NEC_HDR_SPACE 4500
#define NEC_BIT_MARK  560
#define NEC_ONE_SPACE 1600
#define NEC_ZERO_SPACE  560
#define NEC_RPT_SPACE 2250

#define TOPBIT 0x80000000

// Sortie
const int OutputPin = 11;

void setup()
{
  Serial.begin(9600);

  pinMode(OUTA_1, INPUT);
  pinMode(OUTA_2, INPUT);
  pinMode(OUTA_3, INPUT);
  pinMode(OUTA_4, INPUT);
  pinMode(OUTA_5, INPUT);
  pinMode(OUTA_6, INPUT);

  pinMode(OUTB_1, INPUT);
  pinMode(OUTB_2, INPUT);
  pinMode(OUTB_3, INPUT);
  pinMode(OUTB_4, INPUT);
  pinMode(OUTB_5, INPUT);
  pinMode(OUTB_6, INPUT);

  pinMode(OutputPin, OUTPUT);
  digitalWrite(OutputPin, HIGH);
}

void Send(unsigned long data, int nbits) {
  mark(NEC_HDR_MARK);
  space(NEC_HDR_SPACE);

  for (int i = 0; i < nbits; i++) {
    if (data & TOPBIT) {
      mark(NEC_BIT_MARK);
      space(NEC_ONE_SPACE);
    } 
    else {
      mark(NEC_BIT_MARK);
      space(NEC_ZERO_SPACE);
    }
    data <<= 1;
  }
  mark(NEC_BIT_MARK);
  space(0);
}

void mark(int time) {
  digitalWrite(OutputPin, LOW); // LOW si branchement direct, HIGH si via transistor.
  delayMicroseconds(time);
}

void space(int time) {
  digitalWrite(OutputPin, HIGH);
  delayMicroseconds(time);
}

void commande_rs232(){
  // Vérifier si des caractères sont reçu sur le port série
  if (Serial.available() >0 ) {
     
     // Variable temporaire pour copier le caractère lu
     char SerialInByte;
     // Lire un caractère (un Byte)
     SerialInByte = Serial.read();
      
     // Si c'est <CR> --> continuer sans ajouter en mémoire tampon
     if(SerialInByte == 13)
     {}
     // Si c'est <LF> --> Traiter la commande
     else if(SerialInByte == 10)
     {
       //Serial.println(cmd);
       Commande();
     }
     else
     {
        // sinon, ajouter le caractère à la mémoire tampon (cmd)
       cmd += String(SerialInByte);
          
        // et afficher un message de déboggage
        //Serial.print( "SerialInByte: " );
        //Serial.print( SerialInByte, DEC );
        //Serial.print( ", Buffer " );
        //Serial.println( cmd );
     }
  }
}

void clearCmd(){
    cmd="";
    Serial.flush();
}

void Commande()
{
  int tps=500;

  if (cmd.equals("OUTA_1"))
  {
    Send(0x807F40BF, 32); // OUT:A IN:1
  }

  if (cmd.equals("OUTA_2"))
  {
    Send(0x807FB04F, 32); // OUT:A IN:2
  }

  if (cmd.equals("OUTA_3"))
  {
    Send(0x807F50AF, 32); // OUT:A IN:3
  }

  if (cmd.equals("OUTA_4"))
  {
    Send(0x807F48B7, 32); // OUT:A IN:4
  }

  if (cmd.equals("OUTA_5"))
  {
    Send(0x807F8877, 32); // OUT:A IN:5
  }

  if (cmd.equals("OUTA_6"))
  {
    Send(0x807F08F7, 32); // OUT:A IN:6
  }


  if (cmd.equals("OUTB_1"))
  {
    Send(0x807F708F, 32); // OUT:B IN:1
  }

  if (cmd.equals("OUTB_2"))
  {
    Send(0x807F30CF, 32); // OUT:B IN:2

  }

  if (cmd.equals("OUTB_3"))
  {
    Send(0x807FF00F, 32); // OUT:B IN:3
  }

  if (cmd.equals("OUTB_4"))
  {
    Send(0x807F906F, 32); // OUT:B IN:4
  }

  if (cmd.equals("OUTB_5"))
  {
    Send(0x807FA05F, 32); // OUT:B IN:5
  }

  if (cmd.equals("OUTB_6"))
  {
    Send(0x807F807F, 32); // OUT:B IN:6
  }

  if (cmd.equals("CG"))
  {
    tps=1;
  }

  //Send(0x807FF807, 32); // ON/OFF

  delay(tps);
  etat();

  clearCmd();

}

void etat()
{

  if ((digitalRead(OUTA_1))==LOW) Serial.print("1");
  if ((digitalRead(OUTA_2))==LOW) Serial.print("2");
  if ((digitalRead(OUTA_3))==LOW) Serial.print("3");
  if ((digitalRead(OUTA_4))==LOW) Serial.print("4");
  if ((digitalRead(OUTA_5))==LOW) Serial.print("5");
  if ((digitalRead(OUTA_6))==LOW) Serial.print("6");

  Serial.print(".");

  if ((digitalRead(OUTB_1))==LOW) Serial.println("1");
  if ((digitalRead(OUTB_2))==LOW) Serial.println("2");
  if ((digitalRead(OUTB_3))==LOW) Serial.println("3");
  if ((digitalRead(OUTB_4))==LOW) Serial.println("4");
  if ((digitalRead(OUTB_5))==LOW) Serial.println("5");
  if ((digitalRead(OUTB_6))==LOW) Serial.println("6");

  //else Serial.println("NULL??");
}

void loop()
{
  commande_rs232();
}
