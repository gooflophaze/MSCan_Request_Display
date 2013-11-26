/*
MSCan_Request_Display - Example program on how to communicate with
a Megasquirt ECU using a CANBus shield without a library. I wrote 
this originally to try and use a little memory as possible.

Suggested using an ANSI capable terminal like putty to view data.

Created by David Will, November 25 2013.

Additional documentation can be found at 
  http://kckr.net/interfacing-megasquirt-with-arduino/

*/
#include <SPI.h>

// Pin Definitions - MCP2515
#define CS_PIN    10
#define MCP_RESET_PIN  11 //this does nothing
#define INT_PIN 0 // int pin != pin output. see below
/* Sparkfun Canbus shield interrupt on D2
Board	       int.0   int.1   int.2   int.3   int.4   int.5
Uno, Ethernet	2	3	 	 	 	 
Mega2560	2	3	21	20	19	18
Leonardo	3	2	0	1	7	 
*/

//mcp2515.h stolen defs
#define CAN_READ        0x03
#define CAN_WRITE       0x02
#define CANINTE         0x2B
#define CANINTF         0x2C
#define BFPCTRL         0x0C
#define CANCTRL         0x0F
#define CANSTAT         0x0E

#define CNF1            0x2A
#define CNF2            0x29
#define CNF3            0x28
#define RXB0CTRL        0x60
#define RXB1CTRL        0x70

// TX Buffer 0
#define TXB0CTRL        0x30
#define TXB0SIDH        0x31
#define TXB0SIDL        0x32
#define TXB0EID8        0x33
#define TXB0EID0        0x34
#define TXB0DLC         0x35
#define TXB0D0          0x36
#define TXB0D1          0x37
#define TXB0D2          0x38
#define TXB0D3          0x39
#define TXB0D4          0x3A
#define TXB0D5          0x3B
#define TXB0D6          0x3C
#define TXB0D7          0x3D
// RX Buffer 0
#define RXB0CTRL        0x60
#define RXB0SIDH        0x61
#define RXB0SIDL        0x62
#define RXB0EID8        0x63
#define RXB0EID0        0x64
#define RXB0DLC         0x65
#define RXB0D0          0x66
#define RXB0D1          0x67
#define RXB0D2          0x68
#define RXB0D3          0x69
#define RXB0D4          0x6A
#define RXB0D5          0x6B
#define RXB0D6          0x6C
#define RXB0D7          0x6D
// RX Buffer 1
#define RXB1CTRL        0x70
#define RXB1SIDH        0x71
#define RXB1SIDL        0x72
#define RXB1EID8        0x73
#define RXB1EID0        0x74
#define RXB1DLC         0x75
#define RXB1D0          0x76
#define RXB1D1          0x77
#define RXB1D2          0x78
#define RXB1D3          0x79
#define RXB1D4          0x7A
#define RXB1D5          0x7B
#define RXB1D6          0x7C
#define RXB1D7          0x7D

unsigned int RPM; 
byte gSIDH, gSIDL, gEID8, gEID0, gDLC;
byte gDATA[8];

