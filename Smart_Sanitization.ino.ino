#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// LCD
#define LCD_ADDR 0x27
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// Pins
#define IR_PIN 34          // IR sensor output
#define RELAY_PIN 13       // Relay indicator
#define BUZZER_PIN 15      // ✅ Buzzer pin

// Keypad setup
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'*','0','#'},
  {'7','8','9'},
  {'4','5','6'},
  {'1','2','3'}
};
byte rowPins[ROWS] = {26, 25, 33, 32}; 
byte colPins[COLS] = {27, 14, 12};     
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Config
#define ESCALATION_MS 60000   // 1 min for demo
const String CLEANER_PIN = "7778";  // PIN for cleaner

// State
bool pending = false;
unsigned long usageStart = 0;
bool escalated = false;
bool occupied = false;
String pinBuffer = "";

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(IR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);   // ✅ Setup buzzer
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print("Sanitation Mon");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("System Ready");
}

void loop() {
  int ir = digitalRead(IR_PIN);
  unsigned long now = millis();

  // Detect usage (enter + leave)
  if (ir == HIGH && !occupied) {
    occupied = true;
    Serial.println("Occupied detected");
  } else if (ir == LOW && occupied) {
    occupied = false;
    Serial.println("Usage complete");
    markUsage();
  }

  // Handle keypad input
  handleKeypad();

  // Escalation check
  if (pending && !escalated && (now - usageStart >= ESCALATION_MS)) {
    escalate();
  }

  delay(100);
}

void handleKeypad() {
  char key = keypad.getKey();
  if (!key) return;

  Serial.print("Key: "); Serial.println(key);

  if (key == '*') {
    pinBuffer = "";
    lcd.clear(); lcd.setCursor(0,0); lcd.print("PIN cleared");
    delay(500);
    updateLCD();
  }
  else if (key == '#') {   // ✅ Use # as OK button
    if (pinBuffer == CLEANER_PIN) {
      lcd.clear(); lcd.setCursor(0,0); lcd.print("PIN OK");
      clearPending();
    } else {
      lcd.clear(); lcd.setCursor(0,0); lcd.print("Wrong PIN");
      delay(800);
      updateLCD();
    }
    pinBuffer = "";
  }
  else {
    pinBuffer += key;
    lcd.clear(); lcd.setCursor(0,0); lcd.print("Enter PIN:");
    lcd.setCursor(0,1);
    lcd.print(pinBuffer);   // Show real digits now
  }
}

void markUsage() {
  pending = true;
  usageStart = millis();
  escalated = false;
  digitalWrite(RELAY_PIN, HIGH); // indicator ON
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Needs Cleaning");
}

void clearPending() {
  pending = false;
  escalated = false;
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);  // ✅ stop buzzer if running
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Cleaned!");
  delay(1000);
  updateLCD();
}

void escalate() {
  escalated = true;
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Manager Alert!");
  Serial.println("⚠ Escalation triggered");

  // Blink relay + buzzer as alert
  for (int i = 0; i < 5; i++) {
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300);
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(300);
  }

  // Keep buzzer ON after blinking
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
}

void updateLCD() {
  if (!pending) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("System Ready");
  } else {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Needs Cleaning");
  }
}