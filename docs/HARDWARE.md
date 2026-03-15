<!--
================================================================================
🔧 docs/HARDWARE.md - Donanım Mimarisi ve Pin Haritaları
================================================================================

İŞLEV:
  - 3-bileşen sistem mimarisi (TX kumanda, RX bridge, Orange Cube autopilot)
  - ESP32-S3 pin tanımları ve harita (SPI, I2C, ADC, GPIO, UART1)
  - SBUS bağlantı şeması (GPIO43 → Orange Cube)
  - Voltage divider kalibrasyonu (12V → 3.3V ADC)
  - Orange Cube RC receiver port bağlantısı

KRİTİK NOKTALAR:
  ⚠️ SBUS OUTPUT = GPIO43 (UART1_TX, INVERTED, 100kbaud)
  ⚠️ RF MODÜLÜ = SPI (GPIO10=CS, GPIO12=CLK, GPIO11=MOSI, GPIO13=MISO)
  ⚠️ OLED SCREEN = I2C (GPIO9=SDA, GPIO8=SCL, addr=0x3C, TX only)
  ⚠️ VOLTAGE DIV = 27kΩ:10kΩ (oranı 3.64, 1% tolerance!)
  ⚠️ Orange Cube SBUS IN = RC5433 port PIN1 (GPIO43 bağlanır)

ODAK: YALNIZCA İLETİŞİM KATMANI
  - RF ↔ SBUS bridge açıklanır
  - Enerji yönetimi, XL6009, pil detayları HARIÇ

KULLANIM:
  1. TX pin setup (kumanda kontrolü)
  2. RX pin setup (RF alıcı + SBUS çıkış)
  3. Orange Cube bağlantı (SBUS in → servo PWM out)

İLGİLİ DOSYALAR:
  - shared/config.h → PIN #define değerleri (Source OF TRUTH)
  - TX/src/main.cpp → Kumanda input oku
  - RX/src/main.cpp → SBUS output gönder
  - docs/SBUS_PROTOCOL.md → 25-byte frame format detayı
  - PROJECT_OVERVIEW.md → Sistem flow ve veri akışı

REFERANS:
  - SPI Hızı: 10 MHz (SX1280 spec)
  - I2C Hızı: 100-400 kHz (OLED standard)
  - ADC Resolution: 12-bit (4095 max)
  - Orange Cube: Pixhawk 4, STM32F765 CPU

================================================================================
-->
# 🔧 Haberleşeme Mimarisi - RF Kumanda Sistemi

**Proje:** 2.4 GHz RF Kumanda (TX → RX → Orange Cube Autopilot)  
**Odak:** SADECE İletişim Katmanı (RF ↔ SBUS bridge)

---

## � Sistem Mimarisi (3-Bileşen)

```
┌──────────────────────────┐
│  KUMANDA İSTASYONU (TX)  │
│  ESP32-S3 + SX1280 RF    │
│  + OLED + Joysticks      │
│  + 2x AA Batteries       │
└────────────┬─────────────┘
             │
      [2.4GHz RF Link]
      [50Hz LoRa Packets]  
             │
  ┌──────────▼────────────┐
  │   RX RF BRIDGE        │
  │  ESP32-S3 + SX1280    │
  │  (GPIO43 UART1_TX)    │
  └──────────┬────────────┘
             │
   [100kbaud Inverted UART]
             │
  ┌──────────▼──────────────┐
  │ ORANGE CUBE (Pixhawk 4) │
  │ SBUS IN → 14 PWM Out    │
  └─────────────────────────┘
             │ (opsiyonel)
  ┌──────────▼──────────────┐
  │  NVIDIA Jetson Nano     │
  │  (MAVLink telemetry)    │
  └─────────────────────────┘
```

**Veri Akışı:**
1. TX: Joystick input → SX1280 RF paket (50Hz, 2.4GHz)
2. RX: SX1280 receive → SBUS frame encoder (25Hz)
3. Orange Cube: SBUS IN → 14 PWM servo outputs

---

## 🔌 Detaylı Pin Haritası

### **TX (Kumanda İstasyonu) - ESP32-S3 Pinleri**

#### SPI Arayüzü (SX1280 RF Modülü)
| Fonksiyon | Pin # | Açıklama |
|-----------|-------|----------|
| CS (NSS) | GPIO 10 | Chip Select |
| RESET | GPIO 7 | RF Modülü Reset |
| BUSY | GPIO 6 | Hazır Sinyali |
| DIO1 | GPIO 5 | Interrupt Sinyali |
| MOSI | GPIO 11 | Master Out Slave In (SPI) |
| MISO | GPIO 13 | Master In Slave Out (SPI) |
| CLK/SCK | GPIO 12 | SPI Clock (10 MHz) |

