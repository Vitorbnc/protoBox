//operating system for the box
//Code by Vítor Barbosa

// ShiftPWM uses timer1 by default. To use a different timer, before '#include <ShiftPWM.h>', add
// #define SHIFTPWM_USE_TIMER2  // for Arduino Uno and earlier (Atmega328)
// #define SHIFTPWM_USE_TIMER3  // for Arduino Micro/Leonardo (Atmega32u4)

// Clock and data pins are pins from the hardware SPI, you cannot choose them yourself if you use the hardware SPI.
// Data pin is MOSI (Uno and earlier: 11, Leonardo: ICSP 4, Mega: 51, Teensy 2.0: 2, Teensy 2.0++: 22) 
// Clock pin is SCK (Uno and earlier: 13, Leonardo: ICSP 3, Mega: 52, Teensy 2.0: 1, Teensy 2.0++: 21)

// You can choose the latch pin yourself.
const int ShiftPWM_latchPin=53;

// ** uncomment this part to NOT use the SPI port and change the pin numbers. This is 2.5x slower **
// #define SHIFTPWM_NOSPI
// const int ShiftPWM_dataPin = 11;
// const int ShiftPWM_clockPin = 13;


// If your LED's turn on if the pin is low, set this to true, otherwise set it to false.
const bool ShiftPWM_invertOutputs = true; 

// You can enable the option below to shift the PWM phase of each shift register by 8 compared to the previous.
// This will slightly increase the interrupt load, but will prevent all PWM signals from becoming high at the same time.
// This will be a bit easier on your power supply, because the current peaks are distributed.
const bool ShiftPWM_balanceLoad = false;

#include <IRremote.h>
#include <Streaming.h>
#include <LiquidCrystal.h>
#include<Wire.h>
#include <WiiChuck.h>
#include<ShiftPWM.h>


// Here you set the number of brightness levels, the update frequency and the number of shift registers.
// These values affect the load of ShiftPWM.
// Choose them wisely and use the PrintInterruptLoad() function to verify your load.

unsigned char maxBrightness = 255;
unsigned char pwmFrequency = 38;
//default is 75Hz, slowed down to allow irsend
//unsigned char pwmFrequency = 75;

int numRegisters = 3;
int numRGBleds = numRegisters*8/3;

// pin of the transistor "switch" to enable lights + Supply(VDD)
const int lightsVDD = 44;

// pin to read battery voltage from a voltage divider
const int batPin = A0;

//IR interface stuff
const int irrecv_pin =45;
//const int irled =9;

//IRrecv irrecv(irrecv_pin);
IRsend irsend;

//Philips RC6 codes
const int RC6pwr = 0xc, RC6volUp = 0x10, RC6volDw = 0x11, RC6mute = 0xD, RC6chUp = 0x4C, RC6chDw = 0x4D, RC6src = 0x38, RC6return = 0xA, RC6home = 0x54;
//RC6 sends 20 bits

//relays
const int rlInt_pin = 42;
const int rlExt_pin = 43;

//inductance meter
const int inductance_Out = 40;
const int inductance_In = 41;

float pulse,frequency,capacitance,inductance;




// create nunchuk object
WiiChuck chuck = WiiChuck();

/*  The LCD circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

*/
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(13, 12, 46, 47, 48, 49);

//boolean lightsVDD = false;

//Values for joystick X axis
const int maxUp = -105,maxDown = 110;
const int trigUp = -85,trigDown = 90;
//For joystick Y axis
const int maxLeft = -123,maxRight = 111;
const int trigLeft = -103,trigRight = 91;
int index,sideIndex,led_sInd;
const int min_index = -1;
const int max_index = 3;
const int min_sideIndex = -1;
const int max_sideIndex = 1;

boolean navLock;
boolean ok;
boolean cancel;

//Variáveis de leitura de dados seriais
const int fields =4;
int valuesbt_a[fields]; // novos valores
int fieldindexbt = 0;
int valuesbt[fields]; // valores antigos
int sign =1;

char chbt; // caractere recebido por serial bt
char ch; // leitura de dados seriais bluetooth(bt)
char lastchbt; // último dado recebido por serial1 bt



