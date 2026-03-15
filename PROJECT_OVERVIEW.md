<!--
================================================================================
📋 PROJECT_OVERVIEW.md - RF KUMANDA SİSTEMİ KAPSAMLI REHBER
================================================================================

⭐⭐⭐ BAŞLANGIÇ NOKTASI - BURADAN BAŞLA! ⭐⭐⭐

İŞLEV:
  - Bütün projenin kapsamlı açıklaması
  - 13 bölümde sistem mimarisi, dosya yapısı, veri akışı
  - Her dosyanın ne işi yaptığı ADETİ ADETİNE açıklandı
  - Yapılan işler, eksik kodlar, sonraki adımlar

OKUMA SIRASI (Tüm Başlayanlar İçin):
  1. "Proje Amacı" bölümü (sistem nedir?)
  2. "Dosya Yapısı" bölümü (hangi dosya nerede ne yapıyor?)
  3. "Veri Akışı" bölümü (kumanda → RF → SBUS → uçak)
  4. "KRİTİK NOKTALAR" bölümü (nelere dikkat et?)
  5. "Yapılan İşler" bölümü (neler bitmiş, neler yapılacak?)
  6. Diğer dosyaları oku (detailed referans)

DOSYA HARITASI (HIZLI İŞARET):
  platformio.ini          → Build config (tx/rx targets)
  shared/config.h         → PIN tanımları (ORTAK, SOURCE OF TRUTH)
  TX/src/main.cpp         → Kumanda yazılımı (tamamlandı)
  RX/src/main.cpp         → SBUS bridge (eksik SBUS encoder)
  TX/lib/config/config.h  → Deprecated duplicate
  RX/lib/config/config.h  → Deprecated duplicate
  
DOKÜMANTASYON (Referans):
  docs/HARDWARE.md        → Pin haritaları, 3-bileşen sistem
  docs/SBUS_PROTOCOL.md   → 25-byte frame detayı, 100kbaud
  docs/protocol.md        → ControlPacket (32 byte RF format)
  docs/calibration.md     → Kalibrasyon prosedürleri
  docs/DEPLOYMENT.md      → Deployment talimatları

KRİTİK NOKTALAR (Özet):
  1. SBUS = Inverted UART (GPIO43, 100kbaud, normal TTL değil)
  2. 11-bit Channel Encoding = kompleks bit packing (22 byte data)
  3. Voltage divider = 3.64 oranı (27k:10k, ±1% tolerance)
  4. TX Rate = 50Hz, RX SBUS Rate = 25Hz (farklı!)

ÖNEMLİ UYARI:
  ⚠️ TX/lib/config/config.h ve RX/lib/config/config.h DEPRECATED
  ⚠️ shared/config.h kullan (MASTER SOURCE)
  ⚠️ Duplicate code bugü var, refactoring lazım

VERSİYON:
  - Platform: ESP32-S3-N16R8 (240MHz, 16MB Flash, 8MB PSRAM)
  - Build: platformio.ini (tx/rx environments)
  - Dil: C++ (Arduino Framework)
  - Status: Aktif geliştirme (RX SBUS bridge kodu yapılıyor)

HEDEFİNİZ NE İSE:
  → TX kumanda kullanmak: TX/src/main.cpp oku
  → RX bridge test: RX/src/main.cpp oku (SBUS encoder eklenecek)
  → Orange Cube bağlantı: HARDWARE.md + SBUS_PROTOCOL.md oku
  → Sorun çözme: "KRİTİK NOKTALAR" + "ARAŞTIRMA ÖNERİLERİ" oku

HATA YAPMADAN:
  ✅ Değişim yap: shared/config.h (SOURCE OF TRUTH)
  ❌ Değişim yapma: TX/lib/config/config.h veya RX/lib/config/config.h
  ✅ SBUS sorunları: docs/SBUS_PROTOCOL.md troubleshooting
  ❌ Tahmin etme: kod referans dökümantasyon'da yazılı

================================================================================
-->

# 📋 RF Kumanda Sistemi - Proje Özet Rehberi

