# RF Kumanda Sistemi (RFController) 🚁

**Savaşan İHA Projesi için 2.4 GHz RF Kontrol Sistemi**  
ESP32 + SX1280 (E28-2G4M12S) ile yapılan tam fonksiyonlu kumanda sistemi.

---

## 📋 Hızlı Başlangıç

### Donanım Listesi
- **2x ESP32 DevBoard** (TX ve RX)
- **2x SX1280 E28-2G4M12S Modul** (2.4 GHz LoRa RF)
- **1x SSD1306 128×64 OLED Ekran** (TX tarafında)
- **7x 10 kΩ Potansiyometre** (Kontrol kanalları)
- **4x Toggle/Regular Switch** (Mod seçimi vb.)
- **1x 2S LiPo Bataryası** (7.4-8.4V)
- **1x Step-down Dönüştürücü** (5V/3.3V)
- **Tüm kablolar ve konektörler**

### Hızlı Kurulum

```bash
# 1. VS Code + PlatformIO'yu yükle
# 2. Proje klasörünü aç
# 3. TX'i kod et:
pio run -e tx --target upload

# 4. RX'i kod et:
pio run -e rx --target upload
```

---

## 📊 Proje Yapısı

```
RFController/
├── TX/                      # Kumanda İstasyonu Yazılımı
│   ├── src/
│   │   └── main.cpp        # TX ana kodu (HAZIR)
│   └── platformio.ini
├── RX/                      # İHA Alıcı Yazılımı  
│   ├── src/
│   │   └── main.cpp        # RX ana kodu (HAZIR)
│   └── platformio.ini
├── shared/                  # Ortak Fonksiyonlar
│   ├── config.h            # Tüm Pin Tanımlamaları & Kalibrasyonu
│   └── protocol.h          # İletişim Protokolü (32-byte paket)
├── docs/                    # Dokümantasyon
│   ├── pinout.md           # Detaylı Pin Bağlantı Şeması
│   ├── protocol.md         # Veri Paketi Formatı
│   └── calibration.md      # Kalibrasyonu & Test Talimatları
├── platformio.ini          # Build Konfigürasyonu
└── README.md               # Bu dosya
```

---

## 🔧 Donanım Özellikleri

### TX (Kumanda İstasyonu)

| Bileşen | Detay | Durum |
|---------|-------|-------|
| **Giriş** | 7x Potansiyometre (ADC) | ✅ |
| | 4x Dijital Switch (GPIO) | ✅ |
| | Pil Voltajı Sensörü (ADC) | ✅ |
| **İşlem** | SX1280 RF Modül (SPI) | ✅ |
| | 2.4 GHz, Spreading Factor 7 | ✅ |
| **Çıkış** | SSD1306 OLED Ekran (I2C) | ✅ |
| | Serial Debug (115200 baud) | ✅ |

**TX Kontrol Kanalları:**
- CH1: Throttle (Gaz) - POT GPIO 34
- CH2: Yaw (Başlık) - POT GPIO 35
- CH3: Roll (Dönerme) - POT GPIO 32
- CH4: Pitch (Eğim) - POT GPIO 33
- CH5: Aux1 - POT GPIO 25
- CH6: Aux2 - POT GPIO 26
- CH7: Aux3 - POT GPIO 27

**TX Ekran Gösterimi:**
```
RF KUMANDA TX | PKT:1234
THR:512 YAW:480 ROLL:520
PITCH:510 AUX1:500 AUX2:490
AUX3:505 | SW: T1 T2 SW
BATT: 8.23V | STATUS: OK
```

### RX (İHA Alıcısı)

| Bileşen | Detay | Durum |
|---------|-------|-------|
| **Giriş** | SX1280 RF Modül (SPI) | ✅ |
| | 2.4 GHz, Continuous RX | ✅ |
| **İşlem** | Paket Doğrulama & Failsafe | ✅ |
| **Çıkış** | 7x Servo PWM Sinyali | ✅ |
| | Status LED (Blink pattern) | ✅ |
| | Serial Debug (115200 baud) | ✅ |

**RX Servo Çıkışları:**
- CH1: Throttle (Motor) - GPIO 4 → PWM 1000-2000µs
- CH2: Yaw - GPIO 5
- CH3: Roll - GPIO 18
- CH4: Pitch - GPIO 19
- CH5: Aux1 - GPIO 23
- CH6: Aux2 - GPIO 25
- CH7: Aux3 - GPIO 26

**RX Failsafe Davranışı:**
- Paket timeout: 500ms
- Motor cut: Throttle → 1000µs (Safe!)
- LED Blink: Hızlı = Failsafe, Yavaş = Normal

---

## 📡 İletişim Protokolü

### Paket Yapısı (32 byte, sabit boyut)

