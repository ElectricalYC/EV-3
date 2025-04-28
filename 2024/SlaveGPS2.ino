#include <Wire.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>  // Library untuk Watchdog Timer

// Baud rate GPS
static const uint32_t GPSBaud = 9600;

// Definisikan pin untuk SoftwareSerial (komunikasi dengan GPS)
SoftwareSerial ss(9, 8);  // RX, TX (gunakan pin yang sesuai)

// Objek GPS
TinyGPSPlus gps;

// Global variables to store GPS data
float speed_kmph = 0;
float latitude = 0;
float longitude = 0;
float altitude = 0;
float hour = 0;
float minute = 0;

// Variabel untuk tombol reset
const int resetPin = 2;  // Pin untuk tombol reset
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
bool lastButtonState = HIGH;  // Tombol dilepaskan
bool buttonState = HIGH;

void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);             // Komunikasi dengan GPS menggunakan SoftwareSerial
  Wire.begin(8);                 // I2C address Arduino Nano as slave
  Wire.onRequest(requestEvent);  // Event I2C untuk permintaan data

  pinMode(resetPin, INPUT_PULLUP);  // Setup tombol reset menggunakan pull-up internal

  Serial.println("Starting GPS communication...");
}

void loop() {
  // Bagian untuk memantau dan membaca data dari GPS
  while (ss.available() > 0) {
    gps.encode(ss.read());

    // Cek jika data GPS valid
    if (gps.location.isValid()) {
      speed_kmph = gps.speed.kmph();
      latitude = gps.location.lat();
      longitude = gps.location.lng();
      altitude = gps.altitude.meters();
      hour = gps.time.hour();
      minute = gps.time.minute();

      // Konversi waktu ke WIB (GMT+7)
      hour = (gps.time.hour() + 7);  // Tambahkan 7 jam untuk WIB

      // Jika jam lebih dari 23, kurangi 24 untuk memastikan tetap dalam rentang 0-23
      if (hour >= 24) {
        hour -= 24;
      } else if (hour < 0) {
        hour += 24;
      }

      // Debugging output
      Serial.print("Speed: ");
      Serial.println(speed_kmph);
      Serial.print("Altitude: ");
      Serial.println(altitude);
      Serial.print("Hour: ");
      Serial.println(hour);
      Serial.print("Minute: ");
      Serial.println(minute);
      Serial.print("Latitude: ");
      Serial.println(latitude, 6);
      Serial.print("Longitude: ");
      Serial.println(longitude, 6);
    }
  }

  // Bagian untuk memantau tombol reset dengan debouncing
  int reading = digitalRead(resetPin);

  // Jika ada perubahan status tombol
  if (reading != lastButtonState) {
    lastDebounceTime = millis();  // Catat waktu saat perubahan terdeteksi
  }

  // Jika tombol tetap dalam kondisi baru selama lebih dari debounceDelay
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Update kondisi tombol saat ini
    if (reading != buttonState) {
      buttonState = reading;

      // Jika tombol ditekan (LOW)
      if (buttonState == LOW) {
        Serial.println("Tombol ditekan, mereset sistem...");
        delay(100);  // Tambahkan sedikit delay sebelum reset

        // Aktifkan Watchdog Timer untuk mereset sistem
        wdt_enable(WDTO_15MS);  // Reset dalam 15ms
        while (1)
          ;  // Tunggu sampai watchdog timer melakukan reset
      }
    }
  }

  lastButtonState = reading;  // Simpan status tombol terakhir

  delay(600);  // Delay untuk mempermudah pembacaan
}

// Fungsi untuk mengirim data saat ada permintaan dari master I2C
void requestEvent() {
  // Mengirim data GPS melalui I2C dalam format yang sederhana
  sendFloat(speed_kmph);
  sendFloat(latitude);
  sendFloat(longitude);
  sendFloat(altitude);
  sendFloat(hour);
  sendFloat(minute);
}

// Fungsi untuk mengirim data float melalui I2C byte per byte
void sendFloat(float value) {
  byte *byteData = (byte *)&value;
  for (int i = 0; i < 4; i++) {
    Wire.write(byteData[i]);
  }
}
