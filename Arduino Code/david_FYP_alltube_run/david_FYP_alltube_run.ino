/* Arduino DS18B20 temp sensor tutorial
   More info: http://www.ardumotive.com/how-to-use-the-ds18b20-temperature-sensor-en.html
   Date: 19/6/2015 // www.ardumotive.com */


//Include libraries
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>


// for LCD display 
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows

// wiring on arduino
#define ONE_WIRE_BUS 2
#define PELTIER_RELAY 3
#define PUMP_RELAY 4
#define OKAY_BUTTON 5
#define RESET_COUNTER 6
#define TEMP_CONTROL A0
#define PIN_1 12
#define PIN_2 13
#define PWM_A 11
#define IR 7
#define BUZZER 10


float mode_read;
String MODE = "Norm" ; // "Norm": normal mode, sample once over hour.  "10in1": sample 3 times every 3 hours

float temperature_set;
float range = 0.3 ;
float temp_now ;
long sampling_interval ; //3600000; MODIFIED IN select_mode() function

int   PELTIER_STATE = HIGH;
int   SAMPLE_COUNT = 0 ;
volatile int   front = HIGH;
volatile int   back = LOW;



long start ;
long timeleft ;


int button_state = 1;         // current state of the button
int last_button_state = 1;  
int OK = 0;
int reset_counter = 0;


// Variables will change:
int TUBE_COUNT = 0;   // counter for the number of button presses
// for IR sensor, "black" is logic 1, white is logic 0
int IR_state = 1;         // current state of the button
int last_IR_state = 1;     // previous state of the button
int last_flakey_state = 1;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
const int DEBOUNCE_DELAY = 10;

//duration to turn on the pump for 
int   WASTE = 3000;
int   COLLECT = 2000;
int   MOTOR_SPEED = 75 ;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

void setup(void)
{
  Serial.println("hi!");
  lcd.init(); // initialize the lcd
  lcd.backlight();

  //setup pins and variables
  Serial.begin(9600); //Begin serial communication
  Serial.println("hi!");
  sensors.begin();
  pinMode(PELTIER_RELAY, OUTPUT) ;
  pinMode(PUMP_RELAY, OUTPUT) ;
  pinMode(TEMP_CONTROL , INPUT ); 
  pinMode(PIN_1, OUTPUT);
  pinMode(PIN_2, OUTPUT);
  pinMode(PWM_A,OUTPUT);
  pinMode(IR, INPUT);
  pinMode(BUZZER, OUTPUT);

  //ensure everything is turned off before we begin
  digitalWrite(PELTIER_RELAY , PELTIER_STATE);
  digitalWrite( PUMP_RELAY , HIGH); 
  analogWrite( BUZZER, 0);
  
  lcd.setCursor(0,0);
  lcd.print("David's FYP Proj ");
  lcd.setCursor(2,1);
  lcd.print("TUBE POS TEST");
  delay(3000);

  //choose between Norm and 10in1 using the knob  (turn left for Norm, Right for 3 in 3 ) and hit "ok" to enter the input
  select_mode();
  
  // User will choose temperature, the system will cool down to selected 
  setup_temp() ;  
  Serial.println("Set up finish!"); 
  
  // wait for user to press "ok" to kick start the whole automation process
  wait_input();
  Serial.println("Set up finish!");
  //start the counting! 
  start = millis();
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("Experiment");
  lcd.setCursor(5,1);
  lcd.print("begins!");
  delay(2000);
  for (int scroller = 0 ; scroller<17 ; scroller ++){
    lcd.scrollDisplayLeft();
    delay(50);}
  lcd.clear();

  // All set up complete , proceeed to main process loop after this.
  
  

}


