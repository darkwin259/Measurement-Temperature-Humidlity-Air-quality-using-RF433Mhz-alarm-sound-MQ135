#include <MQ135.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <VirtualWire.h>

#define DHTPIN 4  
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);

// Địa chỉ I2C của LCD, có thể là 0x27 hoặc 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int receive_pin = 12;
const int blueLED = 8;
const int redLED = 9;
const int buzzer = 10;
const int smokeA0 = A0;

char temperatureChar[10];
char humidityChar[10];
char pChar_remote[10];
char tdsChar_remote[10];

char temperatureChar_local[10];
char humidityChar_local[10];
char pChar_local[10];
char tdsChar_local[10];

struct package {
  int temperature = 0;
  int humidity = 0;
};

typedef struct package Package;
Package data;

void setup() {
  Serial.begin(9600);  
  dht.begin();

  lcd.init();
  lcd.backlight();
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(smokeA0, INPUT);
  delay(1000);

  // Khởi tạo VirtualWire
  vw_set_rx_pin(receive_pin);
  vw_setup(500);   // Bits per sec
  vw_rx_start();   // Bắt đầu nhận dữ liệu
}

void loop() {
  uint8_t buf[sizeof(data)];
  uint8_t buflen = sizeof(data);

  int smokeValue = analogRead(smokeA0);

  if (smokeValue > 170)
  {
    tone(buzzer,2800, 200);
  }
  else
  {
    noTone(buzzer);
  }
  delay(500);


  if (vw_have_message()) {  // Kiểm tra xem có gói tin nào không
    vw_get_message(buf, &buflen);
    memcpy(&data, &buf, buflen);
    Serial.print("\nRemote: ");
    Serial.print(data.temperature);
    
    String temperatureString = String(data.temperature);
    temperatureString.toCharArray(temperatureChar, 10);
    
    String humidityString = String(data.humidity);
    humidityString.toCharArray(humidityChar, 10);
    
    Serial.print("\n");
    Serial.println(data.humidity);

    // Đọc dữ liệu từ cảm biến DHT11
    int Temperature_local = dht.readTemperature();
    int Humidity_local = dht.readHumidity();

    String temperatureString_local = String(Temperature_local);
    temperatureString_local.toCharArray(temperatureChar_local, 10);
    String humidityString_local = String(Humidity_local);
    humidityString_local.toCharArray(humidityChar_local, 10);

    int RH_remote = data.humidity;
    int T_remote  = data.temperature;

    // Tính toán tham số liên quan
    // Remote
    // Độ ẩm cực đại p
    float p_remote = -3*pow(10,-9)*pow(T_remote,6) + 2*pow(10,-7)*pow(T_remote,5) - 2*pow(10,-6)*pow(T_remote,4)*0.0003*pow(T_remote,3) + 0.0088*pow(T_remote,2) + 0.3421*T_remote + 4.8397;
    // Độ ẩm cực đại AH
    float AH_remote = (p_remote*RH_remote) / 100;
    // Nhiệt độ điểm sương Tds
    float Tds_remote = -2*pow(10,-7) * pow(AH_remote, 6) + 3*10e-5 * pow(AH_remote, 5) - 0.0013 * pow(AH_remote, 4) + 0.0349 * pow(AH_remote, 3) - 0.5701 * pow(AH_remote, 2) + 6.4796 * AH_remote - 21.278;

    //Local
    // Độ ẩm cực đại p
    float p_local = -3*pow(10,-9)*pow(Temperature_local,6) + 2*pow(10,-7)*pow(Temperature_local,5) - 2*pow(10,-6)*pow(Temperature_local,4)*0.0003*pow(Temperature_local,3) + 0.0088*pow(Temperature_local,2) + 0.3421*Temperature_local + 4.8397;
    // Độ ẩm cực đại AH
    float AH_local = (p_local*Humidity_local) / 100;
    // Nhiệt độ điểm sương Tds
    float Tds_local = -2*pow(10,-7) * pow(AH_local, 6) + 3*10e-5 * pow(AH_local, 5) - 0.0013 * pow(AH_local, 4) + 0.0349 * pow(AH_local, 3) - 0.5701 * pow(AH_local, 2) + 6.4796 * AH_local - 21.278;

    // Chuyển đổi dữ liệu sang chuỗi
    String pString_remote = String(p_remote, 2);
    pString_remote.toCharArray(pChar_remote, 10);
    String tdsString_remote = String(Tds_remote, 2);
    tdsString_remote.toCharArray(tdsChar_remote, 10);

    String pString_local = String(p_local, 2);
    pString_local.toCharArray(pChar_local, 10);
    String TdsString_local = String(Tds_local, 2);
    TdsString_local.toCharArray(tdsChar_local, 10);

    // Check the condition to turn on the LEDs
    if (((p_local > p_remote) && (18 <= T_remote && T_remote <= 32) && (T_remote > Temperature_local) && (Tds_remote < Tds_local)) || 
    ((T_remote < Temperature_local) && (Tds_remote > Tds_local))) {
      digitalWrite(blueLED, HIGH); // Turn on blue LED
      digitalWrite(redLED, LOW);    // Turn off red LED
    } else {
      digitalWrite(blueLED, LOW);  // Turn off blue LED
      digitalWrite(redLED, HIGH);   // Turn on red LED
    }

    // Hiển thị S và Tdp local trên Serial Monitor
    Serial.print(" Temp local: ");
    Serial.println(Temperature_local);
    Serial.print(" Humid local: ");
    Serial.println(Humidity_local);

    Serial.print(" p remote : ");
    Serial.println(p_remote);
    Serial.print(" AH remote: ");
    Serial.println(AH_remote);
    Serial.print(" Tds remote: ");
    Serial.println(Tds_remote);

    Serial.print(" p local : ");
    Serial.println(p_local);
    Serial.print(" AH local: ");
    Serial.println(AH_local);
    Serial.print(" Tds local: ");
    Serial.println(Tds_local);

    Serial.print("Smokevalue:");
    Serial.print(smokeValue);

    //Hiển thị lên lcd
    lcd.setCursor(0, 0);
    lcd.print(temperatureChar);
    lcd.print(";");
    lcd.print(humidityChar);
    lcd.print(";");
    lcd.print(pChar_remote);
    lcd.print(";");
    lcd.print(tdsChar_remote);

    lcd.setCursor(0, 1);
    lcd.print(temperatureChar_local);
    lcd.print(";");
    lcd.print(humidityChar_local);
    lcd.print(";");
    lcd.print(pChar_local);
    lcd.print(";");
    lcd.print(tdsChar_local);

  }

}
