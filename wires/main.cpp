#include <Arduino.h>
#include <math.h>

//===MULTIPLEXER===
int latchPin = 8;
//Pin connected to SH_CP of 74HC595
int clockPin = 12;
////Pin connected to DS of 74HC595
int dataPin = 11;

//===Ohm Meter===
int raw= 0;
float Vin= 4.97;
float Vout= 0.0;
float R1= 560;
float R2= 0;
float buffer= 0;
float magicNumber = 860; //temporary until the rest of the circuit is designed

//==colors==
short rBlackLow = 195;
short rBlueLow = 350; //nom 350, 430
short rRedLow= 500; //nom 500, 620
short r1= 557;
short rWhiteLow = 740; //nom 740, 900
short rYellowLow = 940; //nom 940, 1340

short rBlackHigh = 300; //nom 160, 300
short rBlueHigh = 430; //nom 500, 620 
short rRedHigh = 620; //nom 500, 620
short rWhiteHigh = 900; //nom 740, 900

//==game logic==
byte wireToCut = 0;
byte connectedWireCount = 0;

bool completed = false;
byte mistakes = 0;


short determineResistor() {
      raw= analogRead(A0);
      if(raw) 
      {
        buffer = raw * Vin;
        Vout= (buffer)/magicNumber;
        buffer=(Vin/Vout) -1;
        R2= R1 * buffer;
        //Serial.println(R2);
        return R2;
      }
      return 0;
}


void shiftTo(byte index) {
    //idx 1 is 560ohms
    int j = 1 << index;

    // digitalWrite(outputEnable, HIGH);
    digitalWrite(latchPin, LOW);

    // shift out the bits:
    shiftOut(dataPin, clockPin, MSBFIRST, j);

    //at this point, the old data is still being displayed
    //signaling the latch pin runs the shift operation and
    //turns the old data into the new data
    digitalWrite(latchPin, HIGH);
    }

 /*
  * 0=unk, 1 = black, 2 = blue, 3 = red, 4 = yellow, white = 5;
  */
byte getWireColor(byte index) {

  shiftTo(index);

  //write to the multiplexer
  short r2 = determineResistor();
  digitalWrite(dataPin, LOW);
        if(r2 > rRedHigh) {
          if(r2 < rYellowLow) {
            //white
            return 5;
          } else {
            //yellow
            return 4;
          }
        } else {
          if(r2 < rBlueLow) {
            if(r2 < rBlackLow) {
              //not connected
              return 0;
            }
            //black
            return 1;
          } else {
            if(r2 > rBlueHigh) {
              //red
              return 3;
            } else {
              //blue
              return 2;
            }
          }
        }
        

    return 0;
}

//==DEBUG ONLY==
void printColors() {
  // count from 0 to 255 and display the number
  // on the LEDs
 for (int i = 0; i < 9; i++) {
   byte color = getWireColor(i);
   // 0=unk, 1 = black, 2 = blue, 3 = red, 4 = yellow, white = 5;
   Serial.print("Index ");
   Serial.print(i);
   switch(color) {
     case 0:
     Serial.println(" none");
     break;
     case 1:
     Serial.println(" black");
     break;
          case 2:
     Serial.println(" blue");
     break;
          case 3:
     Serial.println(" red");
     break;
          case 4:
     Serial.println(" yellow");
     break;
          case 5:
     Serial.println(" white");
     break;
   }
  }
}