void loop(void)
{ 
  
  // Loop Process 1: check the temperature and decide whether to activate peltier chip or not
  regulate_temp();
  
  //
  timeleft = sampling_interval - millis() + start;

  lcd.setCursor(0,1);
  lcd.print("Next:");
  lcd.setCursor(5,1);
  if (timeleft/60000<10){lcd.print(String(" ")+String(int(timeleft/60000))+ String("M"));}
  else{lcd.print(String(int(timeleft/60000) ) + String("M"));}
  lcd.setCursor(10,1);
  lcd.print("Cnt:");
  lcd.setCursor(14,1);    
  if (SAMPLE_COUNT<10){lcd.print(String(" ")+String(int(SAMPLE_COUNT)));}
  else{lcd.print(String(int(SAMPLE_COUNT)));}  

  
  lcd.setCursor(0,0);
  lcd.print("Now:");
  lcd.setCursor(4,0);
  if (temp_now<10){lcd.print(String(" ")+String(int(temp_now))+ String("C"));}
  else{lcd.print(String(int(temp_now))+ String("C"));}
  lcd.setCursor(9,0);
  lcd.print("Set:");
  lcd.setCursor(13,0);    
  if (temperature_set<10){lcd.print(String(" ")+String(int(temperature_set))+ String("C"));}
  else{lcd.print(String(int(temperature_set)) + String("C"));}  
  delay(1000);

  if (digitalRead(RESET_COUNTER)==LOW ){
    reset_counter++;
    if (reset_counter>2){
      SAMPLE_COUNT=0;
      reset_counter =0;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Counter Reset!");
      buzz(1000);
      
      delay(3000);
      lcd.clear();
      Serial.println("counter reset!");}
    }
    
  else{
    reset_counter =0;
    }

  if ((timeleft<1)&& (SAMPLE_COUNT <10) ){    
    if ((MODE == "Norm" || (MODE=="10in1" && SAMPLE_COUNT<9))){
      Serial.println("FETCH");// call function
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Performing");
      lcd.setCursor(7,1);
      lcd.print("Sampling!");
      SAMPLE_COUNT= SAMPLE_COUNT + 1;
      start= millis();
      fetch();
      lcd.clear();
    }
  }
  //update lcd
  // check if should be counting??? with a switch + pull up / down resistor  : https://circuitdigest.com/tutorial/pull-up-and-pull-down-resistor
  //if it is off then i 

  
  // check if 1 hour is up and sample count below 10
    // if it is then
        // update LCD to  "sampling..."
        // perform sampling 
        
}



void select_mode()
{
  OK = false; 
  lcd.clear();
  while (OK == false ){
    mode_read = round( analogRead(TEMP_CONTROL)* (5.0 / 1023.0));
    lcd.setCursor(0, 0);         // move cursor to   (0, 0)
    
    if (mode_read <2.5){    
      MODE = "Norm";
      sampling_interval = 20000; //3600000; 
}
    else{    
      MODE = "10in1";
      sampling_interval = 10000; //3600000; 
      }    
    lcd.print(String("Select Mode:"+ MODE));   
    lcd.setCursor(0, 1);         // move cursor to   (2, 1)
    lcd.print("Hit OK when rdy!"); // print message at (2, 1)
  
    button_state = digitalRead(OKAY_BUTTON);    
    // compare the buttonState to its previous state
    
    if ( (button_state != last_button_state)&& (button_state == LOW)) {
      OK = true;
      buzz(100);
    }
    last_button_state = button_state; 
  }

  lcd.clear();
  lcd.setCursor(0, 0);  
  lcd.print(String("-") + MODE + String("- selected!") );
  lcd.print(String("Choose temp next") );
  delay(3000);
  lcd.clear();

}

