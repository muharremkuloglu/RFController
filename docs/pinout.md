<!--
================================================================================
📍 docs/pinout.md - TX (Kumanda) ESP32-S3 Pin Tanımlamaları
================================================================================

İŞLEV:
  - TX kumanda istasyonunun ESP32-S3 pin haritası
  - SPI (RF), I2C (OLED), ADC (Joystick/Trim), GPIO (Switch) pin detayı

ÖNEMLİ NOT:
  - Şu dosya: shared/config.h'de de tanımlanıyor (DUPLICATE!)
  - SOURCE OF TRUTH: shared/config.h
  - Değişim yapıyorsan: her ikisini güncelle VEYA shared/config.h'yi include et

PIN ÖZET:
  SPI: CS=10, RST=7, BUSY=6, DIO1=5, MOSI=11, MISO=13, CLK=12
  I2C: SDA=9, SCL=8 (OLED addr=0x3C)
  ADC Joystick: 1,2,3,4
  ADC Trim: 42,41,40
  GPIO Switch: 16,17

İLGİLİ DOSYALAR:
  - shared/config.h → MASTER SOURCE
  - docs/HARDWARE.md → Daha detaylı (table format)
  - platformio.ini → Build config

================================================================================ 
-->

# Pin Tanımlamaları - TX Kumanda İstasyonu

## Donanım Özeti
```
ESP32 DevBoard
├── Batarya (2S LiPo)
├── Step-down Converter → 5V/3.3V
├── SX1280 RF Modülü (E28-2G4M12S)
├── SSD1306 OLED Ekran (I2C)
├── 7x Potansiyometre (ADC)
├── 2x Toggle Switch
├── 1x Normal Switch  
├── 1x Power Switch
└── Voltaj Sensörü (GPIO 36)
```

## ESP32 - SX1280 Bağlantısı (SPI)

| SX1280 | ESP32 | Açıklama |
|--------|-------|----------|
| NSS | GPIO 5 | Chip Select |
| MOSI | GPIO 23 | SPI Data In |
| MISO | GPIO 19 | SPI Data Out |
| SCLK | GPIO 18 | SPI Clock |
| RES | GPIO 17 | Reset |
| BUSY | GPIO 4 | Busy Signal |
| DIO1 | GPIO 2 | Interrupt |
| GND | GND | Ground |
| 3V3 | 3V3 | Power |

## Potansiyometreler (ADC)

| Kanal | Adı | GPIO | Aralık | Açıklama |
|-------|---|-------|------|----------|
| 1 | Throttle | GPIO 34 | 1000-2000 µs | Gaz/Motor Kontrol |
| 2 | Yaw | GPIO 35 | 1000-2000 µs | Düşey Eksen (Z) |
| 3 | Roll | GPIO 32 | 1000-2000 µs | Yatış (Y) |
| 4 | Pitch | GPIO 33 | 1000-2000 µs | Baş (X) |
| 5 | Aux 1 | GPIO 25 | 1000-2000 µs | Yardımcı Kanal 1 |
| 6 | Aux 2 | GPIO 26 | 1000-2000 µs | Yardımcı Kanal 2 |
| 7 | Aux 3 | GPIO 27 | 1000-2000 µs | Yardımcı Kanal 3 |

**Potansiyometre Özellikleri:**
- Direnç: 10kΩ
- Besteleme: 3.3V
- ADC Çözünürlüğü: 12-bit (0-4095)

## Pil Voltajı Sensörü (ADC)

| Sensör | GPIO | Not |
|--------|------|------|
| Voltaj ADC | GPIO 36 (VP) | 0-3.3V input |
| Pil Tipi | 2S LiPo | 7.4V - 8.4V |
| Alarm Seviyesi | < 8.0V | Düşük pil uyarısı |

**Voltaj Divider:**
- Giriş: Batarya 7.4V-8.4V
- Çıkış: 3.3V (ADC max)
- Oran: 1:2 (R1=10k, R2=10k)

## Switch'ler (Digital Input - PULLUP)

| Switch | GPIO | Durumu | Açıklama |
|--------|------|--------|----------|
| Toggle 1 | GPIO 12 | HIGH/LOW | Toggle Switch 1 |
| Toggle 2 | GPIO 13 | HIGH/LOW | Toggle Switch 2 |
| Normal | GPIO 14 | HIGH/LOW | Momentary Switch |
| Power | GPIO 15 | HIGH/LOW | Güç On/Off |

**Not:** INPUT_PULLUP modunda, basıldığında LOW olur.

## Ekran (SSD1306 OLED - I2C)

| Bileşen | GPIO | Açıklama |
|---------|------|----------|
| SDA | GPIO 21 | I2C Data |
| SCL | GPIO 22 | I2C Clock |
| Adresi | 0x3C | Standart SSD1306 |
| Besteleme | 3V3 | Power (3.3V) |

**Ekran Özellikleri:**
- Boyut: 128x64 pixel
- Renk: Monochrom (Beyaz/Siyah)
- Protokol: I2C @ 400kHz

## SPI Ayarları

- **Clock Hızı:** 10 MHz (SPI_CLOCK_DIV4)
- **Maksimum:** 16 MHz (SX1280 için)
- **Mode:** SPI_MODE0

## Diğer Ayarlar

- **UART (Debug):** Serial @ 115200 baud (GPIO 1/3)
- **Refresh Hızı:** 50 Hz (TX), 200ms (Display)
- **I2C Hızı:** 400 kHz