#### I2C Arayüzü (OLED Ekran)
| Fonksiyon | Pin # | Açıklama |
|-----------|-------|----------|
| SDA | GPIO 9 | I2C Data |
| SCL | GPIO 8 | I2C Clock |
| Adres | 0x3C | SSD1306 I2C Adresi |

#### ADC Girişleri - Joystickler (4 Eksen)
| Kanal | Pin # | Fonksiyon | Açıklama |
|-------|-------|-----------|----------|
| JST1_X | GPIO 1 | Roll/Aileron | X Ekseni Sağa-Sola |
| JST1_Y | GPIO 2 | Pitch/Elevator | Y Ekseni Yukarı-Aşağı |
| JST2_X | GPIO 3 | Yaw/Rudder | X Ekseni Çevirme |
| JST2_Y | GPIO 4 | Throttle/Gaz | Y Ekseni Gazına Basma |

#### ADC Girişleri - Trim Potansiyometreleri (3x)
| Kanal | Pin # | Fonksiyon | Açıklama |
|-------|-------|-----------|----------|
| TRIM_Roll | GPIO 42 | Roll Trim | Merkez Ayarı Roll |
| TRIM_Pitch | GPIO 41 | Pitch Trim | Merkez Ayarı Pitch |
| TRIM_Yaw | GPIO 40 | Yaw Trim | Merkez Ayarı Yaw |

#### ADC Girişleri - Pil Voltajı
| Kanal | Pin # | Fonksiyon | Açıklama |
|-------|-------|-----------|----------|
| BATT_SENSE | GPIO 8 | Voltaj Ölçümü | (Kullanıcının beslemesi) |

---

### **RX (RF Bridge) - ESP32-S3 Pinleri (Orange Cube & Jetson Interface)**

#### SPI Arayüzü (SX1280 RF Modülü)
*TX ile aynı*

| Fonksiyon | Pin # | Açıklama |
|-----------|-------|----------|
| CS (NSS) | GPIO 10 | Chip Select |
| RESET | GPIO 7 | RF Modülü Reset |
| BUSY | GPIO 6 | Hazır Sinyali |
| DIO1 | GPIO 5 | Interrupt Sinyali |
| MOSI | GPIO 11 | Master Out Slave In |
| MISO | GPIO 13 | Master In Slave Out |
| CLK/SCK | GPIO 12 | SPI Clock (10 MHz) |

#### SBUS UART Çıkışı (→ Orange Cube)
| Fonksiyon | Pin # | Açıklama |
|-----------|-------|----------|
| SBUS_TX | GPIO 43 | UART1 TX → Orange Cube SBUS IN |
| GND | GND | Signal Ground (Referans) |

**SBUS Protokolü Özellikleri:**
- Baudrate: **100kbps** (100000)
- Data Bits: 8
- Stop Bits: 2
- Parity: Even (SERIAL_8E2)
- **İnverted:** Evet (serial signal ters)
- Frame Rate: 25Hz (40ms/frame)
- Start Byte: 0x0F
- End Byte: 0x00
- 16 kanal PWM emulasyonu (11-bit resolution)

#### GPIO - Status & Debug
| Fonksiyon | Pin # | Tip | Açıklama |
|-----------|-------|-----|----------|
| STATUS_LED | GPIO 15 | Output | RF bağlantı göstergesi |

---

### **Orange Cube (Pixhawk 4) Bağlantı**

| Orange Cube Pin | Fonksiyon | ESP32-S3 Bağlantısı |
|-----------------|-----------|-------------------|
| SBUS IN | RX Signal | GPIO 43 (SBUS_TX) |
| GND | Ground | GND |
| VCC (5V) | Power (opsiyonel) | Harici 5V kaynağından |

**NOT:** SBUS inverted UART sinyalini kabul eder. Standart TTL UART uyumsuz!

---

### **NVIDIA Jetson Nano Orin Bağlantı**

| Jetson Pin | Fonksiyon | Orange Cube Bağlantısı |
|------------|-----------|----------------------|
| UART1_RX | MAVLink In | Orange Cube TELEM1 TX |
| UART1_TX | MAVLink Out | Orange Cube TELEM1 RX |
| GND | Ground | Orange Cube GND |
| 5V Power | Besleme | External PSU (25W) |

