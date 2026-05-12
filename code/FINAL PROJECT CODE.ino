//DEVIN DICKSON
//FINAL GROUP 33
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(40, 41, 42, 43, 44, 45);
RTC_DS1307 rtc;
enum State {OFF, IDLE, ACTIVE, ERROR_STATE};
volatile State currentState = OFF;
#define LED_OFF_PIN PC7  
#define LED_IDLE_PIN PC6   
#define LED_ACTIVE_PIN PC5   
#define LED_ERROR_PIN PC4   
#define LED_ACTUATOR_PIN PC3
#define OFF_BUTTON_PIN PA1
#define RESET_BUTTON_PIN PA2
#define PIR_PIN PA0
#define START_PIN PD2
volatile bool startPressed = false;
unsigned long lastMotionTime = 0;
unsigned long lastUpdate = 0;
const int LIGHT_THRESHOLD = 100;

void startISR(){
  startPressed = true;
}
bool readPinA(uint8_t pin){ return PINA & (1 << pin);}
bool readPinD(uint8_t pin){ return PIND & (1 << pin);}
void writeHigh(volatile uint8_t &port, uint8_t pin){
  port |= (1 << pin);
}
void writeLow(volatile uint8_t &port, uint8_t pin){
  port &= ~(1 << pin);
}
int readADC(){
  ADMUX = (1 << REFS0); 
  ADCSRA = (1 << ADEN)|
           (1 << ADSC)|
           (1 << ADPS2)|
           (1 << ADPS1)|
           (1 << ADPS0);
  while (ADCSRA & (1 << ADSC));
  return ADC;
}
void logEvent(const char* msg){
  DateTime now = rtc.now();
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());
  Serial.print(" - ");
  Serial.println(msg);
}
void updateLEDs(){
  writeLow(PORTC, LED_OFF_PIN);
  writeLow(PORTC, LED_IDLE_PIN);
  writeLow(PORTC, LED_ACTIVE_PIN);
  writeLow(PORTC, LED_ERROR_PIN);
  if(currentState == OFF) writeHigh(PORTC, LED_OFF_PIN);
  if(currentState == IDLE) writeHigh(PORTC, LED_IDLE_PIN);
  if(currentState == ACTIVE) writeHigh(PORTC, LED_ACTIVE_PIN);
  if(currentState == ERROR_STATE) writeHigh(PORTC, LED_ERROR_PIN);
}
void actuatorOn(){ writeHigh(PORTC, LED_ACTUATOR_PIN); }
void actuatorOff(){ writeLow(PORTC, LED_ACTUATOR_PIN); }
void setup(){
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();
  lcd.begin(16,2);
  DDRC |= (1<<LED_OFF_PIN)|
          (1<<LED_IDLE_PIN)|
          (1<<LED_ACTIVE_PIN)|
          (1<<LED_ERROR_PIN)|
          (1<<LED_ACTUATOR_PIN);
  DDRA &= ~((1<<OFF_BUTTON_PIN) | (1<<RESET_BUTTON_PIN) | (1<<PIR_PIN));
  DDRD &= ~(1<<START_PIN);
  attachInterrupt(digitalPinToInterrupt(2), startISR, FALLING);
  currentState = OFF;
  logEvent("SYSTEM START");
}
void loop(){
  unsigned long now = millis();
  int light = readADC();
  int motion = readPinA(PIR_PIN);
  updateLEDs();
  if(startPressed && currentState == OFF){
    currentState = IDLE;
    logEvent("OFF -> IDLE");
    startPressed = false;
  }
  if(currentState == OFF){
    actuatorOff();
    lcd.clear();
    lcd.print("SYSTEM OFF");
  }
  else if(currentState == IDLE){
    actuatorOff();
    lcd.clear();
    lcd.print("IDLE L:");
    lcd.print(light);
    if(motion && light < LIGHT_THRESHOLD){
      currentState = ACTIVE;
      lastMotionTime = now;
      logEvent("IDLE -> ACTIVE");
    }
  }
  else if(currentState == ACTIVE){
    actuatorOn();
    lcd.clear();
    lcd.print("ACTIVE L:");
    lcd.print(light);
    if(motion){
      lastMotionTime = now;
    }
    if(now - lastMotionTime > 5000){
      currentState = IDLE;
      actuatorOff();
      logEvent("ACTIVE -> IDLE");
    }
  }
  else if(currentState == ERROR_STATE){
    actuatorOff();
    lcd.clear();
    lcd.print("ERROR");
  }
  if(!(PINA & (1<<OFF_BUTTON_PIN))){
    currentState = OFF;
    actuatorOff();
    logEvent("OFF BUTTON");
  }
  if(!(PINA & (1<<RESET_BUTTON_PIN)) && currentState == ERROR_STATE)
  {
    currentState = IDLE;
    logEvent("RESET -> IDLE");
  }
}