//Returns the index of the connected wire which must be cut</returns>
byte getWireToCut(char lastChar) {
  byte black = 0;
  byte blue = 0;
  byte red = 0;
  byte yellow = 0;
  byte white  = 0;
  byte lastColor = 0; //0=unk, 1 = black, 2 = blue, 3 = red, 4 = yellow, 5 = white
  bool oddSerial = false; //false
  byte lastBlue = 0;
  byte lastRed = 0;
  byte lastIndex = 0;
  byte secondIndex = 0;
  byte thirdIndex = 0;
  byte fourthIndex = 0;
  byte firstIndex = 0;

  if(lastChar == '1' || lastChar == '3' || lastChar == '5' || lastChar == '7' || lastChar == '9') {
    oddSerial = true;
  }

//we actually skip index 0, because it is on the other side of the IC
//The code is now dependent on this
  for(byte i = 1; i < 7; i++) {
    byte col = getWireColor(i);

    switch(col) {
      case 0:
        //do not add if we return 0
      continue;
      case 1:
        black++;
      break;
      case 2:
        blue++;
        lastBlue = i;
      break;
      case 3:
        red++;
        lastRed = i;
      break;
      case 4:
        yellow++;
      break;
      case 5:
        white++;
      break;
    }
    
    if(firstIndex == 0 ) {
      firstIndex = i;
    }
    lastColor = col;
    connectedWireCount++;//if 1,2,3,4

    if(connectedWireCount == 2) {
      secondIndex = i;
    }
    if(connectedWireCount == 3) {
      thirdIndex = i;
    }
    if(connectedWireCount == 4) {
      fourthIndex = i;
    }
    lastIndex = i;
  }
  
  //assign ruleset
  switch(connectedWireCount) {
    case 3:
      if(red == 0) {
        Serial.println("Ruleset 1");
        // 01  If there are no red wires, cut the second wire.
        return secondIndex;
      } else if(lastColor == 5) {
        Serial.println("Ruleset 2");
        // 02  Otherwise, if the last wire is white, cut the last wire.
        return lastIndex;
      } else if(blue > 1) {
        Serial.println("Ruleset 3");
        // 03  Otherwise, if there is more than one blue wire, cut the last blue wire.
        return lastBlue;
      } else {
        Serial.println("Ruleset 4");
        // 04  Otherwise, cut the last wire.
        return lastIndex;
      }
    break;
    case 4:
      if(red > 1 && oddSerial) {
        Serial.println("Ruleset 5");
        //05  If there is more than one red wire and the last digit of the serial number is odd, cut the last red wire.
        return lastRed;
      } else if(lastColor == 4 && red == 0) {
        Serial.println("Ruleset 6");
        //06  Otherwise, if the last wire is yellow and there are no red wires, cut the first wire.
        return firstIndex;
      } else if(blue == 1) {
        Serial.println("Ruleset 7");
        //07  Otherwise, if there is exactly one blue wire, cut the first wire.
        return firstIndex;
      } else if(yellow > 1) {
        Serial.println("Ruleset 8");
        //08  Otherwise, if there is more than one yellow wire, cut the last wire.
        return lastIndex;
      } else {
        Serial.println("Ruleset 9");
        //09  Otherwise, cut the second wire.
        return secondIndex;
      }
    break;
    case 5:
      if(lastColor == 1 && oddSerial) {
        Serial.println("Ruleset 10");
        //10  If the last wire is black and the last digit of the serial number is odd, cut the fourth wire.
        return fourthIndex;
      } else if(red == 1 && yellow > 1) {
        Serial.println("Ruleset 11");
        //11  Otherwise, if there is exactly one red wire and there is more than one yellow wire, cut the first wire.
        return firstIndex;
      } else if (black == 0) {
        Serial.println("Ruleset 12");
        //12  Otherwise, if there are no black wires, cut the second wire.
        return secondIndex;
      } else {
        Serial.println("Ruleset 13");
        //13  Otherwise, cut the first wire.
        return firstIndex;
      }

    break;
    case 6:
      if(yellow == 0 && oddSerial) {
        return thirdIndex;
        Serial.println("Ruleset 14");
        return 14;
      } else if(red == 0) {
        return lastIndex;
        Serial.println("Ruleset 16");
        return 16;
      } else {
        return fourthIndex;
        Serial.println("Ruleset 15 && 17");
        return 17;
      }
    break;
    default:
      //error
    break;
  }
}

void redlight() {
  shiftTo(0);
}

void greenlight() {
  shiftTo(7);
}

void setup() {
  Serial.begin(9600);
  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  pinMode(A0, INPUT);
  // pinMode(masterReset, OUTPUT);
  // pinMode(outputEnable, OUTPUT);

  Serial.println("On");
  printColors();

  wireToCut = getWireToCut('c');
  Serial.print("Index to cut: " );
  Serial.println(wireToCut);
  

  redlight();
  delay(100);
  greenlight();
  delay(100);
}



void lose() {
  Serial.println("LOST");
  redlight();
}

void solved() {
  Serial.println("SOLVED");
  completed = true;
  greenlight();
}

void mistake() {
  Serial.print("X");
  redlight();
  mistakes++;
  if(mistakes == 3) {
    lose();
  }
  delay(250);
}

void checkWires() {
byte wireCount = 0;
bool potentialSolve = false;
  for(byte i = 1; i < 7; i++) {
    if(getWireColor(i) == 0) {
      if(i == wireToCut) {
        //win
        potentialSolve = true;
        connectedWireCount--;
        //do not return, check for cheaters who pulled all wires;
      }
      //no wire here
    } else {
      wireCount++;
    }
  }

  if(wireCount != connectedWireCount) {
  //  Serial.println(wireCount);
  //  Serial.println(connectedWireCount);
    //a wire was cut, and it wasn't the right one
    mistake();
    connectedWireCount--;
  }
  if(potentialSolve) {
    solved();
  }
}

void loop() {
  delay(500);
  checkWires();
  if(completed || mistakes >= 3) {
    delay(60000);
  }
}