**Jetson Nano Orin Özellikleri:**
- Quad-core ARM Cortex-A78AE (3.5GHz)
- 8GB LPDDR5 RAM
- 128-bit 204GB/s memory bandwidth
- 6 TOPS AI performance (INT8)
- ROS2, TensorFlow, PyTorch destekleri
- USB 3.1 x2 + USB 2.0 x4
- Gigabit Ethernet
- 25W max power consumption

**İletişim Protokolü:**
- Orange Cube ↔ Jetson: MAVLink 2.0 (100Hz telemetry)
- Jetson ↔ Orange Cube: Komut gönderimi (waypoint, mode switch)

---

## 🔋 TX Beslemesi - 8x AA Alkaline (12V)

```
8x AA Alkaline Batarya (12V nominal)
    ↓
[XL6009 Step-Down: 12V → 3.3V]
    ├─→ ESP32-S3 (3.3V @ 500mA)
    ├─→ SX1280 RF Modülü (3.3V @ 300mA peak)
    ├─→ OLED Ekran (3.3V @ 20mA)
    ├─→ Joystickler & Trim (3.3V @ 10mA)
    └─→ Anahtarlar (3.3V @ 5mA)

Tahmini Pil Ömrü: 13 saat (normal), 8 saat (ağır)
```

---

## 📡 Haberleşme Mimarisi (RX Tarafı)

Kalibrasyon Tablosu:
| Pil Voltajı | V_ADC | ADC Okuma |
|-------------|-------|----------|
| 12.0V (Yeni) | 3.24V | 4023 |
| 10.0V (Normal) | 2.70V | 3355 |
| 8.0V (Uyarı) | 2.16V | 2685 |
| 7.2V (Boş) | 1.94V | 2410 |

Test Prosedürü:
1. Multimetre ile gerçek voltajı ölç
2. Serial monitöre ADC okuma kayde et
3. VOLTAGE_DIVIDER değerini ince ayar yap (3.64 base)
```

---

## ⚡ XL6009 Step-Down Konverter Kurulum Detayları

**12V → 3.3V Dönüşümü için Devre ve Ayarlama:**

```
Devre Şeması:
                L (Induktör)
                │ ┌───┐
    +12V ───────┤ │   D1 (Schottky Diyot)
    GND ───────┐├─┴───┤
               │      ├──→ +3.3V OUT
           XL6009    ═══ C1 (Çıkış)
          Modülü      100µF/16V
               │
               └──FB ─┬─[R1:10kΩ]─┬── +12V
                      │          │
                  GND ┴─[R2:3.3kΩ]┘

Feedback Oranı Hesapping:
- Vout = 0.8V × (1 + R1/R2)
- 3.3V = 0.8V × (1 + R1/R2)
- 3.3/0.8 = 1 + R1/R2
- R1/R2 = 3.125

Standart Direnç Seçenekleri:
┌──────────────────────────────────┐
│ R1 = 10kΩ ±1%  (Vin tarafı)      │
│ R2 = 3.3kΩ ±1% (GND tarafı)      │
│ → Vout = 3.33V ±2%               │
│                                  │
│ Alternatif:                      │
│ R1 = 10kΩ, R2 = 3.2kΩ            │
│ → Vout = 3.30V (exact)           │
│                                  │
│ Ince Ayar (opsiyonel):           │
│ R2 position'u 5kΩ potansiyometre │
│ ile değiştir: 3V—3.5V aralığı    │
└──────────────────────────────────┘

Elektronik Bileşenler Listesi:
- **Modul:** XL6009 Step-Down Konverter (eBay/AliExpress)
- **R1:** 10kΩ ±1% (0402 SMD veya 1/4W through-hole)
- **R2:** 3.3kΩ ±1% (0402 SMD veya 1/4W through-hole)
- **C_in:** 10µF/16V elektrolit + 0.1µF seramik
- **C_out:** 100µF/16V elektrolit + 10µF seramik
- **L (Induktor):** 22-47µH (modülde çoğunlukla hazır)
- **D1 (Schottky):** Modülde genelde tanımlanmış
- **Kaynaklar:** PCB pads veya kros-kaynakla lehim

Kurulum Adımları:
1. XL6009 modülsürüyü kumanda kapsünü içine yapışkan bantla yapıştır
2. +12V (kırmızı) ve GND (siyah) bağlantılarını kontrol et
3. Çıkış kapasitörlerini (+, GND) kontrol et
4. R1 ve R2 dirençlerini FB pads'ine lehimle:
   - R1 (10k): Vin tarafına
   - R2 (3.3k): GND tarafına
