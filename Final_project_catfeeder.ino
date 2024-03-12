#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LCD_UPDATE_INTERVAL 300
#define THRESHOLD 100
#define ROTATE_TIME 1000
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define OK 'B'
#define UP 'A'
#define DOWN 'C'
char key;
bool soundSensorMode = true;
unsigned long lt1;
const int soundSensorPin = A0;
const int buttonPin = 2;
unsigned long lu;
const byte ROWS = 4;
const byte COLS = 4;
unsigned long lr;

const int servoPin = 9;  // so chan cho tin hieu dieu khien servo
// pulse width cho dong co servo voi don vi  microseconds
volatile int pulseWidth = 1500;
const char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
char rowPins[ROWS] = { 3, 4, 5, 6 };
char colPins[COLS] = { 10, 11, 8, 7 };

char getKey() {
  char k = 0;

  for (char c = 0; c < COLS; c++) {
    digitalWrite(colPins[c], LOW);
    for (char r = 0; r < ROWS; r++) {
      //
      if (digitalRead(rowPins[r]) == LOW) {
        while (digitalRead(rowPins[r]) == LOW)
          ;
        k = keys[r][c];
      } else {
      }
    }
    digitalWrite(colPins[c], HIGH);
  }

  return k;
}

// Khai bao bien va thiet lap gia tri ban dau
int soundValue = 0;
int buttonState = 0;
int lastButtonState = 0;
int angle = 0;
unsigned long lastFeedTime = 0;  // Khai bao bien lastFeedTime de luu thoi diem thuc an cuoi cung duoc cho

int cursor = 0;

void printMenu() {
  lcd.clear();
  lcd.setCursor(0, cursor);
  lcd.print(">");
  lcd.setCursor(1, 0);
  lcd.print("Sound Sensor");
  lcd.setCursor(1, 1);
  lcd.print("Button");
}
//write a function that check the time user hold the # button and return the time
unsigned long last_hold_time = 0;
int checkHoldTime(char key) {
  if (key == '#') {
    if (millis() - last_hold_time > 2000) {
      last_hold_time = millis();
      return 1;
    }
  } else {
    last_hold_time = millis();
    return 0;
  }
}

void menu() {
  printMenu();
  while (true) {
    key = getKey();
    if (checkHoldTime(key)) return;
    if (key == OK) {
      if (cursor == 0) {
        soundSensorMode = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sound ");
        lcd.setCursor(0, 1);
        lcd.print("activated");
      } else if (cursor == 1) {
        soundSensorMode = false;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Button mode");
        lcd.setCursor(0, 1);
        lcd.print("activated");
      }
      break;
    } else if (key == UP) {
      cursor--;
      if (cursor < 0) cursor = 1;
      printMenu();
    } else if (key == DOWN) {
      cursor++;
      if (cursor > 1) cursor = 0;
      printMenu();
    }
  }
}

