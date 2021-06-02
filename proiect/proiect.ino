#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Create an LCD object. Parameters: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd = LiquidCrystal(2, 3, 4, 5, 6, 7);

// Used for the temperature sensors
const int ONE_WIRE_BUS = 13;
OneWire one_wire(ONE_WIRE_BUS);
DallasTemperature sensors(&one_wire);

float celsius_water = 0;
float fahrenheit_water = 0;
float celsius_room = 0;

// LED pins
const int RED_PIN = A1;
const int GREEN_PIN = A2;
const int BLUE_PIN = A3;

// Potentiometer pin
const int POTENT_PIN = A0;

// Lower and upper temperature bounds
float base_temp = 45.0f;
float interval = 5.0f;
float grade_per_val = 30.0f / 1024.0f;
 
// Button functionality variables
int is_on = false;
int is_temp = false;
int is_set = false;
int is_probe = false;

// Constants used for buttons pins
const int ON_BUTTON_PIN = 8;
const int TEMP_BUTTON_PIN = 9;
const int SET_BUTTON_PIN = 10;
const int PROBE_BUTTON_PIN = 11;
const int DEBOUNCE_DELAY = 50;

// Variables used for debouncing the buttons
int last_steady_on = HIGH;
int last_flicker_on = LOW;
int last_steady_temp = HIGH;
int last_flicker_temp = LOW;
int last_steady_set = HIGH;
int last_flicker_set = LOW;
int last_steady_probe = HIGH;
int last_flicker_probe = LOW;

unsigned long last_debounce_on = 0;
unsigned long last_debounce_temp = 0;
unsigned long last_debounce_set = 0;
unsigned long last_debounce_probe = 0;

// Used for probing the temperature
int was_probed = false;
unsigned long last_probe = 0;
const unsigned long BETWEEN_PROBES = 10000;
const unsigned long PROBE_DURATION = 2000;
float first_temp = 0;
float second_temp = 0;
float time_taken = 0;

// Send signals to color the RGB
void RGB_color(int red_light_value, int green_light_value, int blue_light_value) {
  analogWrite(RED_PIN, red_light_value);
  analogWrite(GREEN_PIN, green_light_value);
  analogWrite(BLUE_PIN, blue_light_value);
}

// Used to check if the state of a button has changed
void check_button(const int PIN, int *last_flicker, int *last_steady,
                      unsigned long *last_debounce, int *is_bool) {
  int current_state = digitalRead(PIN);
  if (current_state != *last_flicker) {
    *last_debounce = millis();
    *last_flicker = current_state;
  }

  if (millis() - *last_debounce > DEBOUNCE_DELAY) {
    if (*last_steady == HIGH && current_state == LOW) {
      *is_bool = ((*is_bool) ^ true);
      lcd.clear();
    }

    *last_steady = current_state;
  }
}

// Instruction set for when the system is off
void compute_off() {
    is_temp = false;
    is_set = false;
    is_probe = false;
    was_probed = false;

    RGB_color(255, 255, 255); // None

    lcd.setCursor(0, 0);
    lcd.print("Press the button");
    lcd.setCursor(1, 1);
    lcd.print("to turn it on!");
}

// Instruction set for when the system is on standby
void compute_standby() {
    RGB_color(0, 0, 0); // White
    lcd.setCursor(0, 0);
    lcd.print("1-On/Off, 2-Temp");
    lcd.setCursor(0, 1);
    lcd.print("3-Set, 4-Predict");  
}

// Instruction set for when the system displays the temperature
void compute_temp() {
    is_set = false;
    is_probe = false;
    was_probed = false;

    // Get the temperature of the beverage
    sensors.requestTemperatures();
    celsius_water = sensors.getTempCByIndex(0);
    fahrenheit_water = sensors.toFahrenheit(celsius_water);

    int val = analogRead(POTENT_PIN);
    float curr_temp = base_temp + val * grade_per_val;

    // Display a color based on how hot the beverage is
    if (celsius_water < curr_temp - interval) {
      RGB_color(255, 255, 0); // Blue
    } else if (celsius_water >= curr_temp - interval && celsius_water <= curr_temp + interval) {
      RGB_color(255, 0, 255); // Green
    } else {
      RGB_color(0, 255, 255); // Red
    }

    // Print corresponding temperatures
    lcd.setCursor(0, 0);
    lcd.print("Celsius: ");
    lcd.setCursor(9, 0);
    lcd.print(celsius_water);

    lcd.setCursor(0, 1);
    lcd.print("Fahr: ");
    lcd.print(fahrenheit_water);
}

// Instruction set for when the system is in set mode
void compute_set() {
    is_temp = false;
    is_probe = false;
    was_probed = false;

    RGB_color(127, 0, 127); // Cyan

    // Change the temperature based on the potentiometer reading
    int val = analogRead(POTENT_PIN);
    float curr_temp = base_temp + val * grade_per_val;

    // Display the lower and upper bound of the new temperature
    lcd.setCursor(0, 0);
    lcd.print("Min temp: ");
    lcd.print(curr_temp - interval);
    lcd.print("C");
    
    lcd.setCursor(0, 1);
    lcd.print("Max temp: ");
    lcd.print(curr_temp + interval);
    lcd.print("C");
}

// Function used to get the cooling time, based on Newton's Law of Cooling
float get_time() {
  float newton_cooling = log((second_temp - celsius_room) / (first_temp - celsius_room)) * 12;
  return log((base_temp - celsius_room) / (first_temp - celsius_room)) / newton_cooling; 
}

// Instruction set for when the system is in probe mode
void compute_probe(){
    is_temp = false;
    is_set = false;

    if (!was_probed) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Probing...");
    
        lcd.setCursor(0, 1);
        lcd.print("Please wait.");

        // Read the reference temperatures of the beverage and the room
        sensors.requestTemperatures();
        celsius_room = sensors.getTempCByIndex(1);
    
        sensors.requestTemperatures();
        first_temp = sensors.getTempCByIndex(0);
        delay(PROBE_DURATION);
        
        sensors.requestTemperatures();
        second_temp = sensors.getTempCByIndex(0);

        // Print the computed time and the room temperature
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Cool time: ");
        lcd.print(get_time());
        lcd.setCursor(0, 1);
        lcd.print("Room temp: ");
        lcd.print(celsius_room);

        was_probed = true;
    }
}

// Initializes the pins, the sensors and the LCD
void setup(void)
{
    sensors.begin();
    lcd.begin(16, 2);
  
    pinMode(ON_BUTTON_PIN, INPUT_PULLUP);
    pinMode(TEMP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SET_BUTTON_PIN, INPUT_PULLUP);
    pinMode(PROBE_BUTTON_PIN, INPUT_PULLUP);
  
    pinMode(POTENT_PIN, OUTPUT);
  
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
}

// Waits for change in buttons status and goes into the given mode
void loop(void)
{
    check_button(ON_BUTTON_PIN, &last_flicker_on, &last_steady_on, &last_debounce_on, &is_on);
  
    if (!is_on) {
        compute_off();
    } else {
        check_button(TEMP_BUTTON_PIN, &last_flicker_temp, &last_steady_temp, &last_debounce_temp, &is_temp);
        check_button(SET_BUTTON_PIN, &last_flicker_set, &last_steady_set, &last_debounce_set, &is_set);
        check_button(PROBE_BUTTON_PIN, &last_flicker_probe, &last_steady_probe, &last_debounce_probe, &is_probe);
  
      if (is_temp) {
        compute_temp();
      } else if (is_set) {
        compute_set();
      } else if (is_probe) {
        compute_probe();
      } else {
        compute_standby();
      }
    }
}