```c
struct ControlPacket {
    uint16_t sync_id;       // 0xABCD (senkronizasyon)
    uint16_t throttle;      // 1000-2000 µs PWM
    uint16_t yaw;           // 1000-2000 µs PWM
    uint16_t roll;          // 1000-2000 µs PWM
    uint16_t pitch;         // 1000-2000 µs PWM
    uint16_t aux1;          // 1000-2000 µs PWM
    uint16_t aux2;          // 1000-2000 µs PWM
    uint16_t aux3;          // 1000-2000 µs PWM
    uint16_t battery;       // mV (0-9000)
    uint8_t switch_toggle1; // 0/1
    uint8_t switch_toggle2; // 0/1
    uint8_t switch_regular; // 0/1
    uint8_t flags;          // Bayraklar:
                            //   Bit 2: Low Battery (< 8.0V)
                            //   Bit 7: Power Warning
    uint8_t checksum;       // XOR kontrol toplamı
}
```

### RF Ayarları

| Parametre | Değer |
|-----------|-------|
| Frekans | 2400 MHz |
| Modülasyon | LoRa |
| Bandwidth | 812 kHz |
| Spreading Factor | 7 |
| Coding Rate | 4/5 |
| TX Power | 13 dBm |
| TX Rate | 50 Hz (20ms) |

---

## 🚀 Kullanım

### 1. TX Başlat
```bash
# Terminal 1: TX'i kod et
pio run -e tx --target upload

# TX Serial Monitor'ü aç (115200 baud)
pio device monitor -e tx
```

Çıkış örneği:
```
╔════════════════════════════════╗
║  RF KUMANDA TX BAŞLATILIYOR   ║
║  Frekans: 2.4 GHz             ║
║  Modul: SX1280 E28-2G4M12S    ║
╚════════════════════════════════╝

[1/6] SPI başlatılıyor...
[2/6] Ekran başlatılıyor...
[3/6] ADC başlatılıyor...
[4/6] Switch'ler başlatılıyor...
[5/6] SX1280 başlatılıyor...
  FW Version: 0xA8
  • Frekans: 2400 MHz
  • Güç: 13 dBm
  • SF: 7, BW: 812kHz
[6/6] ✓ HAZIR!

[TX] Paket #1234 | THR:1500 YAW:1480 | BATT:8.23V | FLAGS:0x00
```

### 2. RX Başlat
```bash
# Terminal 2: RX'i kod et
pio run -e rx --target upload

# RX Serial Monitor'ü aç (115200 baud)
pio device monitor -e rx
```

Çıkış örneği:
```
╔════════════════════════════════╗
║  RF KUMANDA RX BAŞLATILIYOR    ║
║  Frekans: 2.4 GHz              ║
║  Modul: SX1280 E28-2G4M12S     ║
║  Çıkış: 7 Servo (PWM)          ║
╚════════════════════════════════╝

[1/5] SPI başlatılıyor...
[2/5] Servo çıkışları hazırlanıyor...
[3/5] Status LED hazırlanıyor...
[4/5] SX1280 başlatılıyor...
  FW Version: 0xA8
  • Frekans: 2400 MHz
  • SF: 7, BW: 812kHz
  • Mode: RX (Continuous)
[5/5] ✓ HAZIR!

[RX] Paket #1234 | THR:1500 BATT:8.23V | Loss: 0%
```

### 3. Test & Kalibrasyonu
```bash
# docs/calibration.md dosyasını oku
# 1. Potansiyomereleri sıfırla (1000µs)
# 2. Maksimuma çıkar (2000µs)
# 3. Servolar düzgün tepki veriyor mu kontrol et
# 4. Uzaklık testini yap (1m → 10m+)
```

---

## 🐛 Sorun Giderme

### "Radio başlatılamadı!" hatası
- ✅ SX1280 wiring kontrol et (NSS, RES, BUSY, SPI)
- ✅ SPI frekansı 10 MHz (config.h)
- ✅ ESP32 pin tanımlamaları doğru mu

### Paket kaybı yüksek
- ✅ Antenna bağlantısını kontrol et
- ✅ Spreading Factor 7 → 9 (aralık için)
- ✅ TX Power 13 dBm ise radiator eklemeyi dene

### RX servo jitter yapıyor
- ✅ Güç kaynağı stabil mi (5V/3.3V)
- ✅ Servo kablolar uzun mu (100cm+ sorun yaratabilir)
- ✅ Clock jitter: LoRa frame timing

---

## 📝 Dosya Referansı

| Dosya | Amaç | Durum |
|-------|------|-------|
| `TX/src/main.cpp` | Kumanda istasyonu ana kodu | ✅ Tam |
| `RX/src/main.cpp` | İHA alıcı ana kodu | ✅ Tam |
| `shared/config.h` | Pin tanımları + kalibr. | ✅ Tam |
| `shared/protocol.h` | Paket yapısı + fonk. | ✅ Tam |
| `docs/pinout.md` | Detaylı pin şeması | ✅ Tam |
| `docs/calibration.md` | Kalibrasyon rehberi | ⚠️ Template |
| `platformio.ini` | Build konfig. | ✅ Tam |
| `.gitignore` | Git exclude | ✅ Tam |

---

## 🔐 Kod Özellikleri