ISR(TIMER1_COMPA_vect) {
  digitalWrite(servoPin, HIGH);           // set the servo control signal high
  delayMicroseconds(pulseWidth);          // wait for the pulse width duration
  digitalWrite(servoPin, LOW);            // set the servo control signal low
  delayMicroseconds(20000 - pulseWidth);  // wait for the rest of the 20ms cycle
}
// Function to control the angle of the servo
void rotateServo(int angle) {
  // góc quay
  pulseWidth = map(angle, 0, 180, 1000, 2000);
}
int last_update_value = 0;
void updateLCD() {
  int new_value = soundSensorMode ? soundValue : (int)!buttonState;
  //đoạn code phụ để cho màn hình LCD không bị hiển thị chèn nội dung lên nhau
  if (millis() - lu < LCD_UPDATE_INTERVAL || abs(new_value - last_update_value) == 0) return;
  lu = millis();
  lcd.clear();

  if (soundSensorMode) {
    lcd.setCursor(0, 0);
    lcd.print("Sound mode");
    lcd.setCursor(0, 1);
    lcd.print("Sound: ");
    lcd.print(soundValue);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Button mode");
    lcd.setCursor(0, 1);
    lcd.print("Button ");
    lcd.print(!buttonState ? "pressed" : "released");
  }
  last_update_value = new_value;
}
void setup() {
  // Khoi tao cac chan la INPUT hoac OUTPUT
  pinMode(soundSensorPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(servoPin, OUTPUT);            // set the servo pin as an output
  TCCR1A = 0;                           // clear Timer/Counter Control Register A
  TCCR1B = 0;                           // clear Timer/Counter Control Register B
  TCNT1 = 0;                            // clear Timer/Counter Register (timer value)
  OCR1A = 31250;                        // set Output Compare Register A to generate an interrupt every 20ms
  TCCR1B |= (1 << WGM12);               // set CTC mode (Clear Timer on Compare Match)
  TCCR1B |= (1 << CS11) | (1 << CS10);  // set prescaler to 64
  TIMSK1 |= (1 << OCIE1A);              // enable Timer/Counter1 Output Compare A Match Interrupt
  // Thiet lap goc xoay ban dau cua servo motor
  rotateServo(0);
  Serial.begin(9600);
  for (char r = 0; r < ROWS; r++) {
    pinMode(rowPins[r], INPUT);      //set the row pins as input
    digitalWrite(rowPins[r], HIGH);  //turn on the pullups
  }

  for (char c = 0; c < COLS; c++) {
    pinMode(colPins[c], OUTPUT);  //set column pins as output
  }
  // Khoi tao LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Setup complete");
  delay(1000);
  lcd.clear();
}

void loop() {
  // Doc gia tri tu cam bien am thanh
  soundValue = analogRead(soundSensorPin);

  // Doc trang thai cua nut bam
  buttonState = digitalRead(buttonPin);
  key = getKey();
  Serial.println(key);

  // In thong tin len LCD
  updateLCD();
  if (key != 0) {
    if (checkHoldTime(key)) menu();
  }
  // Neu nut bam duoc nhan
  if (!soundSensorMode) {
    if (buttonState != lastButtonState) {
      if (buttonState == LOW) {
        // Xoay servo motor den goc 90 do
        angle = 180;
        rotateServo(angle);
      } else {
        // Xoay servo motor den goc 0 do
        angle = 0;
        rotateServo(angle);
      }
    }

    // Luu trang thai cua nut bam
    lastButtonState = buttonState;
  } else {
    // Neu gia tri doc tu cam bien am thanh vuot qua nguong va da qua 10 giay ke tu lan thuc an cuoi cung duoc cho
    if (soundValue > THRESHOLD && millis() - lastFeedTime > 10000) {
      // Xoay servo motor den goc 90 do
      angle = 180;
      rotateServo(angle);
      lastFeedTime = millis();
      // In thong tin len LCD
      lcd.setCursor(0, 0);
      lcd.print("Feeding...");
      //động cơ servo sau khi quay sẽ dừng lại 2s để nắp mở cho thức ăn tuôn ra
      unsigned long lt = millis();
      while (millis() - lt < 2000)
        ;

      // Xoay servo motor den goc 0 do
      angle = 0;
      rotateServo(angle);

      // In thong tin len LCD
      lcd.setCursor(0, 0);
      lcd.print("Feed done.");
      lcd.setCursor(0, 1);
      lcd.print("Sound: ");
      lcd.print(soundValue);
    }
    if (soundValue > THRESHOLD && millis() - lastFeedTime < 10000) {
      // In thong tin len LCD
      lcd.setCursor(0, 0);
      lcd.print("It's not time to eat yet");
      lcd.setCursor(0, 1);
      lcd.print("Sound: ");
      lcd.print(soundValue);
      unsigned long lt = millis();
      while (millis() - lt < 2000)
        ;
    }
  }
}