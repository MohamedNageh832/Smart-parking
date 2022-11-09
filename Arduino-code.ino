/*
  NOTE:-
  - Parking slots IR sensors are to be connected through pin(4 - 7),
  if any is damaged connect its pin to ground
*/
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo parking_door;

// Pins vars
const byte door_IR_pin1 = 2;
const byte door_IR_pin2 = 3;

// Slots vars
byte slots = 0;
byte available_slots = 0;
int slots_flags[4] = {0};
long int slots_check_in[4] = {0};
int free_slots[4] = {1, 2, 3, 4};

// Price calculation vars
const int price_per_min = 20;
int checkout_time_mins[4] = {0};
int checkout_time_secs[4] = {0};
float total_prices[4] = {0};

// Door control vars
int flag1 = 0;
int flag2 = 0;
long int door_opened_since = 0;

void setup() {
  // Lcd set up
  lcd.init();
  lcd.backlight();
  lcd.blink();

  // Setting up message
  lcd.print("Smart parking");
  lcd.setCursor(0, 10);
  lcd.print("project");
  delay(1500);

  lcd.clear();
  lcd.print("Setting up...");
  
  // Pins setup
  for (int i = 2; i < 8 ; i++) {
    pinMode(i, INPUT);
  }

  lcd.setCursor(0, 1);
  lcd.print("Slots: ");

  // Detecting available slots
  for (int i = 4; i <8; i++) {
    if (digitalRead(i) == HIGH) {
      slots++;

    lcd.setCursor(8, 1);
    lcd.print(slots);
    delay(500);
    }  
  }

  lcd.noBlink();
  delay(2000);

  available_slots = slots;
  // Configuring servo
  parking_door.attach(8);

  checkSlotsStatus();
  showBasicMessage();
}

void loop() {
  // Controlling the IR sensor outside the parking door
  if (digitalRead(door_IR_pin1) == LOW && flag1 == 0) {
    if (flag2 == 1) {
      closeParkingDoor();
      showPrice();
      delay(2000);

      flag2 = 0;
      available_slots++;
      showBasicMessage();
    } else {
      if (available_slots > 0) {
        flag1 = 1;
        showFreeSlots();
        delay(2000);
        
        openParkingDoor();
        door_opened_since = millis();
      } else {
          lcd.clear();
          lcd.print("Sorry");
          lcd.setCursor(0, 1);
          lcd.print("Parking full!");
    
          delay(2000);
          flag1 = 0;
          showBasicMessage();
      }
    }  
  }   

  // Controlling the IR sensor inside the parking door
  if (digitalRead(door_IR_pin2) == LOW && flag2 == 0) {
    flag2 = 1;
    
    if (flag1 == 0) {
      if (available_slots < 4) {
        openParkingDoor();
        door_opened_since = millis();
      }
    } else if (flag1 == 1) {
      closeParkingDoor();
      delay(1500);
      available_slots--;
      showBasicMessage();
    }
  }

  if (millis() - door_opened_since > 6500 && millis() - door_opened_since < 7000) { 
    closeParkingDoor();
    showBasicMessage();
    
    flag1 = 0;
    flag2 = 0;
  }

  if (flag1 == 1 && flag2 == 1) {flag1 = 0; flag2 = 0;}

  checkSlotsStatus();
}

// welcome message and price
void showBasicMessage() {
  lcd.clear();
  lcd.blink();
  lcd.print("welcome");
  lcd.setCursor(0, 1);
  lcd.print("price: "); 
  lcd.print(price_per_min);
  lcd.print("LE/min");
  lcd.noBlink();
}

// Show which slots are free for clients on entry
void showFreeSlots() {
  lcd.clear();
  lcd.print("free slots: ");
  lcd.setCursor(0, 1);

  for (int i = 0; i < 4; i++) {
    if (free_slots[i] != -1) {
      lcd.print(free_slots[i]); 
      if (i < 3) {lcd.print(", ");};
    }
  }

  delay(2000);
  lcd.clear();
  lcd.print("Happy Parking:-)");
}

// Parking door controls
void openParkingDoor() {
  digitalWrite(9, HIGH);
  
  for (int i = 0; i <= 90; i++) {
    parking_door.write(i);
    delay(5);
  }
  
  delay(1000);
  digitalWrite(9, LOW);
}

void closeParkingDoor() {
  for (int i = 90; i >= 0; i--) {
    parking_door.write(i);
    delay(5);  
  }

  door_opened_since = 0;
}

// Calculating total parking time and price
void checkSlotsStatus() {
  for (int i = 0; i < slots; i++) {
    if (digitalRead(i + 4) == LOW && slots_flags[i] == 0) {
      slots_flags[i] = 1;
      byte index = searchArray(free_slots, i + 1);
  
      if (index != -1) {
        free_slots[i] = -1;
        slots_check_in[i] = millis();
      }
    } 
    
    if (digitalRead(i + 4) == HIGH && slots_check_in[i] != 0) {
      if (available_slots == 0) {
        lcd.clear();
        lcd.print("Car in slot ");
        lcd.print(1);
        lcd.setCursor(0, 1);
        lcd.print("is leaving..."); 
      }
      
      // Calculating parking price
      int time_now = millis();
      int total_time = time_now - slots_check_in[i];
  
      for (int j = 0; j < 4; j++) {
        if (total_prices[j] == 0) {
          checkout_time_mins[j] = total_time / 1000 / 60;
          checkout_time_secs[j] = (total_time / 1000) % 60;
          total_prices[j] = (float) total_time / 1000 / 60 * price_per_min;
          break;
        }
      }

      slots_flags[i] = 0;
      slots_check_in[i] = 0;
      free_slots[i] = i + 1;
    }  
  }
}

void showPrice() {
  lcd.clear();
  
  if (total_prices[0] != 0) {
    char time_spent[20];
    sprintf(time_spent, "Time: %dm %ds", checkout_time_mins[0], checkout_time_secs[0]);
    lcd.print(time_spent);
  
    lcd.setCursor(0, 1);
    lcd.print("price: ");
    lcd.print(total_prices[0]);
    lcd.print("LE");

    checkout_time_mins[0] = 0;
    checkout_time_secs[0] = 0;
    total_prices[0] = 0;
    
    for (int i = 0; i < 4; i++) {
      if (total_prices[i] == 0 && total_prices[i + 1] != 0) {
        int temp = total_prices[i];
        total_prices[i] = total_prices[i + 1];
        total_prices[i + 1] = temp;
      }
    }
    delay(3000);
  }
  
  
  lcd.clear();
  lcd.print("Thank you");
  lcd.setCursor(0, 1);
  lcd.print("GoodBye");
}

int searchArray(int arr[], int n) {
  for (int j = 0; j < sizeof(arr); j++) {
    if (arr[j] == n) {  
      return j;
    }
  }
  return -1;
}