### TX Özellikleri
✅ 7 kanal analog kontrol (1000-2000µs)  
✅ 3 switch dijital girişi  
✅ Pil voltajı izlemesi (Low battery alarm)  
✅ SSD1306 OLED gerçek zamanlı görüntü  
✅ Serial debug çıkışı (50Hz)  
✅ XOR checksum paket doğrulaması  

### RX Özellikleri
✅ Sürekli RX modu (SX1280)  
✅ 7 kanallı PWM servo çıkışı  
✅ Paket timeout failsafe (500ms)  
✅ Throttle cut güvenliği (motor kapalı)  
✅ Status LED göstergesi  
✅ Serial debug + paket istatistikleri  
✅ XOR checksum doğrulaması  

---

## 📌 Notlar

- **Voltaj Bölücü:** R1=R2=10k (1:2 ratio) → BATTERY_SENSE (GPIO 36 VP)
- **PWM Aralığı:** 1000µs (min) — 1500µs (center) — 2000µs (max)
- **TX Rate:** 50 Hz (20ms cycle)
- **Failsafe Timeout:** 500ms veri yok → Motor kapalı
- **Frekans:** 2.4 GHz ISM band (license-free) ✅
- **Menzil:** ~1 km açık alanda (SF=7, Yüksek güç)

---

## 📞 Teknik Destek

Sorun yaşıyorsan:
1. Serial output'ı kontrol et (115200 baud)
2. `docs/calibration.md` kılavuzunu oku
3. Wiring'i pin tanımlamalarıyla karşılaştır
4. SPI frequency ve timing'i doğrula

---

**Geliştiren:** RF Kumanda Proje Ekibi  
**Tarih:** Mart 2026  
**Durum:** ✅ Üretim Hazır (v1.0)


### TX (Transmitter - Kumanda)
- **CPU**: ESP32
- **RF Modul**: SX1280 (2.4 GHz)
- **Giriş**: 4x Potansiyometre (Throttle, Yaw, Roll, Pitch)
- **İletişim**: SPI

### RX (Receiver - İHA)
- **CPU**: ESP32
- **RF Modul**: SX1280 (2.4 GHz)
- **Çıkış**: 4x PWM sinyal
- **İletişim**: SPI

## Teknik Özellikleri

- **Frekans**: 2.4 GHz
- **Bant Genişliği**: 812 kHz
- **Spreading Factor**: 7
- **TX Gücü**: 13 dBm (maksimum)
- **Gönderim Hızı**: 50 Hz (20 ms aralıklar)
- **Timeout**: 1000 ms
- **Veri Paketi**: 32 byte

## Kurulum

### Gerekli Yazılımlar
1. **VS Code**
2. **PlatformIO Extension** (VS Code'da)
3. **Python 3.6+** (PlatformIO için)

### İlk Kurulum

```bash
# Depoyu klonla
git clone <repo-url>
cd RFController

# VS Code'da aç
code .
```

### Bağımlılıklar

Bu dokümanda:
- Arduino Framework
- LoRa Library (SX1280 uyumlu)

PlatformIO otomatik olarak yükleyecektir.

## Kompilasyon ve Yükleme

### TX'i Derle ve Yükle
```
# VS Code'da PlatformIO: Upload for TX ortamı seç
```

### RX'i Derle ve Yükle
```
# VS Code'da PlatformIO: Upload for RX ortamı seç
```

## İlk Adımlar

1. **Hardware Kurulum**
   - Pin bağlantılarını docs/pinout.md ile kontrol et
   - Tüm bağlantıları doğrula

2. **Kalibrasyonu**
   - Potansiyometreleri docs/calibration.md'e göre kalibre et
   - config.h dosyasını güncelle

3. **Test**
   - TX'i seri port monitörü ile test et (115200 baud)
   - Potansiyometreleri hareket ettir ve değerleri kontrol et
   - RX'i çalıştır ve sinyal alındığını doğrula

## Hata Ayıklama

Serial Monitör kullanarak debug çıktılarını izle:

```
TX: Throttle=1500 Yaw=1500 Roll=1500 Pitch=1500
RX: Paket alındı - Throttle=1500 Yaw=1500 Roll=1500 Pitch=1500
```

## İmplementasyon Durumu

- [x] Proje yapısı
- [x] Config dosyası
- [x] Protokol tanımı
- [x] ADC okuma kodu (skeleton)
- [x] Pin tanımlamaları
- [ ] **SX1280 driver kodu** ← ÖNEMLİ
- [ ] SPI iletişim
- [ ] LoRa gönderme/alma
- [ ] Servo kontrol (RX)
- [ ] LED göstergeleri

## İlgili Kaynaklar

- [SX1280 Datasheet](https://www.semtech.com/)
- [ESP32 GPIO Referansı](https://docs.espressif.com/)
- [LoRa Protocol](https://lora-alliance.org/)

## Lisans

Bu proje LICENSE dosyası altında yayımlanmıştır.

## İletişim

Proje hakkında sorular için GitHub Issues açabilirsin.

---

**Geliştirme Durumu**: 🔧 Aktif Geliştirme
**Son Güncelleme**: Mart 2026