5. Yüksüz ölçüm: FB—GND arasında 0.8V—1.2V olmalı
6. Hafif yük (100mA LED): Vout ≈ 3.3V (multimetre ile kontrol)
7. Tam yük (500mA+): Voltaj denetimi, ripple kontrolü

Ayarlama ve Fine-Tuning:
- Eğer Vout < 3.3V: R1 biraz büyült (10k → 11k/12k) veya R2 küçült
- Eğer Vout > 3.3V: R1 küçült (10k → 8.2k) veya R2 büyült
- Potansiyometre varsa: R2 yerine 3k—5k değişken direnç kullan

Beklenen Performans:
- Load Regulation: ±2% (0mA → 2A)
- Line Regulation: ±1% (9V → 13V input)
- Ripple Voltage: <50mVpp @ 100mA load (20-100kHz)
- Noise: ~5mVrms @ 100kHz—1MHz
- Thermal: <50°C @ normal kullanım

Güvenlik Notu:
- ⚠️ Ters polarite koruması yok! Bağlantıyı çift kontrol et
- ⚠️ 12V direkt kısa devre risk yüksek—fuseli devre ekstansıyonu düşün (5A)
- ⚠️ Her pil takısında voltaj ölçümü yap (battery monitor)
```

---

## � Haberleşme Mimarisi (RX Tarafı)

### 3-Bileşen RF Kumanda Sistemi

```
KUMANDA (TX)                        UÇAK (RX)
═════════════════════════════════════════════════════════════

ESP32-S3                           Orange Cube (Pixhawk 4)
├─ SX1280 ──[2.4GHz RF]─────────→  \
└─ OLED                             ├─ SBUS IN ← ESP32-S3
                                    ├─ 14x PWM Servo OUT
                                    └─ MAVLink ← Jetson Orin
                                    
                                    Jetson Nano Orin
                                    └─ Vision/AI Compute
```

**Haberleşme Akışı:**
1. TX kumanda: 2 Joystick + 3 Trim pot + 2 switch → ESP32 TX
2. ESP32 TX: SX1280 ile 2.4GHz RF paketleri gönderir (50Hz)
3. ESP32 RX: SX1280 ile RF paketleri alır → SBUS protokolü oluştur
4. Orange Cube: SBUS sinyali → 14x PWM servo çıkışı
5. Jetson Nano: Orange Cube'dan MAVLink telemetri (opsiyonel)

---

## 🔌 SBUS Protokolü (ESP32-RX → Orange Cube)

| Parametre | Değer | Açıklama |
|-----------|-------|----------|
| Frekans | 2400 MHz | ISM Bandı (2.4 GHz) |
| Modülasyon | LoRa | Uzun Menzil, Düşük Enerji |
| TX Gücü | +20 dBm | Maksimum (PA+LNA) |
| Bant Genişliği | 812 kHz | Orta genişlik |
| Yayılma Faktörü | 7 | LoRa SF7 |
| Paket Boyutu | 32 bytes | TX kontrol komutları |
| TX Hızı | 50 Hz | 20ms aralıklar |
| Sync ID | 0xABCD | Paket kimliği |

---

## 🎮 Kontrol Girdileri Harita

```
TX Kumanda Paneli:
┌────────────────────────────────────┐
│   JOYSTICK1        │     TRIM POT   │
│   (Roll/Pitch)     │   (Merkez Ayar)│
│    GPIO1, GPIO2    │ GPIO42,41,40   │
│                    │                │
│  Left Stick:       │  Roll Trim     │
│    ├─ Roll (X)     │  Pitch Trim    │
│    └─ Pitch (Y)    │  Yaw Trim      │
│                    │                │
│   JOYSTICK2        │   SWITCHES     │
│  (Yaw/Throttle)    │  (Modes)       │
│   GPIO3, GPIO4     │ GPIO16,17      │
│                    │                │
│  Right Stick:      │  Flight Mode   │
│    ├─ Yaw (X)      │  Trim Lock     │
│    └─ Throttle (Y) │                │
└────────────────────────────────────┘

RX Servo Çıkışları:
┌──────────────────────────────────┐
│ Motor/ESC ← CH1 (GPIO14)          │
│ Rudder    ← CH2 (GPIO13)          │
│ Aileron   ← CH3 (GPIO12)          │
│ Elevator  ← CH4 (GPIO11)          │
│ Aux1      ← CH5 (GPIO37)          │
│ Aux2      ← CH6 (GPIO36)          │
│ Aux3      ← CH7 (GPIO35)          │
└──────────────────────────────────┘
```

---

## 🔍 Seri Haberleşme (Debugging)

```
TX & RX Seri Port Ayarları:
- Baud Rate: 115200 bps
- Veri Bitleri: 8
- Stop Bitleri: 1
- Parite: None
- Akış Kontrolü: None