float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void setup() {

    // PC serial
  Serial.begin(115200);
  // bluetooth serial
  Serial2.begin(115200);
  
//irrecv.enableIRIn(); // Start the receiver

  
  // Sets the number of 8-bit registers that are used.
  ShiftPWM.SetAmountOfRegisters(numRegisters);

  // SetPinGrouping allows flexibility in LED setup. 
  // If your LED's are connected like this: RRRRGGGGBBBBRRRRGGGGBBBB, use SetPinGrouping(4).
  ShiftPWM.SetPinGrouping(1); //This is the default, but I added here to demonstrate how to use the funtion
  
  // start function is blocking IRremote
  ShiftPWM.Start(pwmFrequency,maxBrightness);
  //delay(100);
   ShiftPWM.SetAll(0);

  
  chuck.begin();
 // delay(100);
  chuck.update();
  
// relay pin setup
pinMode(rlInt_pin,OUTPUT);
pinMode(rlExt_pin,OUTPUT);

//inductance meter pin setup
pinMode(inductance_In,INPUT);
pinMode(inductance_Out,OUTPUT);

pinMode(lightsVDD,OUTPUT);
  
   // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.setCursor(6,0);
  lcd << "boxOS";
  lcd.setCursor(8,1);
  lcd<<"by Gello";
   delay(1500);
   
  
}

void loop() {
  // put your main code here, to run repeatedly: 
// lcd.setCursor(0,0);
// lcd.clear();
readserialbt();
 menu();
 backgroundledMan();
// delay(150);
}

void meterMan(){
 lcd.setCursor(6,0);
   lcd<<"Medir";
   lcd.setCursor(0,1);
  switch(sideIndex){
    
    case 0:
lcd<<"Indutor:";
  inductanceMeter();
break;
  }
}

void inductanceMeter(){
digitalWrite(inductance_Out,HIGH);
delay(5);
//give some time to charge inductor.
digitalWrite(inductance_Out,LOW);
delayMicroseconds(100);
//make sure resination is measured
//pulse in for smaller inductors
//pulse=pulseIn(11,HIGH,5000);

pulse=pulseIn(inductance_In,HIGH,20000);

//returns 0 if timeout
if(pulse>0.1){
  //if a timeout did not occur and it took a reading:
capacitance=2.E-6;//insert capacitance here. Currently using 2uF
frequency=1.E6/(2*pulse);
inductance=1./(capacitance*frequency*frequency*4.*3.14159*3.14159);
//one of my profs told me just do squares like this
inductance*=1E6;
//note that this is the same as saying inductance = inductance*1E6

if(inductance<1e3){
lcd<<inductance;
lcd<<"uH";
}
if(inductance>1e3 && inductance<1e6){
lcd<<(inductance/1e3);
lcd<<"mH";
}
if(inductance>1e6 && inductance<1e9){
lcd<<(inductance/1e6);
lcd<<"H";
}

//delay(100);
delay(20);
}
}  

void ledMan(){
  lcd.setCursor(6,0);
   lcd<<"LEDs";
   lcd.setCursor(0,1);
  switch(sideIndex){
    
    case 0:
   lcd<<"Acender/Apagar";
   if(cancel){
   ShiftPWM.SetAll(0);
   digitalWrite(lightsVDD,LOW);
   cancel = false;
   }
   if(ok){
   led_sInd = 0;
   digitalWrite(lightsVDD,HIGH);
   ShiftPWM.SetAll(255);
     ok = false;
   }
   break;
   
   case 1:
   lcd<<"Chuva de cores";
   if(ok)
   led_sInd = 1;
   digitalWrite(lightsVDD,HIGH);
   ok = false;
   if(cancel)
   led_sInd = 0;
   cancel = false;
   break;
   
  }
}
void backgroundledMan(){
  switch (led_sInd){
    case 1:
    rgbLedRainbow(numRGBleds, 5, 3, numRegisters*8/3); // Fast, over all LED's
    break;
    
    default:
    
    break;
  }
}
void navigator(){
 if(!navLock){
  if(read_nunchuck('x')>=trigUp)
  index=min(index++,max_index);
 
   if(read_nunchuck('x')<=trigDown)
  index=max(index--,min_index);
  
   if(read_nunchuck('y')<=trigLeft)
 sideIndex = max(sideIndex--,min_sideIndex);
   if(read_nunchuck('y')>=trigRight)
 sideIndex = min(sideIndex++,max_sideIndex);
  if(read_nunchuck('z')==1)
  ok = true;
  if(read_nunchuck('c')==1)
  cancel = true;
  
  
 // delay(25);
 }
}

void menu(){
  navigator();
  lcd.clear();
 switch(index){
   case 0:
   showBat();
    break;
    case 1:
    ledMan();
    break;
    case -1:
    relayMan();
    break;
    case 2:
    meterMan();
    break;
 }
 delay(25);
// lcd.clear();
}