void setup_temp()
{
  OK = false; 
  lcd.clear();
  while (OK == false ){
    temperature_set = round( 5* analogRead(TEMP_CONTROL)* (5.0 / 1023.0));
    lcd.setCursor(0, 0);         // move cursor to   (0, 0)
    
    if (temperature_set <10){    lcd.print(String("Select Temp: ") + String(int(temperature_set)) + String(".C"));   }
    else{    lcd.print(String("Select Temp:") + String(int(temperature_set)) + String(".C"));   }     // print message at (0, 0)}// print message at (0, 0)}

    lcd.setCursor(0, 1);         // move cursor to   (2, 1)
    lcd.print("Hit OK when rdy!"); // print message at (2, 1)
  
    button_state = digitalRead(OKAY_BUTTON);    
    // compare the buttonState to its previous state
    
    if ( (button_state != last_button_state)&& (button_state == LOW)) {
      OK = true;
      buzz(100);
    }
    last_button_state = button_state; 
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Cooling the box!");
  delay(1000);
  sensors.requestTemperatures();  
  temp_now = sensors.getTempCByIndex(0);
  while ( temp_now  -15 >= temperature_set  ){ // REMOVE THE -15 WHEN CHECKING
  
    sensors.requestTemperatures();  
    temp_now = sensors.getTempCByIndex(0);
    // temperature_set = round( 5* analogRead(TEMP_CONTROL)* (5.0 / 1023.0));       
    PELTIER_STATE = LOW; 
    digitalWrite(PELTIER_RELAY, PELTIER_STATE);
    lcd.setCursor(0,1);
    lcd.print("Now:");
    lcd.setCursor(4,1);
  
    if (temp_now<10){lcd.print(String(" ")+String(int(temp_now))+ String("C"));}
    else{lcd.print(String(int(temp_now ) ) + String("C"));}
    lcd.setCursor(9,1);
    lcd.print("Set:");
    lcd.setCursor(13,1);    
    if (temperature_set<10){lcd.print(String(" ")+String(int(temperature_set))+ String("C"));}
    else{lcd.print(String(int(temperature_set)) + String("C"));}  
    delay(1000);
  }
}


// updates set temperature, turn on or off arduino if necessary
void regulate_temp()
{
  //During set up , read for temperature set if there's a change from knob
 //temperature_set =  round(5*analogRead(TEMP_CONTROL)* (5.0 / 1023.0)) ; //read value from potentiometer , 
  sensors.requestTemperatures();  
  temp_now = sensors.getTempCByIndex(0);
  //if too hot nad the peltier is off
  if (( temp_now - temperature_set >= range) && ( PELTIER_STATE == HIGH )){
    PELTIER_STATE = LOW;
  }; 
  // if cold enough 
  if (( temperature_set  - temp_now >= range ) && ( PELTIER_STATE == LOW )){
    PELTIER_STATE = HIGH;
  };
    digitalWrite( PELTIER_RELAY , PELTIER_STATE );
}

void wait_input(){
  OK = false; 
  button_state = HIGH;
  last_button_state =HIGH;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("All systems go!");
  lcd.setCursor(0,1);  
  lcd.print("Hit OK when rdy");
  while (OK == false ){
    regulate_temp();
    button_state = digitalRead(OKAY_BUTTON);    
    // compare the buttonState to its previous state
    
    if ( (button_state != last_button_state)&& (button_state == LOW)) {
      OK = true;
      buzz(100);
    }
    last_button_state = button_state; 
  }
  }

void fetch()
{
  // position zero at waste always
  
  // turn on relay to waste 
  digitalWrite(PUMP_RELAY ,LOW);
  delay(WASTE); 
  // cut off relay
  digitalWrite(PUMP_RELAY, HIGH);
  
  move(front, SAMPLE_COUNT );
  delay(1000);  
  digitalWrite(PUMP_RELAY ,LOW);    
  delay(COLLECT);
  digitalWrite(PUMP_RELAY, HIGH);
  delay(1000);
  
 if ((MODE == "10in1" ) && (SAMPLE_COUNT < 10)){
  for (int scroller = 0 ; scroller<9 ; scroller ++){
    move(front, 1 );
    delay(1000);  
    digitalWrite(PUMP_RELAY ,LOW);    
    delay(COLLECT);
    digitalWrite(PUMP_RELAY, HIGH);
    delay(1000);
    }
  SAMPLE_COUNT  +=9;
  }

  move(back, SAMPLE_COUNT);
  delay(2000);  

}

void move(int directionz , int steps){
  //set direction
  if (directionz == front){
    digitalWrite(PIN_1, LOW);
    digitalWrite(PIN_2, HIGH);    
    }
  else {  
    digitalWrite(PIN_1, HIGH);
    digitalWrite(PIN_2, LOW);  }
  
  //reset tubecount
  TUBE_COUNT = 0;
  last_IR_state = digitalRead(IR);
    if (last_IR_state== LOW){
    TUBE_COUNT--;
    } 

  //kickstart the motor
  Serial.println("Moving Motor!");
  Serial.print("TUBE_COUNT: ");
  Serial.println(TUBE_COUNT);
  analogWrite(PWM_A, 70);
  
  while ( steps > TUBE_COUNT ){
    IR_state = digitalRead(IR);
    if (IR_state != last_IR_state){
      if (IR_state== HIGH){
        TUBE_COUNT ++;
        Serial.print("ON :");
        Serial.println(TUBE_COUNT);
        }
       else{Serial.print("OFF");}
      delay(30);
    last_IR_state = IR_state; 
    analogWrite(PWM_A,70);
    }
  }
  //cut immediately once SAMPLE_COUNT = TUBE_COUNT
  digitalWrite(PIN_1, LOW);
  digitalWrite(PIN_2, LOW);
   analogWrite(PWM_A,0);
  
  
  }

  void buzz(int duration)
  {analogWrite(BUZZER, 200);
  delay(duration);
  analogWrite(BUZZER,0);
    }
