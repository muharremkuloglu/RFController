# Pin Tanımlamaları - RX (İHA Alıcısı)

## Donanım Özeti
```
ESP32 DevBoard
├── Batarya (3S LiPo)
├── Step-down Converter → 5V/3.3V
├── SX1280 RF Modülü (E28-2G4M12S)
├── 7x Servo/ESC Motor (PWM Output)
├── Status LED (GPIO 12)
└── Serial Debug (UART)
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
| DIO1 | GPIO 2 | Interrupt (RX Done) |
| GND | GND | Ground |
| 3V3 | 3V3 | Power (3.3V) |

## Servo Çıkışları (PWM)

| Kanal | Servo/ESC | GPIO | PWM Aralığı | Açıklama |
|-------|-----------|------|------------|----------|
| 1 | Motor ESC | GPIO 14 | 1000-2000 µs | Throttle/Motor Control |
| 2 | Yaw Servo | GPIO 13 | 1000-2000 µs | Yaw (Başlık) |
| 3 | Roll Servo | GPIO 12 | 1000-2000 µs | Roll (Yatış) |
| 4 | Pitch Servo | GPIO 27 | 1000-2000 µs | Pitch (Eğim) |
| 5 | Aux1 Servo | GPIO 25 | 1000-2000 µs | Yardımcı 1 |
| 6 | Aux2 Servo | GPIO 26 | 1000-2000 µs | Yardımcı 2 |
| 7 | Aux3 Servo | GPIO 11 | 1000-2000 µs | Yardımcı 3 |

**PWM Özellikleri:**
- Frekans: 50 Hz (20ms period)
- Min: 1000 µs (Min)
- Center: 1500 µs (Neutral)
- Max: 2000 µs (Max)

## Status LED

| GPIO | Açıklama | Blink Pattern |
|------|----------|--------------|
| GPIO 12 | Status LED | Yavaş = Normal, Hızlı = Failsafe |

**LED Davranışı:**
- Normal: Blink 500ms aralığı (OK)
- Failsafe: Blink 250ms aralığı (Paket kaybı!)

## SPI Ayarları

- **Clock Hızı:** 10 MHz
- **Maksimum:** 16 MHz
- **Mode:** SPI_MODE0

## I/O Özeti

| GPIO | Kullanım | Yön |
|------|----------|-----|
| GPIO 14 | Motor PWM | Out |
| GPIO 13 | Yaw PWM | Out |
| GPIO 12 | Roll PWM | Out |
| GPIO 27 | Pitch PWM | Out |
| GPIO 25 | Aux1 PWM | Out |
| GPIO 26 | Aux2 PWM | Out |
| GPIO 11 | Aux3 PWM | Out |
| GPIO 15 | Status LED | Out |
| GPIO 17 | SX1280 RES | Out |
| GPIO 4 | SX1280 BUSY | In |
| GPIO 18 | SPI SCLK | Out |
| GPIO 23 | SPI MOSI | Out |
| GPIO 19 | SPI MISO | In |
| GPIO 5 | SPI NSS | Out |

## Failsafe Değerleri

| Kanal | Failsafe PWM | Açıklama |
|-------|--------------|----------|
| Motor | 1000 µs | Tamamen kapalı (SAFE!) |
| Yaw | 1500 µs | Merkez pozisyon |
| Roll | 1500 µs | Merkez pozisyon |
| Pitch | 1500 µs | Merkez pozisyon |
| Aux1-3 | 1500 µs | Merkez pozisyon |

**Failsafe Trigger:** 500ms veri alınmazsa aktivasyonu

## Batarya Bağlantısı

- **Voltaj:** 3S LiPo (11.1V - 12.6V)
- **Step-down:** → 5V (Servolar için)
- **Step-down:** → 3.3V (ESP32 & SX1280 için)
- **Besteleme:** GND ← düz bağlantıya dönüştür

## UART Debug

- **Baud Rate:** 115200
- **Port:** Serial (GPIO 1/3 default)
- **Format:** 8N1

## Elektrik Özellikleri

- **ESP32 Çıkış:** 12mA per GPIO (max)
- **Servo Çıkışı:** PWM logic level (3.3V)
- **LED Reddetme Çıkışı:** 12mA max @ 3.3V
- **Tüm Çıkışlar:** 3.3V logic level

---

**En Önemli Notlar:**
1. ⚠️ Motor PWM (GPIO 4) → ESC → Motor
2. ⚠️ Failsafe: Motor otomatik kapalı (1000µs)
3. ⚠️ 500ms paket timeout → Failsafe aktivasyon
4. ⚠️ Status LED: Hızlı blink = Sorun!

---

**Status:** ✅ Kayıtsız