void setup() {
  Serial.begin(115200);
  pinMode(MCP_RESET_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.begin();
  digitalWrite(MCP_RESET_PIN, LOW);
  delay(10);
  digitalWrite(MCP_RESET_PIN, HIGH);
  delay(10);
  CANWrite(CANCTRL, B10000111); // conf mode
  CANWrite(CNF1, 0x00); // CNF1 b00000000
  CANWrite(CNF2, 0xA4); // CNF2 b10100100
  CANWrite(CNF3, 0x84); // CNF3 b10000100
  CANWrite(CANCTRL, B00000111); // normal mode
  delay(10);
  CANWrite(RXB0CTRL, B01101000); //RXB0CTRL clear receive buffers
  CANWrite(RXB1CTRL, B01101000); //RXB1CTRL clear receive buffers
  CANWrite(CANINTE, B00000011); // enable interrupt on RXB0, RXB1
  CANWrite(BFPCTRL, B00001111); // setting interrupts 
  
  attachInterrupt(INT_PIN, canISR, FALLING);

}

void loop() {
  // request RPM - block 7, offset 6, 2 bytes
  MSrequest(7, 6, 2);
  Serial.println("RECEIVED ------------------");
  bytePrintColor(gSIDH, gSIDL, gEID8, gEID0, gDLC);
  byte datalength=(gDLC & 0x0F);
  Serial.print("Data Buffer: ");
  for (byte x=0 ; x < datalength ; x++) {
    bytePrint(gDATA[x]);
    }
  Serial.println("\n");
  Serial.print("RPM: ");
  Serial.println(RPM);
  Serial.println("");

  delay(1000);

}

void canISR() {
  /*overly large function for assigning variables back from interrupt - 
    I wish i knew a better way to do this. This interrupt is too large */
  byte SIDH, SIDL, EID8, EID0, DLC;
  byte databuffer[7];
  unsigned int data; 
  byte block, canintf, temp;
  unsigned int offset;

  canintf=CANRead(CANINTF); // which buffer?
//  Serial.print("canintf: ");
//  bytePrint(canintf);
//  Serial.println();
  if (canintf & B00000001) {
    SIDH=CANRead(RXB0SIDH);
    SIDL=CANRead(RXB0SIDL);
    EID8=CANRead(RXB0EID8);
    EID0=CANRead(RXB0EID0);
    DLC=CANRead(RXB0DLC);
    databuffer[0]=CANRead(RXB0D0);
    databuffer[1]=CANRead(RXB0D1);
    databuffer[2]=CANRead(RXB0D2);
    databuffer[3]=CANRead(RXB0D3);
    databuffer[4]=CANRead(RXB0D4);
    databuffer[5]=CANRead(RXB0D5);
    databuffer[6]=CANRead(RXB0D6);
    databuffer[7]=CANRead(RXB0D7);
  } 
  else if (canintf & B00000010) {
    SIDH=CANRead(RXB1SIDH);
    SIDL=CANRead(RXB1SIDL);
    EID8=CANRead(RXB1EID8);
    EID0=CANRead(RXB1EID0);
    DLC=CANRead(RXB0DLC);
    databuffer[0]=CANRead(RXB1D0);
    databuffer[1]=CANRead(RXB1D1);
    databuffer[2]=CANRead(RXB1D2);
    databuffer[3]=CANRead(RXB1D3);
    databuffer[4]=CANRead(RXB1D4);
    databuffer[5]=CANRead(RXB1D5);
    databuffer[6]=CANRead(RXB1D6);
    databuffer[7]=CANRead(RXB1D7);
  }

gSIDH=SIDH; // copy to global vars
gSIDL=SIDL;
gEID8=EID8;
gEID0=EID0;
gDLC=DLC;
for (byte x=0 ; x <= 7; x++) {
  gDATA[x]=databuffer[x];
}

  // convert 2 bytes into an int.
  RPM=(int)(word(databuffer[0], databuffer[1]));

  CANWrite(CANINTF, 0x00); // clear interrupt
}

void CANWrite(byte addr, byte data) {
  digitalWrite(CS_PIN,LOW);
  SPI.transfer(CAN_WRITE);
  SPI.transfer(addr);
  SPI.transfer(data);
  digitalWrite(CS_PIN,HIGH);
}

byte CANRead(byte addr) {
  byte data;
  digitalWrite(CS_PIN,LOW);
  SPI.transfer(CAN_READ);
  SPI.transfer(addr);
  data=SPI.transfer(0x00);
  digitalWrite(CS_PIN,HIGH);
  return data;
}

void MSrequest(byte block, unsigned int offset, byte req_bytes) {
  byte SIDH, SIDL, EID8, EID0, DLC, D0, D1, D2;
  
SIDH = lowByte(offset  >> 3);

// var_offset<2:0> SRR IDE msg_type <3:0>
SIDL = (lowByte((offset << 5)) | B0001000); //set IDE bit
//      MFFFFTTT msg_type, From, To
EID8 = B10011000; //:7 msg_req, from id 3 (4:3)

//      TBBBBBSS To, Block, Spare
EID0 = ( ( block & B00001111) << 3); // last 4 bits, move them to 6:3
EID0 = ((( block & B00010000) >> 2) | EID0); // bit 5 goes to :2

DLC = B00000011;
D0=(block);
D1=(offset >> 3);
D2=(((offset & B00000111) << 5) | req_bytes); // shift offset
  
  
digitalWrite(CS_PIN,LOW);
  SPI.transfer(0x40); // Push bits starting at 0x31 (RXB0SIDH)
  SPI.transfer(SIDH); //0x31
  SPI.transfer(SIDL); //0x32
  SPI.transfer(EID8); //0x33
  SPI.transfer(EID0); //0x34
  SPI.transfer(DLC);  //0x35
  SPI.transfer(D0); // 0x36 TXB0D0 my_varblk
  SPI.transfer(D1); // 0x37 TXB0D1 my_offset
  SPI.transfer(D2); // 0x38 TXB0D2 - request 8 bytes(?) from MS3
digitalWrite(CS_PIN,HIGH); // end write

// RTS - Send this buffer down the wire
digitalWrite(CS_PIN,LOW);
  SPI.transfer(B10000001);
digitalWrite(CS_PIN,HIGH);

CANWrite(CANINTF,0x00);

  Serial.println("TRANSMIT ------------------");
  bytePrintColor(SIDH, SIDL, EID8, EID0, DLC);
  Serial.print("Data Buffer: ");
  bytePrint(D0);
  bytePrint(D1);
  bytePrint(D2);
  Serial.println("");
}

void bytePrint(byte victim) {
  boolean temp;
  Serial.print("b");
  for (int x = 7; x >=0; x--) {
    temp=bitRead(victim,x);
    Serial.print(temp,BIN);
  }
}

void bytePrintColor(byte SIDH, byte SIDL, byte EID8, byte EID0, byte DLC) {
  /*looks really ugly - but produces colored output with an ANSI capable
    serial terminal. 
    */
#define BRACE 0x5B
#define ESCAPE 0x1B
byte temp;

Serial.write(ESCAPE); //SIDH - all var offset
Serial.write(BRACE);
Serial.print("0;39;49m");
Serial.print("SIDH: b");
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;46m");
for (int x = 7; x >=0; x--) {
    temp=bitRead(SIDH,x);
    Serial.print(temp,BIN);
  }
Serial.write(ESCAPE); // SIDL - var_offset, control bits, msg_type
Serial.write(BRACE);
Serial.print("0;39;49m");
Serial.println("   // var_offset"); // end SIDH


Serial.print("SIDL: b"); //begin SIDL
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;46m");
Serial.print(bitRead(SIDL,7),BIN); //var_offset
Serial.print(bitRead(SIDL,6),BIN); //var_offset
Serial.print(bitRead(SIDL,5),BIN); //var_offset
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("1;36;49m");
//control bits - EXIDE, etc
Serial.print(bitRead(SIDL,4),BIN); //SRR
Serial.print(bitRead(SIDL,3),BIN); //IDE
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;37;47m");
Serial.print(bitRead(SIDL,2),BIN); //not used
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;30;43m");
Serial.print(bitRead(SIDL,1),BIN); //msg_type
Serial.print(bitRead(SIDL,0),BIN); //msg_type
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;49m");
Serial.println("   // var_offset, SRR + IDE, NA, msg_type"); //end SIDL


Serial.print("EID8: b");
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;30;43m");
Serial.print(bitRead(EID8,7),BIN); //msg_type
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;42m");
Serial.print(bitRead(EID8,6),BIN); //from_id
Serial.print(bitRead(EID8,5),BIN); //from_id
Serial.print(bitRead(EID8,4),BIN); //from_id
Serial.print(bitRead(EID8,3),BIN); //from_id
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;41m");
Serial.print(bitRead(EID8,2),BIN); //to_id
Serial.print(bitRead(EID8,1),BIN); //to_id
Serial.print(bitRead(EID8,0),BIN); //to_id
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;49m");
Serial.println("   // msg_type, from_id, to_id");


Serial.print("EID0: b");
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;41m");
Serial.print(bitRead(EID0,7),BIN); //to_id
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;44m"); 
Serial.print(bitRead(EID0,6),BIN); //var_block
Serial.print(bitRead(EID0,5),BIN); //var_block
Serial.print(bitRead(EID0,4),BIN); //var_block
Serial.print(bitRead(EID0,3),BIN); //var_block
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("1;34;46m"); 
Serial.print(bitRead(EID0,2),BIN); //var_block - bit 5
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;45m"); 
Serial.print(bitRead(EID0,1),BIN); //spare
Serial.print(bitRead(EID0,0),BIN); //spare
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;49m"); 
Serial.println("   // to_id, var_block 3:0, var_block 4:, spare");

Serial.print("DLC: ");
bytePrint(DLC);
Serial.println("");
}