**Proje Başlama Tarihi:** Devam Ediyor  
**Sistem Türü:** 2.4GHz RF Kumanda (Otonom İHA için)  
**Ana Diller:** C++ (Arduino)  
**Hedef Platform:** ESP32-S3-N16R8 (Dual-core, 240MHz, 16MB Flash)

---

## 🎯 Proje Amacı

Savaşan/Otonom İHA (İnsansız Hava Aracı) için **tam wireless kumanda sistemi** geliştirmek. Sistem 3 ana bileşenden oluşur:

1. **TX (Kumanda):** Pilot tarafından kullanılan kontrol istasyonu
2. **RX (Alıcı):** İHA'ya monte edilen RF bridge (SBUS dönüştürücü)
3. **Orange Cube:** İHA'nın avioniği (autopilot)

```
    PILOT
      ↓
   [TX Kumanda]
   (Joysticks, Trims, Switches)
      ↓ (2.4GHz RF Signal)
   [RX Bridge]
   (ESP32 + SX1280)
      ↓ (SBUS Protocol)
   [Orange Cube Autopilot]
   (Pixhawk 4 → 14 PWM Servo)
```

---

## 📁 Dosya Yapısı ve İşlevleri

### **Kök Dizin Dosyaları**

#### 1. `platformio.ini` 📋
**İşlev:** Bütün proje için build ve derleme konfigürasyonu  
**Konumu:** `/platformio.ini`  
**İçeriği:**
- İki build hedfi (environment): `tx` ve `rx`
- ESP32-S3 board tanımı (esp32-s3-devkitc-1)
- Framework: Arduino
- Seri port hızı: 115200 baud
- Yükleme hızı: 921600 baud
- Kütüphane bağımlılıkları (Adafruit SSD1306, OLED ekran)

**Kullanım:**
```bash
pio run -e tx --target upload       # TX katmanını derle ve yükle
pio run -e rx --target upload       # RX kathanını derle ve yükle
pio run -e tx -t monitor            # TX seri portunu izle
```

---

#### 2. `README.md` 📖
**İşlev:** Projenin genel tanıtımı ve hızlı başlangıç  
**Konumu:** `/README.md`  
**Not:** Veri eksik, ihtiyaç halinde düzenlenebilir

---

### **Paylaşılan Konfigürasyon (`/shared/`)**

#### 3. `shared/config.h` ⚙️
**İşlev:** ESP32-S3'üm pin tanımları ve tüm sabit değerler (ORTAK)  
**Konumu:** `/shared/config.h`  
**Çok Önemli:** Hem TX hem RX bu dosyayı includeediyor!

**İçeriği:**
```c
// SPI (SX1280 RF)
SX1280_NSS = GPIO10, SX1280_RES = GPIO7, SX1280_BUSY = GPIO6, SX1280_DIO1 = GPIO5

// ADC Joystickler
JOYSTICK1_X = GPIO1  , JOYSTICK1_Y = GPIO2
JOYSTICK2_X = GPIO3  , JOYSTICK2_Y = GPIO4

// ADC Trim Potleri  
TRIM_ROLL = GPIO42, TRIM_PITCH = GPIO41, TRIM_YAW = GPIO40

// Anahtarlar (GPIO)
SWITCH_FLIGHT_MODE = GPIO16, SWITCH_TRIM_LOCK = GPIO17

// OLED I2C
OLED_SDA = GPIO9, OLED_SCL = GPIO8, I2C_ADDR = 0x3C

// RX Servo PWM Çıkışları (14 Servo)
GPIO_SERVO_THROTTLE = GPIO14, GPIO_SERVO_YAW = GPIO13, ...

// Pil Kalibrasyonu (8x AA = 12V)
VOLTAGE_DIVIDER = 3.64  // Gerilim bölücü (27k:10k)
BATTERY_MIN = 8.0V, BATTERY_MAX = 12.0V

// RF Parametreleri
FREQUENCY = 2400MHz (ISM band)
TX_POWER = 20dBm
SPREADING_FACTOR = 7 (LoRa SF7)
```

**İşlev:** Bu dosya AYNI olduğundan her PIN değişikliğinde HER İKİ taraftada senkron!

---

### **TX (Kumanda) Katmanı (`/TX/`)**

