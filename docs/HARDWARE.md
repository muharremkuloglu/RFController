# 🔧 Donanım Şeması - RF Kumanda Sistemi

**Proje:** Savaşan İHA için 2.4 GHz RF Kumanda Sistemi  
**Kumanda (TX):** ESP32-S3-N16R8 + SX1280 RF + OLED  
**Uçak (RX) - Ana Bilgisayar:** Pixhawk 4 Orange Cube (Autopilot)  
**Uçak (RX) - Görüş:** NVIDIA Jetson Nano Orin (AI/Vision)  
**Uçak (RX) - RF Bridge:** ESP32-S3-N16R8 + SX1280 → SBUS  
**Enerji Kaynağı:** 8x AA Alkaline (12V, Kumanda)

---

## 📌 Cihaz Bloğu (Block Diagram)

```
┌─────────────────────────────────────────────────────────────┐
│                    KUMANDA İSTASYONU (TX)                   │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │            ESP32-S3-N16R8 (240 MHz)                 │  │
│  │          16MB Flash, 8MB PSRAM (Dual-Core)          │  │
│  └──────────────────────────────────────────────────────┘  │
│          ↑           ↑           ↑           ↑              │
│         SPI         I2C        GPIO        ADC              │
│          │           │          │           │              │
│    ┌─────┴─────┐ ┌──┴──┐  ┌─────┴──┐  ┌────┴─────┐       │
│    │ SX1280    │ │OLED │  │Switches│  │Joysticks │       │
│    │  2.4GHz   │ │0.96"│  │ 2x    │  │ 2x      │       │
│    │ PA+LNA    │ │ LCD │  │        │  │         │       │
│    │ +20 dBm   │ │ I2C │  │GPIO16:17│ │GPIO1-4 │       │
│    └─────┬─────┘ └─────┘  └────────┘  └────────┘       │
│          │                                                  │
│      RF Antenna                                            │
│      @ 2.4 GHz                                            │
│                                                              │
│  Trim POT Girişleri (ADC):                                │
│  ├─ GPIO42: Roll Trim                                    │
│  ├─ GPIO41: Pitch Trim                                   │
│  ├─ GPIO40: Yaw Trim                                     │
│  └─ GPIO8: Pil Voltajı Sensörü                          │
│                                                              │
│  Enerji: 2S LiPo (7.4-8.4V) → 5V/3.3V Step-down         │
└─────────────────────────────────────────────────────────────┘
                        ↓ RF Signal ↓
┌─────────────────────────────────────────────────────────────┐
│                      İHA ALICISI (RX)                       │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │            ESP32-S3-N16R8 (240 MHz)                 │  │
│  │          16MB Flash, 8MB PSRAM (Dual-Core)          │  │
│  └──────────────────────────────────────────────────────┘  │
│          ↑              ↑                                    │
│         SPI            PWM                                  │
│          │              │                                    │
│    ┌─────┴─────┐   ┌────┴──────────────────┐              │
│    │ SX1280    │   │ Servo Motor Çıkışları │              │
│    │  2.4GHz   │   │  7x PWM Ch (GPIO11-14 │              │
│    │ PA+LNA    │   │        35-37)         │              │
│    │ +20 dBm   │   │                        │              │
│    └─────┬─────┘   └────────┬───────────────┘              │
│          │                  │                                │
│      RF Antenna        Throttle, Pitch, Roll,             │
│      @ 2.4 GHz       Yaw, Aux1, Aux2, Aux3               │
│                                                              │
│  Status LED: GPIO15 (Durum göstergesi)                    │
│  Enerji: Pil kutusundan (LiPo)                           │
└─────────────────────────────────────────────────────────────┘
```

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
| BATT_SENSE | GPIO 8 | Voltaj Ölçümü | Gerilim Bölücü (2.0x) |

#### GPIO - Anahtarlar
| Fonksiyon | Pin # | Tip | Açıklama |
|-----------|-------|-----|----------|
| FLIGHT_MODE | GPIO 16 | Dijital Input | AUTO ↔ FBWA Geçişi |
| TRIM_LOCK | GPIO 17 | Dijital Input | Trim Kilidi (Yanlışlıkla Değişimi Engelle) |

#### Güç Kaynağı
| Pin | Voltaj | Akım | Açıklama |
|-----|--------|------|----------|
| VCC | 3.3V | 500mA (max) | ESP32-S3 Beslemesi |
| GND | 0V | - | Referans |
| VBAT (opsiyonel) | 5V | 1A (max) | Harici Cihazlar |

---

### **RX (İHA Alıcısı) - ESP32-S3 Pinleri**

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

#### PWM Çıkışları - Servo Kanalları (7 Kanal)
| Kanal # | Pin # | Fonksiyon | Min PWM | Max PWM | Merkez |
|---------|-------|-----------|---------|---------|--------|
| CH1 | GPIO 14 | Throttle/Motor | 1000 µs | 2000 µs | 1500 µs |
| CH2 | GPIO 13 | Yaw/Rudder | 1000 µs | 2000 µs | 1500 µs |
| CH3 | GPIO 12 | Roll/Aileron | 1000 µs | 2000 µs | 1500 µs |
| CH4 | GPIO 11 | Pitch/Elevator | 1000 µs | 2000 µs | 1500 µs |
| CH5 | GPIO 37 | Auxiliary 1 | 1000 µs | 2000 µs | 1500 µs |
| CH6 | GPIO 36 | Auxiliary 2 | 1000 µs | 2000 µs | 1500 µs |
| CH7 | GPIO 35 | Auxiliary 3 | 1000 µs | 2000 µs | 1500 µs |

