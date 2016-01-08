//7 digit voltmeter 
//24bit ADC IC: LTC2400
//4.096 precision reference: TI REF3040
//
//Adopted and expanded by Wojciech Krysmann 
//
//originaly:
//By coldtears electronics
//LTC2400 code is adapted from Martin Nawrath
//Kunsthochschule fuer Medien Koeln
//Academy of Media Arts Cologne
//
//

#include <LiquidCrystal.h>
#include <Stdio.h>
#include <stdlib.h>
#include <Average.h>

//screen initialize
LiquidCrystal lcd(9, 8, 5, 4, 3, 2);
int menuCur = 1;

//ltc 2400 initialize
#ifndef cbi
#define cbi(sfr, bit)     (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit)     (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define LTC_CS 2         // LTC2400 Chip Select Pin on Portb 2
#define LTC_MISO  4      // LTC2400 SDO Select Pin on Portb 4
#define LTC_SCK  5       // LTC2400 SCK Select Pin on Portb 5

// buttons initialize.
const int button1Pin = 7;
const int button2Pin = 6;
int button1State = 0;         // variable for reading the pushbutton status
int button2State = 0; 
int timer = 200;

// Reserve space for "averages" entries in the average bucket.
// Change the type between < and > to change the entire way the library works.
// AND set the refresh rate for averages on screen
int averages = 20;
int maxes = 20;
int minis = 20;
// Size of table which holds the values
int table = 20;

Average<float> ave(table);
  
int minat = 0;
int maxat = 0;

int slow_disp = 0;
int slow_disp2 = 0;
int slow_disp3 = 0;

float volt;
float v_ref = 4.094;        // Reference Voltage
long int ltw = 0;         // ADC Data long int
byte b0;
byte sig;                 // sign bit flag
char real[10];            // pomocnicza do konwersji

void setup() {
  lcd.begin(20, 4); //configure

  lcd.print("Voltmeter");
  lcd.setCursor(15, 3);
  lcd.print("v1.0");
  delay(800);
  lcd.clear();
  
  lcd.setCursor(1, 0);
  lcd.print("Real:");
  
  lcd.setCursor(0, 1);
  lcd.print(">Ave(");
  lcd.setCursor(7, 1);
  lcd.print("):");
  
  lcd.setCursor(1, 2);
  lcd.print("Max(");
  lcd.setCursor(7, 2);
  lcd.print("):");
  
  lcd.setCursor(1, 3);
  lcd.print("Min(");
  lcd.setCursor(7, 3);
  lcd.print("):");
  
  cbi(PORTB, LTC_SCK);     // LTC2400 SCK low
  sbi (DDRB, LTC_CS);      // LTC2400 CS HIGH

  cbi (DDRB, LTC_MISO);
  sbi (DDRB, LTC_SCK);

  // init SPI Hardware
  sbi(SPCR, MSTR) ; // SPI master mode
  sbi(SPCR, SPR0) ; // SPI speed
  sbi(SPCR, SPR1); // SPI speed
  sbi(SPCR, SPE);  //SPI enable

  // initialize the pushbutton pin as an input:
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
}

/********************************************************************/
void loop() {

  // read the state of the pushbutton value:
  button1State = digitalRead(button1Pin);
  button2State = digitalRead(button2Pin);
 
  if (button1State == HIGH) {
      menuCur++;
      if(menuCur >= 4) menuCur = 0;
      for(int i = 1; i <= 3; i++) {
        lcd.setCursor(0, i);
        lcd.print(" ");
      }
      if(menuCur == 0) {
        for(int i = 1; i <= 3; i++) {
          lcd.setCursor(0, i);
          lcd.print(">");
        }
      } else {
        lcd.setCursor(0, menuCur);
        lcd.print(">");
      }
      delay(timer);
  }
  
  if (button2State == HIGH) {
    switch (menuCur) {
       case 0:
        if ((minis != maxes) || (maxes != averages)) {
          minis = 10; maxes = 10; averages = 10;
        } else {
          averages += 10;
        }
        if (averages >= 50) averages = 10;
        maxes = minis = averages;
        slow_disp = 100;
        slow_disp2 = slow_disp3 = slow_disp;
        delay(timer);
        break;
      case 1:
        averages += 10;
        if (averages >= 50) averages = 10;
        slow_disp = 100;
        delay(timer);
        break;
      case 2:
        maxes += 10;
        if (maxes >= 50) maxes = 10;
        slow_disp2 = 100;
        delay(timer);
        break;
      case 3:
        minis += 10;
        if (minis >= 50) minis = 10;
        slow_disp3 = 100;
        delay(timer);
        break;
      default:
        lcd.setCursor(0,0);
        lcd.print("E");
        break;
    }
  }
  cbi(PORTB, LTC_CS);            // LTC2400 CS Low
  delayMicroseconds(1);
  if (!(PINB & (1 << 4))) {    // ADC Converter ready ?
    //    cli();
    ltw = 0;
    sig = 0;

    b0 = SPI_read();             // read 4 bytes adc raw data with SPI
    if ((b0 & 0x20) == 0) sig = 1; // is input negative ?
    b0 &= 0x1F;                  // discard bit 25..31
    ltw |= b0;
    ltw <<= 8;
    b0 = SPI_read();
    ltw |= b0;
    ltw <<= 8;
    b0 = SPI_read();
    ltw |= b0;
    ltw <<= 8;
    b0 = SPI_read();
    ltw |= b0;

    delayMicroseconds(1);

    sbi(PORTB, LTC_CS);          // LTC2400 CS Low

    if (sig) ltw |= 0xf0000000;    // if input negative insert sign bit
    ltw = ltw / 16;                // scale result down , last 4 bits have no information
    volt = ltw * v_ref / 16777216; // max scale
    volt = volt * 4.5;
    dtostrf(volt, 9, 6, real);
    ave.push(volt);
    lcd.setCursor(11, 0);
    lcd.print(real);
  if ( slow_disp >= averages)
  {
    slow_disp = 0;
    lcd.setCursor(11, 1);
    lcd.print(dtostrf(ave.mean(), 9, 6, real));
    lcd.setCursor(5, 1);
    lcd.print(averages);

  }
  slow_disp++;
  if ( slow_disp2 >= maxes)
  {
    slow_disp2 = 0;
      lcd.setCursor(11, 2);
      lcd.print(dtostrf(ave.maximum(&maxat), 9, 6, real));
      lcd.setCursor(5, 2);
      lcd.print(maxes);
        }
  slow_disp2++;
  if ( slow_disp3 >= minis)
  {
    slow_disp3 = 0;
      lcd.setCursor(11, 3);
      lcd.print(dtostrf(ave.minimum(&minat), 9, 6, real));
      lcd.setCursor(5, 3);
      lcd.print(minis);
   }
  slow_disp3++;
  }
  sbi(PORTB, LTC_CS); // LTC2400 CS hi
  delay(20);
}
/********************************************************************/
byte SPI_read()
{
  SPDR = 0;
  while (!(SPSR & (1 << SPIF))) ; /* Wait for SPI shift out done */
  return SPDR;
}