#### 4. `TX/lib/config/config.h` 🎮
**İşlev:** TX-specific konfigürasyonlar (şu an boş/basit)  
**Konumu:** `/TX/lib/config/config.h`  
**Notlar:**
- `shared/config.h` zaten tüm TX pinlerini tanımlıyor
- Gelecekte TX-specific ayarlar buraya eklenebilir (tx_power_level, vb)

---

#### 5. `TX/src/main.cpp` 🎮
**İşlev:** KUMANDA istasyonunun ana yazılımı  
**Konumu:** `/TX/src/main.cpp`  
**Kapsam (TAMAMLANMIŞ):**

```cpp
setup():
  ├─ Serial başlat (Debug: 115200 baud)
  ├─ I2C başlat (OLED ekran)
  ├─ SPI başlat (SX1280 RF)
  ├─ SX1280 RF modülünü initialize (2.4GHz, 50Hz TX rate)
  ├─ OLED displayini başlat (0.96" LCD)
  └─ GPIO anahtarlarını interrupt olarak konfigüre

loop():
  ├─ Joystick ADC değerlerini oku (4 eksen)
  ├─ Trim potansiyometre ADC değerlerini oku (3 eksen)
  ├─ Anahtar durumlarını oku (Mode, Trim Lock)
  ├─ Pil voltajını ölç (ADC voltage divider via GPIO8)
  ├─ Signal strength simülasyonu (-95 ~ -80 dBm range)
  ├─ Her 20ms'de paket gönder (50Hz rate)
  │  └─ ControlPacket struct -> SX1280 -> RF
  ├─ OLED ekranı güncelle (her 200ms)
  │  └─ 8 satır telemetri: voltage, signal, channels, switches
  └─ Hata durumlarını kontrol et (RF timeout, vb)
```

**Ekran Gösterim (8 satır):**
```
TX | BAT:(75%) PKT:234
SIGNAL: -92dBm [====- ] 85%
VOLT: 7.85V | ADC:3892
THR: 512 YAW: 0
ROLL: -12 PITCH:150
SW: [T1] [T2]
MODE: AUTO
STATUS: OK
```

**Paket Formatı (TX → RX RF):**
```c
struct ControlPacket {
  uint16_t throttle;   // 0-2047 (Gaz)
  uint16_t yaw;        // 0-2047 (Yaw/Dönüş)
  uint16_t roll;       // 0-2047 (Roll/Yalpalama)
  uint16_t pitch;      // 0-2047 (Pitch/Başgösteri)
  uint16_t trim_roll;  // 0-2047 (Roll trim)
  uint16_t trim_pitch; // 0-2047 (Pitch trim)
  uint16_t trim_yaw;   // 0-2047 (Yaw trim)
  uint8_t flight_mode; // 0=Auto, 1=FBWA
  uint8_t trim_locked; // 0=Unlocked, 1=Locked
  uint32_t packet_id;  // Paket numarası
};
```

**TX Gücü:** +20 dBm (maksimum, PA+LNA ile)  
**TX Hızı:** 50 Hz (20ms aralıklar)  
**Menzil:** ~500m+ line-of-sight (2.4GHz LoRa SF7)

---

### **RX (Alıcı Bridge) Katmanı (`/RX/`)**

#### 6. `RX/lib/config/config.h` 🛰️
**İşlev:** RX-specific konfigürasyonlar  
**Konumu:** `/RX/lib/config/config.h`  
**Notlar:**
- `shared/config.h` zaten tüm RX pinlerini tanımlıyor
- Gelecekte RX-specific ayarlar buraya eklenebilir

---

