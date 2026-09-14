#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>

// ================= تعريف المنافذ (Pins) =================
#define WHITE_LED 4
#define GREEN_LED 2
#define RED_LED 15
#define BUZZER_PIN 17  // المنفذ TX2 يعادل الرقم 17 برمجياً

// منافذ قارئ البطاقات
#define SS_PIN 5
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);

// ================= إعدادات لوحة المفاتيح =================
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
// الأسلاك الثمانية الموصولة باللوحة
byte rowPins[ROWS] = {13, 14, 27, 26};
byte colPins[COLS] = {25, 33, 32, 21};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ================= إعدادات كلمات المرور =================
String correctPassword = "1234"; // الرقم السري المطلوب إدخاله (يمكنك تغييره)
String inputPassword = "";

// ضع هنا كود البطاقة الصحيح بعد معرفته من الـ Serial Monitor
String correctRFID = "23917D29";

unsigned long lastActivityTime = 0;
const unsigned long IDLE_TIMEOUT = 10000; // مسح الإدخال بعد 10 ثواني بدون ضغط مفتاح

// ================= تصحيح ترتيب اللوحة (الصفوف/الأعمدة معكوسة بالتوصيل) =================
char translateKey(char k) {
  switch (k) {
    case 'D': return '1';
    case 'C': return '2';
    case 'B': return '3';
    case '#': return '4';
    case '9': return '5';
    case '3': return 'B';
    case '0': return '7';
    case '5': return '9';
    case '2': return 'C';
    case '7': return '0';
    case '4': return '#';
    case '1': return 'D';
    default:  return k; // A و 6 و 8 و * ترجع لنفسها (ما تتأثر)
  }
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(WHITE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  keypad.setDebounceTime(50); // يقلل تسجيل نفس الضغطة مرتين بسبب النطّة (bounce)

  digitalWrite(WHITE_LED, HIGH); // النظام جاهز دائمًا، بدون زر تفعيل
  Serial.println("النظام جاهز. أدخل الرمز أو مرر البطاقة...");
}

void loop() {
  // مسح الرمز الجزئي المُدخل إذا مرّ وقت طويل بدون ضغط مفتاح
  if (inputPassword.length() > 0 && millis() - lastActivityTime > IDLE_TIMEOUT) {
    inputPassword = "";
    Serial.println("تم مسح الإدخال (انتهت المهلة)");
  }

  char key = keypad.getKey();
  if (key) {
    key = translateKey(key); // تصحيح المفتاح قبل استخدامه
    lastActivityTime = millis();
    tone(BUZZER_PIN, 1500, 50);

    if (key == '#') {
      Serial.println("\n[تشخيص] الرمز المُدخل فعليًا: " + inputPassword);
      checkPassword(inputPassword);
    }
    else if (key == '*') {
      inputPassword = "";
      Serial.println("تم مسح الإدخال");
    }
    else {
      inputPassword += key;
      Serial.print(key); // اطبع المفتاح نفسه بدل * للتشخيص المؤقت
    }
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String cardID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      cardID += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      cardID += String(rfid.uid.uidByte[i], HEX);
    }
    cardID.toUpperCase();
    Serial.println("\nتمت قراءة بطاقة: " + cardID);

    checkCard(cardID);

    rfid.PICC_HaltA();
  }
}

void checkPassword(String pass) {
  if (pass == correctPassword) {
    accessGranted();
  } else {
    accessDenied();
  }
}

void checkCard(String card) {
  if (card == correctRFID) {
    accessGranted();
  } else {
    accessDenied();
  }
}

void accessGranted() {
  Serial.println("\nتم فتح الباب!");
  digitalWrite(WHITE_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);

  tone(BUZZER_PIN, 2000, 150); delay(150);
  tone(BUZZER_PIN, 2500, 150); delay(150);
  tone(BUZZER_PIN, 3000, 300);

  delay(3000);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(WHITE_LED, HIGH); // رجوع لحالة الجاهزية
  inputPassword = "";
}

void accessDenied() {
  Serial.println("\nالرمز أو البطاقة خاطئة!");
  digitalWrite(WHITE_LED, LOW);
  digitalWrite(RED_LED, HIGH);

  tone(BUZZER_PIN, 500, 300); delay(300);
  tone(BUZZER_PIN, 300, 500);

  delay(2000);

  digitalWrite(RED_LED, LOW);
  digitalWrite(WHITE_LED, HIGH); // رجوع لحالة الجاهزية
  inputPassword = "";
}