Bağlantı (USB):
USB-C ← ESP32-S3 GPIO0 (RXD), GPIO43 (TXD)

Debugging Seçenekleri:
- Serial Monitor (PlatformIO)
- Arduino IDE Serial Monitor
- RealTerm / PuTTY
- VS Code Serial Monitor Eklentisi
```

---

## 📏 ESP32-S3 Pin Diyagramı

```
              ┌─────────────────────┐
         EN  ─┤1                  40├─ GND
       SVDD  ─┤2                  39├─ 3V3
        3V3  ─┤3                  38├─ IO46
       10k Ω─┬┤                      ├─
        GND  ─┤4                  37├─ IO45
      (USB)  ─┤5  ESP32-S3-N16R8 36├─ IO44
       GND   ─┤6  (240MHz, Dual) 35├─ IO43 (TXD)
        IO0  ─┤7      16MB Flash  34├─ IO42 (TRIM_ROLL)
        GND  ─┤8      8MB PSRAM   33├─ IO41 (TRIM_PITCH)
       GND   ─┤9                  32├─ IO40 (TRIM_YAW)
        IO1  ─┤10    [Pin Layout]31├─ IO39
        IO2  ─┤11                 30├─ IO38
        IO3  ─┤12                 29├─ IO37 (CH6_PWM)
        IO4  ─┤13                 28├─ IO36 (CH5_PWM)
        IO5  ─┤14                 27├─ IO35 (CH7_PWM)
        IO6  ─┤15    [SPI]        26├─ IO34
        IO7  ─┤16    MOSI:11      25├─ IO33
                    MISO:13
        IO8  ─┤17    CLK:12       24├─ IO32
        IO9  ─┤18    CS:10        23├─ IO31
       IO10 ─┤19    [I2C]        22├─ IO30
       IO11 ─┤20    SDA:9        21├─ IO29
       GND  ─┤     SCL:8              ├─
              └─────────────────────┘
```

---

## ✅ Donanım Kontrol Listesi (Assembly)

- [ ] ESP32-S3-N16R8 iki adet
- [ ] SX1280 E28-2G4M12S RF Modülü iki adet
- [ ] SSD1306 0.96" OLED Ekran (I2C, 128×64)
- [ ] 2× Analog Joystick (PS5 tarzı)
- [ ] 3× 10kΩ Potansiyometre (Roll, Pitch, Yaw Trimler)
- [ ] Uçuş Modu Anahtarı (Toggle/3-way)
- [ ] Trim Lock Anahtarı (2-way toggle)
- [ ] 27kΩ + 10kΩ Direnç (Voltaj Bölücü)
- [ ] **XL6009 Step-Down Konverter Modülü**
- [ ] 10kΩ ±1% + 3.3kΩ ±1% (XL6009 Feedback)
- [ ] 8x AA Alkaline Pil Tutuşu (12V nominal)
- [ ] 5A Fuse + Fuse Holder (güvenlik)
- [ ] ESP32 Servo Kılıfı (DuPont) - 7x çevrim
- [ ] SPI/I2C Jumper Kablolar (22 AWG)
- [ ] RF Anten 2.4GHz (2 adet, 12cm dipole)
- [ ] USB-C Kablo (programlama & debug için)
- [ ] LED Göstergesi (durum, status)
- [ ] Yapışkan Bant + Shrink Tubing (yazılılama)

---

## 🛠️ Hata Ayıklama

**Sorunu:** RF İletişim Yok
- ✅ SPI bağlantılarını kontrol et (10,7,6,5)
- ✅ RESET pini vs. sürücü voltajını kontrol et
- ✅ Antenaleri kontrol et (doğru frekansta mı?)

**Sorunu:** Servo Çıkışları Çalışmıyor
- ✅ PWM pinlerini kontrol et (11-14, 35-37)
- ✅ Servo kütüphanesini kontrol et

**Sorunu:** Ekran Görmüyor
- ✅ I2C adresi (0x3C) kontrol et
- ✅ SDA/SCL pinleri (GPIO9/8) kontrol et
- ✅ Pull-up dirençler (var mı?)

**Sorunu:** ADC Joystick Değerleri Bozuk
- ✅ Joystick kalibrasyonunu çalıştır
- ✅ Analog pin bağlantılarını kontrol et

---

**Son Güncelleme:** Mart 2026  
**Versiyon:** 1.0 (ESP32-S3 ile Optimize Edilmiş)