#### GPIO - Durum ve İndikatorler
| Fonksiyon | Pin # | Tip | Açıklama |
|-----------|-------|-----|----------|
| STATUS_LED | GPIO 15 | Çıkış (LED) | Durum göstergesi (Hızlı blink = hata) |
| GND | GND | Referans | RF iletişim hassas GND |

---

## 🔋 Enerji Yönetimi - 8x AA Alkaline (12V Sistem)

### Kumanda İstasyonu (TX)

```
8x AA Alkaline Batarya (12V nominal)
    ↓
[Step-down Konverter: 12V → 5V/3.3V]
    ├─→ ESP32-S3 (3.3V @ 500mA)
    ├─→ SX1280 RF Modülü (3.3V @ 300mA peak TX sırasında)
    ├─→ OLED Ekran (3.3V @ 20mA)
    ├─→ Joystickler & Potansiyometreler (3.3V @ 10mA)
    └─→ Anahtarlar (3.3V @ 5mA)

Gerilim Bölücü (Battery Monitoring):
- R1 = 27kΩ, R2 = 10kΩ → GPIO8 (ADC)
- 12V giriş → 3.24V ADC → 4023 okuması

AA Alkaline Özellikleri:
- Nominal: 1.5V/cell
- 8x seri = 12.0V nominal
- Boş voltaj: 0.9V/cell = 7.2V total
- Uyarı seviyesi: 1.0V/cell = 8.0V total

Tahmini Akım Tüketimi (Normal Çalışma):
┌─────────────────────────────────┐
│ Bekleme: 40-50mA (ESP32 sleep)  │
│ Normal: 120-180mA (RF working)  │
│ Peak: 400-500mA (TX moment)     │
└─────────────────────────────────┘

Pil Ömrü Hesaplaması (AA Alkaline: ~2000mAh):
- Hafif kullanım: 2000mAh / 100mA = 20 saat
- Normal kullanım: 2000mAh / 150mA = 13 saat  
- Ağır kullanım: 2000mAh / 250mA = 8 saat
- Peak nöbeti: 2000mAh / 400mA = 5 saat

Konverter Spesifikasyonları (XL6009):
┌──────────────────────────────────────────┐
│ Model: XL6009 Step-Down Buck Converter   │
│ Input: 5V-32V (12V nominal)              │
│ Output: 3.3V (ayarlanabilir)             │
│ Max Current: 2A                          │
│ Efficiency: ~92%                         │
│ Frequency: 180-400kHz                    │
└──────────────────────────────────────────┘

Feedback Direnç Ayarlaması (R1, R2):
- Vout = 0.8V × (1 + R1/R2)
- 3.3V istiyorsak: R1/R2 = 3.125

Önerilen Konfigürasyon:
┌────────────────────────────┐
│ R1 = 10kΩ (Vin tarafında)  │
│ R2 = 3.3kΩ (GND'ye)        │
│ Vout = 3.33V ±1%           │
│ Potansiyometre: 5kΩ (ince) │
└────────────────────────────┘

Devre Bağlantısı:
- Vin = +12V pil
- GND = Pil GND
- Vout = 3.3V (ESP32, SX1280, OLED)
- FB pin = R1 ve R2'nin verişme noktası
```
```

### İHA Alıcısı (RX)

```
8x AA Alkaline Batarya (12V nominal)
    ↓
[Step-down Konverter: 12V → 5V/3.3V]
    ├─→ ESP32-S3 (3.3V @ 500mA)
    ├─→ SX1280 RF Modülü (3.3V @ 300mA peak)
    └─→ 7x Servo Motor Driver (PWM sinyali)

⚠️ ÖNEMLİ: Servo motorları doğrudan 12V pil kutusundan 
          beslenmelidir! Step-down sadece kontrol bilgisayarı için.

Servo Power Budget:
- Per servo: ~100-300mA (hızlıca hareket ederken)
- 7 servo peak: ~2000mA toplam
- Step-down: Sadece ESP32 + SX1280 lojik beslemesi
```

---

## 🔧 Voltaj Bölücü Kalibrasyonu (Battery Sensing - 12V Sistem)

8x AA Alkaline pil sisteminde maksimum **12V** voltaj gelir. ESP32-S3 ADC maksimum **3.3V** giriş kabul eder.

**Voltage Divider Devresi:**

```
        12V (Pil Max)
          │
          ├─[R1: 27kΩ]─┬─ GPIO8 (ADC)
          │            │
          └─[R2: 10kΩ]─┤
          │            │
         GND           GND

Bölme Oranı: R2 / (R1 + R2) = 10k / 37k = 0.270
ADC Giriş: V_ADC = V_BATT × 0.270 = 12V × 0.270 = 3.24V ✓

Gerçek Voltaj Hesaplaması:
- V_BATT = V_ADC / 0.270
- V_BATT = V_ADC × 3.636 (VOLTAGE_DIVIDER = 3.64)

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

## 📊 İletişim Protokolü Özeti

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
