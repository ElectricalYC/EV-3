#include <Wire.h>

// Konfigurasi Nextion (gunakan Serial2 pada ESP32 untuk Nextion)
HardwareSerial nexSerial(2);  // Serial2 pada ESP32

// Alamat I2C dari Arduino Nano (slave)
const int NANO_I2C_ADDRESS = 8;

// Variabel untuk menyimpan data yang diterima dari Arduino Nano
float speed_kmph = 0.0;
float latitude = 0.0;
float longitude = 0.0;
float altitude = 0.0;
float hour = 0;
float minute = 0;

// Pin untuk input reset timer
const int resetPin = 5;  // GPIO15 pada ESP32
const int battPin = 2;   //GPIO2 pada ESP32

// Variabel untuk timer
unsigned long startTime = 0;
unsigned long currentTime = 0;
unsigned long elapsedTime = 0;
int milliseconds = 0;
int seconds = 0;
int tminutes = 0;

//Variabel untuk pembagi tegangan
// Tegangan referensi ESP32 (3.3V)
const float referenceVoltage = 3.3;
float batteryVoltage = 0;

// Rasio pembagian tegangan (40k dan 2k)
const float voltageDividerRatio = 43.0 / 2.0;
float measuredVoltage = 0;

// Resolusi ADC ESP32 (biasanya 12-bit, nilai maksimal 4095)
const int adcMaxValue = 4095;
int adcValue = 0;

// Debouncing untuk tombol reset
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 50;  // Debounce delay 200 ms

void setup() {
  Serial.begin(115200);                       // Debugging di Serial Monitor
  nexSerial.begin(9600, SERIAL_8N1, 16, 17);  // Nextion pada Serial2 (RX=16, TX=17)
  analogReadResolution(12);                   // Mengatur resolusi ADC

  // Inisialisasi I2C
  Wire.begin();

  // Inisialisasi pin reset sebagai input
  pinMode(resetPin, INPUT_PULLUP);

  // Mulai timer
  startTime = millis();  // Set waktu awal dari timer

  // Inisialisasi Nextion
  sendToNextion("text.txt=\"NA\"");
}

void loop() {
  // Memeriksa input reset dari pin D2 dengan debouncing
  if (digitalRead(resetPin) == LOW && (millis() - lastButtonPress) > debounceDelay) {
    resetTimer();                // Reset timer ketika tombol ditekan
    lastButtonPress = millis();  // Update waktu terakhir tombol ditekan
  }
  batteryCheck();
  // Update timer
  updateTimer();

  // Ambil data dari Arduino Nano melalui I2C
  requestGPSDataFromNano();

  // Tampilkan data ke Nextion
  sendToNextionValue("speed.val=", (int)speed_kmph);  // Kirim kecepatan sebagai nilai
  // sendToNextionValue("latitude.txt=\"", String(latitude, 4) + "\"");    // Kirim latitude sebagai teks
  // sendToNextionValue("longitude.txt=\"", String(longitude, 4) + "\"");  // Kirim longitude sebagai teks
  sendToNextionValue("tinggi.val=", (int)altitude);  // Kirim ketinggian sebagai nilai
  sendToNextionValue("hours.val=", hour);            // Kirim jam sebagai nilai
  sendToNextionValue("minute.val=", minute);         // Kirim menit sebagai nilai

  // Debugging: Print ke serial monitor
  Serial.print("Hour: ");
  Serial.println(hour);
  Serial.print("Minute: ");
  Serial.println(minute);
  Serial.print("Milliseconds: ");
  Serial.println(milliseconds);
  Serial.print("Seconds: ");
  Serial.println(seconds);
  Serial.print("Minutes: ");
  Serial.println(tminutes);

  delay(50);  // Tunggu 1 detik sebelum update berikutnya
}

// Fungsi untuk mengambil data dari Arduino Nano melalui I2C
void requestGPSDataFromNano() {
  Wire.requestFrom(8, 24);  // Meminta 20 byte data dari Arduino Nano (address 8)

  // Ambil data dari I2C jika tersedia
  if (Wire.available() == 24) {
    // Terima data floating point (4 byte per variabel)
    speed_kmph = receiveFloat();
    latitude = receiveFloat();
    longitude = receiveFloat();
    altitude = receiveFloat();

    // Terima data integer (2 byte per variabel)
    hour = receiveFloat();
    minute = receiveFloat();
  }

  delay(123);
}

// Fungsi untuk menerima data float (4 byte) dari I2C
float receiveFloat() {
  byte byteData[4];
  for (int i = 0; i < 4; i++) {
    byteData[i] = Wire.read();
  }
  float value;
  memcpy(&value, byteData, sizeof(value));
  return value;
}

// Fungsi untuk mengirimkan nilai ke Nextion (digunakan untuk val)
void sendToNextionValue(String command, int value) {
  nexSerial.print(command + String(value));
  nexSerial.write(0xFF);  // Mengirimkan 3 kali 0xFF sesuai protokol Nextion
  nexSerial.write(0xFF);
  nexSerial.write(0xFF);
}

// Fungsi untuk mengirimkan teks ke Nextion
void sendToNextionValue(String command, String text) {
  nexSerial.print(command + text);
  nexSerial.write(0xFF);  // Mengirimkan 3 kali 0xFF sesuai protokol Nextion
  nexSerial.write(0xFF);
  nexSerial.write(0xFF);
}

void sendToNextion(String command) {
  nexSerial.print(command);
  nexSerial.write(0xFF);  // Mengirimkan 3 kali 0xFF sesuai protokol Nextion
  nexSerial.write(0xFF);
  nexSerial.write(0xFF);
}

// Fungsi untuk update timer
void updateTimer() {
  currentTime = millis();                 // Ambil waktu sekarang
  elapsedTime = currentTime - startTime;  // Hitung waktu yang telah berlalu

  // Debugging: Print elapsed time
  Serial.print("Elapsed Time: ");
  Serial.println(elapsedTime);

  // Konversi waktu menjadi milidetik, detik, dan menit
  milliseconds = elapsedTime % 1000;
  seconds = (elapsedTime / 1000) % 60;
  tminutes = (elapsedTime / (1000 * 60)) % 60;

  // Kirimkan data timer ke Nextion
  sendToNextionValue("tmili.val=", milliseconds);
  sendToNextionValue("tsec.val=", seconds);
  sendToNextionValue("tmin.val=", tminutes);
}

// Fungsi untuk reset timer
void resetTimer() {
  startTime = millis();  // Reset waktu awal ke waktu saat ini
  elapsedTime = 0;       // Reset elapsedTime
}

void batteryCheck() {
  // Membaca nilai ADC dari baterai
  adcValue = analogRead(battPin);

  // Menghitung tegangan pada pin analog
  measuredVoltage = (adcValue * referenceVoltage) / adcMaxValue;

  // Menghitung tegangan baterai sebenarnya sebelum pembagian tegangan
  batteryVoltage = measuredVoltage * voltageDividerRatio;
  sendToNextionValue("batt.val=", (int)batteryVoltage);
}