void relayMan(){
 lcd.setCursor(5,0);
   lcd<<"Reles";
   lcd.setCursor(0,1);
  switch(sideIndex){
    
    case 0:
   lcd<<"ZVS: VDC Interno";
   if(cancel){
   digitalWrite(rlInt_pin,LOW);
    cancel = false;
   }
   if(ok){
     digitalWrite(rlExt_pin,LOW);
     digitalWrite(rlInt_pin,HIGH);
   ok = false;
   }
   break;
   
   case 1:
      lcd<<"ZVS: VDC Externo";
 if(cancel){
   digitalWrite(rlExt_pin,LOW);
    cancel = false;
   }
   if(ok){
     digitalWrite(rlInt_pin,LOW);
     digitalWrite(rlExt_pin,HIGH);
   ok = false;
   }
   break;
   
   case -1:
   lcd<<"Desligar todos";
   if(ok){
     digitalWrite(rlInt_pin,LOW);
     digitalWrite(rlExt_pin,LOW);
   ok = false;
   }
   break;
}
}

void showBat(){
 lcd<<"Bateria:"<<readBat();
}
  
float readBat(){
  float voltage;
  voltage =  mapfloat(analogRead(batPin),0,1023,0,8.4f);
  return voltage*1.7403f;
    
}
void rgbLedRainbow(int numRGBLeds, int delayVal, int numCycles, int rainbowWidth){
  // Displays a rainbow spread over a few LED's (numRGBLeds), which shifts in hue. 
  // The rainbow can be wider then the real number of LED's.

  ShiftPWM.SetAll(0);
  for(int cycle=0;cycle<numCycles;cycle++){ // loop through the hue shift a number of times (numCycles)
    for(int colorshift=0;colorshift<360;colorshift++){ // Shift over full color range (like the hue slider in photoshop)
      for(int led=0;led<numRGBLeds;led++){ // loop over all LED's
        int hue = ((led)*360/(rainbowWidth-1)+colorshift)%360; // Set hue from 0 to 360 from first to last led and shift the hue
        ShiftPWM.SetHSV(led, hue, 255, 255); // write the HSV values, with saturation and value at maximum
      }
      delay(delayVal); // this delay value determines the speed of hue shift
    } 
  }  
}
int read_nunchuck(char data){
  // função pede o dado desejado char r(roll),p(pitch),x(joyx),y(joyy),z(botãoz),c(botãoc)
  // retorna int do valor dos sensores ou 0 ou 1 para os botões
    delay(20);
  chuck.update(); 
  int out;
// pitch(inclinação) e roll(rotação) são dados mais importantes e precisos
 switch(data){
/* case 'r':
  out =chuck.readRoll();
  return out;
    break;
    case'p':
    out =chuck.readPitch();
    return out;
    break;
    */
  case'x':
     out=chuck.readJoyX();
    return out;
    break;
    case 'y':
  out =chuck.readJoyY();
   return out;  
   break;
   case 'z':
  if (chuck.buttonZ) {
     out =1;
  } else  {
     out=0;
  }
  return out;
    break;
  case'c':
  if (chuck.buttonC) {
     out=1;
  } else  {
    out=0;
    }
    return out;
 break;
 }
}

  void readserialbt(){
//  Esta função espera 4 campos de dados numéricos separados por vírgula,seguidos de uma letra ou outro caractere diferente de número,vírgula ou "-"
// valuesbt_a[x] tem os últimos valores valuesbt[x] tem os valores anteriores
if( Serial2.available())
{
  ch = Serial2.read();
//
chbt = ch; // lê um caractere diferente de dígito ou vírgula
if (chbt != '0'&& chbt != '-'){
switch (chbt){
  // ler a partir dos valores _a
  /*
  case 'b':
  Serial.print("1,");
   Serial.print(read_nunchuck('c'));
  Serial.print(",");
  Serial.print(read_nunchuck('z'));
  Serial.print(",");
  
  break;
  case 'j':
  // envia valores x do joystick
  Serial.print("1,");
  Serial.print(read_nunchuck('x'));
  Serial.print(",");
  Serial.print(read_nunchuck('y'));
  Serial.print(",");
 
  break;
  case 'p':
  // valores pitch
   Serial.print("1,");
  Serial.print(read_nunchuck('p'));
  Serial.print(",");
  Serial.print(read_nunchuck('r'));
  Serial.print(",");
  
  break; */
  case 'a':
  Serial.println("teste com sucesso");
  
  break;
  case 't':
   Serial.println("t");
   Serial.print(valuesbt_a[0]);
   Serial.print(",");
   Serial.print(valuesbt_a[1]);
   Serial.print(",");
   Serial.print(valuesbt_a[2]);
   Serial.print(",");
    Serial.print(valuesbt_a[3]);
   Serial.print(",");
    Serial.println();
   break;
   
   case 'r':
   // função para os relés
   //0,0,0,0r desliga todos; 1,1,0,0,r liga o de alimentação interna; 1,2,0,0,r liga alimentação externa
   
     if (valuesbt_a[0] == 0){
     digitalWrite(rlInt_pin,LOW);
     digitalWrite(rlExt_pin,LOW);
     }
     else{
         if (valuesbt_a[1] == 1){
     digitalWrite(rlExt_pin,LOW);
     digitalWrite(rlInt_pin,HIGH);
         }
         if (valuesbt_a[1] == 2){
     digitalWrite(rlInt_pin,LOW);
     digitalWrite(rlExt_pin,HIGH);
         }
          
       }
           
    break;
    
    case 'l':

    // leds control function
    //-1,0,0,0,l to turn all off
   // -1,x,x,x,l to set all HSV
     led_sInd = 0; // disable leds background tasks
   
    if (valuesbt_a[0] == -1&&valuesbt_a[1]==0&&valuesbt_a[2]==0&&valuesbt_a[3]==0){
      ShiftPWM.SetAll(0);
    }
    else if(valuesbt_a[0] == -1)
    ShiftPWM.SetAllHSV(min(valuesbt_a[1],360),min(valuesbt_a[2],255),min(valuesbt_a[3],255));
     
    break;
    
    case 'k':
   if (valuesbt_a[0] == 0){ 
      for (int i = 0; i < 2; i++) {
      irsend.sendRC6(RC6pwr, 20); // Philips TV power code
               Serial.println("ligar a tv");
      delay(40);   }
   }
         break;
    
    case 'i':
   // função para o infravermelho
   //0,1,2,0,i tv power  // 0,2,0,0,i +volume  // 0,-2,0,0,i -volume  //0,3,0,0,i +ch  //0,-3,0,0,i -ch
   //0,4,0,0,i mute //0,5,0,0,i source
   //primeiro dígito para escolher aparelho, 0 é tv
   // terceiro dígito indica número de repetições
  
     if (valuesbt_a[0] == 0){
       switch(valuesbt_a[1]){
         case 1:

          for(int i=0;i< valuesbt_a[2];i++){
        // irsend.sendRaw(S_pwr,68,38);
         irsend.sendRC6(RC6pwr,20);
         delay(90);
          }
          
         break;
          
         case 2:
         for(int i=0;i< valuesbt_a[2];i++){
        // irsend.sendRaw(S_vup,68,38);
           irsend.sendRC6(RC6volUp,20);
          Serial.println("vup");
         delay(90);
         }
        
         break;
         
         case -2:
         for(int i=0;i< valuesbt_a[2];i++){
       //  irsend.sendRaw(S_vdown,68,38);
          irsend.sendRC6(RC6volDw,20);
         Serial.println("vdown");
         delay(90);
         }
         
         break;
         
         case 3:
         for(int i=0;i< valuesbt_a[2];i++){
        // irsend.sendRaw(S_cup,68,38);
         irsend.sendRC6(RC6chUp,20);
         Serial.println("cup");
         delay(90);
         }
         
         break;
         
          case -3:
         for(int i=0;i< valuesbt_a[2];i++){
       //  irsend.sendRaw(S_cdown,68,38);
          irsend.sendRC6(RC6chDw,20);
         Serial.println("cdown");
         delay(90);
         }
         
         break;
         
         case 4:
        // irsend.sendRaw(S_mute,68,38);
        irsend.sendRC6(RC6mute,20);
          Serial.println("mute"); 
          delay(90);
         break;
         
         case 5:
       //  irsend.sendRaw(S_scr,68,38);
       irsend.sendRC6(RC6src,20);
          Serial.println("source"); 
          delay(90);
         break;
                    }
                  }
                  
               
      break;
        // fim do bloco de controle ir
        
        
        
   // deixar a linha abaixo no fim
   chbt='0';
  
}}
if(isDigit(ch)) // is this an ascii digit between 0 and 9?
{
// yes, accumulate the value if the fieldIndex is within range
// additional fields are not stored

if(fieldindexbt < fields) {
valuesbt_a[fieldindexbt] = (valuesbt_a[fieldindexbt] * 10) + (ch - '0');
}
}
else if( ch == '-'){
  sign = -1; // muda o sinal do numero para negativo
}
else if (ch == ',') // comma is our separator, so move on to the next field
{
valuesbt_a[fieldindexbt] = valuesbt_a[fieldindexbt]*sign; 
sign=1; // reinicia o valor do sinal para positivo
fieldindexbt++; // increment field index
}
else
{
valuesbt[0] = valuesbt_a[0];
valuesbt[1] = valuesbt_a[1];
valuesbt[2] = valuesbt_a[2];
valuesbt[3] = valuesbt_a[3];
for(int i=0; i < min(fields, fieldindexbt+1); i++)
{
//Serial.println(valuesbt[i]);
valuesbt_a[i] = 0; // set the values to zero, ready for the next message
}
fieldindexbt = 0; // ready to start over
}
// funções para os dados recebidos
}
}