#### 7. `RX/src/main.cpp` 🛰️
**İşlev:** RF-SBUS BRIDGE yazılımı (İHA'ya monte edilir)  
**Konumu:** `/RX/src/main.cpp`  
**Kapsam (KISMEN TAMAMLANMIŞ):**

```cpp
setup():
  ├─ Serial başlat (Debug: 115200 baud)
  ├─ UART1 başlat (SBUS output: GPIO43, 100kbaud, **INVERTED**)
  ├─ SPI başlat (SX1280 RF receiver)
  ├─ SX1280'i RX moduna konfigüre
  ├─ Servo PWM çıkışlarını hazırla (GPIO11-14, 35-37)
  └─ Status LED'ini konfigüre (GPIO15)

loop():
  ├─ SX1280'den RF paketleri al (kontinyu RX mode)
  ├─ ControlPacket'ı SBUS frame'e dönüştür
  ├─ Her 40ms'de SBUS frame gönder (25Hz rate) → GPIO43 → Orange Cube
  │  └─ 25 byte SBUS frame (0x0F start, 16 ch × 11-bit, 0x00 end)
  ├─ Signal loss detect: Timeout > 500ms → failsafe
  ├─ Status LED'i blink (rx signal based)
  └─ **ÖNEMLİ:** SBUS inverted UART (normal TTL değil!)
```

**SBUS Frame Yapısı (25 byte):**
```
[0] = 0x0F (start)
[1-22] = 16 channel data (176 bits = 16 × 11-bit)
[23] = 0x00 (flags/status)
[24] = 0x00 (end)

Baudrate: 100,000 bps (100 kbaud) - NON-STANDARD!
Format: SERIAL_8E2 (8 data, Even parity, 2 stop)
Signal: INVERTED (idle=LOW, active=HIGH)
```

**SBUS Channel Mapping (örnek):**
```
SBUS CH → Orange Cube Function
1 = Throttle (Motor)
2 = Yaw (Rudder)
3 = Roll (Aileron)
4 = Pitch (Elevator)
5-16 = Auxiliary (Trim, Mode, Camera, vb)
```

**EKSIK KODLAR (Yapılacak):**
- ⚠️ `encodeSBUSFrame()` fonksiyonu tamamlanmadı
- ⚠️ `sendSBUSFrame()` implementasyonu eksik
- ⚠️ UART1 inversion setup kodu eksik
- ⚠️ Servo PWM output kodu (backup analog PWM) eksik

---

### **Dokümantasyon Dosyaları (`/docs/`)**

#### 8. `docs/HARDWARE.md` 🔧
**İşlev:** Donanım mimarisi, pin haritaları, bağlantı şemaları  
**Konumu:** `/docs/HARDWARE.md`  
**İçeriği:**
- 3-bileşen sistem mimarisi (TX, RX, Orange Cube)
- TX pin haritası (SPI, I2C, ADC, GPIO)
- RX pin haritası (SPI, UART1 SBUS output)
- Orange Cube SBUS bağlantı detayları
- NVIDIA Jetson Nano Orin (opsiyonel AI compute)
- Blok diyagramı

**ÖNEMLİ NOT:** Bu dosya SADECE HABERLEŞİYİ kapsıyor (güç yönetimi değil!)

---

#### 9. `docs/SBUS_PROTOCOL.md` 📡
**İşlev:** SBUS protokolü tam referansı ve kod örnekleri  
**Konumu:** `/docs/SBUS_PROTOCOL.md`  
**İçeriği (200+ satır):**
- SBUS frame formatı (25 byte detaylı analiz)
- Baudrate 100kbaud (non-standard!)
- **Inverted UART** açıklaması (TTL'den farklı!)
- 11-bit channel encoding algoritması
- C++ encoder/decoder örnekleri
- GPS43 UART1 setup kodu
- Orange Cube bağlantı prosedürü
- QGroundControl test adımları
- Sorun giderme (baudrate hataları, inversion, vb)

**Kritik Kod Snippets:**
```cpp
// UART1 SBUS setup (100kbaud, inverted)
Serial1.begin(100000, SERIAL_8E2, -1, 43);
uart_set_line_inverse(UART_NUM_1, UART_INVERSE_TXD); // INVERT!

// SBUS frame encode örneği
void encodeSBUSFrame(uint16_t *channels, uint8_t *frame) {
    frame[0] = 0x0F;  // start
    // 16 channel × 11-bit encoding (complex bit packing)
    // ...
    frame[23] = 0x00; // flags
    frame[24] = 0x00; // end
}

// Send to Orange Cube
Serial1.write(frame, 25);
```

---

#### 10. `docs/protocol.md` 📜
**İşlev:** TX ↔ RX RF iletişim protokolü  
**Konumu:** `/docs/protocol.md`  
**Notlar:** Şu an basit, ControlPacket struct açıklıyor

---

#### 11. `docs/pinout.md` & `docs/pinout_rx.md` 📍
**İşlev:** TX ve RX ESP32-S3 pin eşlemeleri  
**Konumu:** `/docs/pinout.md`, `/docs/pinout_rx.md`  
**Notlar:** `shared/config.h`'de de bu pinler tanımlanıyor (kaynak tek!)

---

#### 12. `docs/DEPLOYMENT.md` 🚀
**İşlev:** Sistemin nasıl konuşlandırılacağı (deployment)  
**Konumu:** `/docs/DEPLOYMENT.md`  
**Notlar:** Şu an sınırlı bilgi içeriyor

---

#### 13. `docs/calibration.md` 🎯
**İşlev:** Kumanda kalibrasyon prosedürleri  
**Konumu:** `/docs/calibration.md`  
**Notlar:** Joystick neutral points, voltage divider calibration

---

---

## 🔄 Veri Akışı (Haberleşeme Zinciri)

### **Durum 1: Normal İşletim**

```
1. PILOT KUMANDASI (TX)
   ├─ Joystick Position: Roll +50, Pitch -30, Yaw 0, Throttle +800
   ├─ Trim Potleri: Roll 0, Pitch +10, Yaw -5
   ├─ Anahtarlar: Mode=AUTO, TrimLock=ON
   └─ Pil Voltajı: 10.2V

   ↓ (TX/src/main.cpp loop)
   
2. ControlPacket OLUŞTUR
   {
     throttle: 800,
     yaw: 0,
     roll: 50,
     pitch: 30,
     trim_roll: 0,
     trim_pitch: 10,
     trim_yaw: -5,
     flight_mode: 0 (AUTO),
     trim_locked: 1,
     packet_id: 234
   }

   ↓ (Every 20ms @ 50Hz)

3. SX1280 VIA SPI (GPIO10=CS, GPIO12=CLK, GPIO11=MOSI)
   ├─ SX1280 RAM'a packet yazılır
   ├─ TX komutu gönderilir
   └─ 2.4GHz LoRa modülasyonyla broadcast

   ↓ (20ms transmission time ≈ 2-3ms)
   ↓ (500m+ menzilde)

4. RX ALICISI (RX/src/main.cpp)
   ├─ SX1280 kontinyu RX modunda
   ├─ Packet interrupt (DIO1 pin, GPIO5)
   ├─ ControlPacket verisi SPI'dan okunur (GPIO13=MISO)
   └─ CRC doğrulanır (geçerse işlep devam)

   ↓ (RX loop)

5. SBUS FRAME ENCODERİ
   {
     Input: ControlPacket (16 channels)
     Output: 25-byte SBUS frame
     
     Encoding:
     - Frame[0] = 0x0F (start)
     - Frame[1-22] = 176-bit channel data (11-bit × 16 ch)
       * CH1 (throttle): 800 → 11-bit encoding in frame[1-2]
       * CH2 (yaw): 0 → 11-bit
       * ...CH16
     - Frame[23] = flags (signal loss bit, failsafe bit)
     - Frame[24] = 0x00 (end)
   }

   ↓ (Every 40ms @ 25Hz)

6. UART1 TRANSMISSION (GPIO43)
   ├─ Baud: 100,000 bps (non-standard!)
   ├─ Format: 8E2 (8 data, Even parity, 2 stop bits)
   ├─ **Signal:** INVERTED (idle=LOW, active=HIGH)
   │  (Not normal TTL: normal would be idle=HIGH)
   ├─ 25 bytes × ~11 bits each = ~2.75ms transmission
   └─ GPIO43 → SBUS cable → Orange Cube SBUS IN

   ↓ (Frame arrives at RC receiver port)

7. ORANGE CUBE AUTOPILOT
   ├─ SBUS frame alıcısı: RC_CHANNELS_RAW message oluştur
   ├─ 16 channel decode (11-bit → PWM µs mapping)
   │  * CH1 (throttle) → PWM 988-2012 µs
   │  * CH2 (yaw) → PWM 988-2012 µs
   │  * ...
   ├─ 14 PWM servo çıkışı (ESC, servo motorlar)
   └─ Autopilot logic: Roll, Pitch, Yaw servolarını harekete geçir

   ↓ (Concurrent)

8. İHA PRATİK RESPONSE
   ├─ Motor: Throttle PWM değerine göre hızlanır/yavaşlar
   ├─ Ailerons (CH3): Roll servo yapar
   ├─ Elevator (CH4): Pitch servo yapar
   ├─ Rudder (CH2): Yaw servo yapar
   └─ Uçak kumanda sinyaline karşılık verir!
```

### **Zaman Senkronizasyonu**

```
TX Rate (50Hz)  : Every 20ms  paket gönder
  └─ RF link    : ~2-3ms transmission + latency (100-200ms total)

RX Rate (25Hz)  : Every 40ms  SBUS frame gönder
  └─ Orange Cube: ~1-5ms frame receive + servo update

Total Latency   : 100-250ms (acceptable for slow-flying UAV)
```

---

## ⚠️ KRİTİK NOKTALAR

### **1. SBUS Inverted UART - ÇOK ÖNEMLİ! 中**

Normal TTL UART:
- Idle (boşta): HIGH (3.3V)
- Active (transmit): LOW (0V)
- Inverse demodulation gerekli

SBUS (Inverted):
- Idle: LOW (0V)
- Active: HIGH (3.3V)
- Direct connection (no inverter needed)

**RX/src/main.cpp'da YAPILMASI GEREKEN:**
```cpp
uart_set_line_inverse(UART_NUM_1, UART_INVERSE_TXD);  // Enableinversion!
```

---

### **2. Non-Standard 100kbaud Baudrate**

Most standard rates: 9600, 115200, 230400 bps  
SBUS: **100,000 bps** (weird but standard for SBUS!)

Arduino Serial setup:
```cpp
Serial1.begin(100000, SERIAL_8E2, -1, 43);  // Exact 100000!
```

---

### **3. 11-Bit Channel Encoding**

Each channel: 11-bit değeri (0-2047 range)
16 channels × 11 bits = 176 bits = 22 bytes

Complex bit-packing algoritması! (SBUS_PROTOCOL.md'de örnekler var)

---

### **4. Voltage Divider Kalibrasyonu**

12V battery → 27kΩ + 10kΩ → 3.3V ADC

Dikkat: 27k ve 10kΩ **1% tolerance** olmalı! Aksi tavkdirde hatalı ölçüm.

Kalibrasyon:
```
Real Voltage (multimetre) ← ADC value ×ölç
Doğrulama: VOLTAGE_DIVIDER = 3.64 (27k:10k)
```

---

## 🛠️ Şimdiye Kadar Yapılan İşler

✅ **TAMAMLANDI:**
- platformio.ini proje yapısı oluşturdu (tx/rx targets)
- shared/config.h tüm pinler tanımlaması
- TX/src/main.cpp kumanda yazılımı (joystick, telemetry, OLED)
- TX telemetry display (8-satır bilgi gösterimi)
- Voltage filtering (exponential moving average)
- Signal strength simulation (-95 to -80 dBm)
- HARDWARE.md haberleışme mimarisi (3-component)
- SBUS_PROTOCOL.md kapsamlı referans (200+ lines)

⚠️ **KISMEN YAPILDI:**
- RX/src/main.cpp RF alıcısı kodlanıyor (eksik SBUS encoder)
- SBUS frame encoding algoritması (örnek verildi, integration yok)
- Inverted UART setup (stub var, test yok)

❌ **YAPıLAMADI:**
- Fiziksel donanım test (embedded system henüz yok)
- SBUS → Orange Cube bağlantı test
- QGroundControl RC channels verification
- Edge case handling (signal loss, failsafe, battery warning)
- Servo PWM backup moduna (analog PWM if UART fails)

---

## 📚 ARAŞTIRMA "ÖNERİ LİSTESİ"

Eğer hatalar veya issues yaşarsan, şunları kontrol et:

1. **SBUS Inversion Issues**
   - ESP32-S3 UART register inversion fonksiyonları
   - `uart_set_line_inverse()` çalışıp çalışmadığı
   - Oscilloscope: GPIO43 sinyalinişekli kontrolü

2. **100kbaud Serial Timing**
   - ESP32-S3 seri baud rate hesaplaması
   - PLL ayarlanması (genellikle otomatik ama kontrol et)
   - Jitter tolerance (±2% acceptable)

3. **SBUS 11-bit Channel Encoding**
   - Bit packing algoritması (complexity!)
   - Frame[1-22] bit order (LSB/MSB?)
   - Online SBUS decoder tool ile manual test

4. **SX1280 Modülasyon Parametreleri**
   - 2.4GHz frequency accuracy (±50kHz tolerance)
   - LoRa SF7 transmission time vs packet size
   - PA+LNA power amplifier maximum rating

5. **Orange Cube SBUS Receiver Support**
   - Pixhawk 4 Orange Cube' SBUS input modülü havailable?
   - MAVLink auto-protocol detection (ne belirtir?)
   - QGroundControl RC_CHANNELS_RAW message parsing

---

## 📖 Dosya Okuma Sırası (Yeni Başlayanlar İçin)

1. **Bu dosyayı oku:** `PROJECT_OVERVIEW.md` (Şu an okuduğun)
2. **Hadware anlayın:** `docs/HARDWARE.md` (3-component sistem)
3. **SBUS protokolü:** `docs/SBUS_PROTOCOL.md` (25-byte frame detayı)
4. **TX kodu:** `TX/src/main.cpp` (kumanda logic)
5. **RX kodu:** `RX/src/main.cpp` (RF → SBUS bridge)
6. **Konfigürasyonu yönet:** `shared/config.h` (PIN & CONST değişiklikleri)
7. **Build & test:** `platformio.ini` (tx/rx targets)

---

## 🚀 SONRAKI ADIMLAR

### **Acil (Critical Path - Bu Hafta)**
1. RX/src/main.cpp'de SBUS encoder fonksiyonlarınıaraştır
   - `encodeSBUSFrame()` tamamlanacak
   - `sendSBUSFrame()` UART1'e bağlanacak
   - `uart_set_line_inverse()` test edilecek

2. UART1 inversion test et
   - Oscilloscope: GPIO43 sinyali kontrol
   - 100kbaud timing accuracy
   - Orange Cube bağlantısı simüle et

3. Mock Orange Cube testi
   - ESP32 RX tarafında SBUS frames debug output
   - 25Hz frame rate consistency
   - Channel value ranges (988-2012 µs mapping)

### **Orta Vadede (Test & Validation - 2-3 Hafta)**
1. Fiziksel TX kumanda kütüphanesi teste
   - Joystick calibration prosedürü
   - Pil voltajı doğrulaması
   - RF link range test (500m çıkış var mı?)

2. RX - Orange Cube integrasyonu
   - SBUS cable bağlantısı
   - QGroundControl'de RC channels görülüyor mu?
   - Servo response lag testi (latency measurement)

3. Uçak simulatoru veya benchtop test
   - Motor + ESC mock ile servo PWM output test
   - Failsafe behavior (signal loss handling)
   - Battery warning telemetry

### **Uzun Vadede (Dağıtım - 1-2 Ay)**
1. Serileştirme & dokumentasyon
2. Kasa tasarımı (3D printing)
3. Production build (PCB vs breadboard)
4. Field testing & tuning

---

## 📞 KİŞİSEL NOTLAR

- **Proje Sahibi İhtiyacı:** 2 ESP32-S3, 2 SX1280, 1 OLED, Orange Cube, Jetson (opsiyonel)
- **PlatformIO Environment:** Çalışında `.venv/` Python venv
- **Derleme Hedefi:** `platformio.ini` tx/rx environments
- **Debug Output:** Serial @ 115200 baud (PlatformIO monitor)

**Iletişim:**
- Hata Ayıklama: `platformio.ini` → Serial Monitor
- Donanım Bağlantı: `shared/config.h` → PIN tanımları
- SBUS Protokolü: `docs/SBUS_PROTOCOL.md` → Full Reference

---

**Son Güncelleme:** 2026-03-15  
**Durum:** Aktif Geliştirme - RX SBUS Bridge Kodu İçinde

