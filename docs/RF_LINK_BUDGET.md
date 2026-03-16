<!--
================================================================================
📡 docs/RF_LINK_BUDGET.md - RF Haberleşme Desibel ve Mesafe Hesaplamaları
================================================================================

İŞLEV:
  - Friis Path Loss formülü ile RF linki analiz
  - TX power, RX sensitivity, antenna gain hesaplaması
  - Line-of-sight menzil (~500m) doğrulaması
  - Frekans etkisi (2.4 GHz vs 2.5 GHz)

KRİTİK PARAMETRELER (SX1280 için):
  TX Power: +20 dBm (100 mW, PA+LNA)
  RX Sensitivity: -98 dBm (SF7, BW 812kHz, 100baud)
  Antenna Gain TX/RX: 0 dBi (dipole antenna, ideal)
  Fade Margin: 10 dB (shadow, obstacle, weather)
  Frekans: 2.4-2.5 GHz (ISM band)

================================================================================
-->

# 📡 RF Link Budget Analizi - Desibel ve Mesafe Hesaplaması

## 1. Temel Kavramlar

### Desibel (dB) Nedir?
```
dBm = 10 × log₁₀(Power_mW)

Örnek:
- 1 mW = 0 dBm
- 10 mW = 10 dBm
- 100 mW = 20 dBm (← SX1280 TX power)
- 1 W = 30 dBm
```

### Path Loss (Yol Kaybı)
Serbest uzayda EM dalgaların zayıflaması:

```
Friis Denklemi:
Path Loss (dB) = 20 × log₁₀(f) + 20 × log₁₀(d) + 20 × log₁₀(4π/c)

Pratik Form:
Path Loss (dB) = 32.45 + 20 × log₁₀(f_MHz) + 20 × log₁₀(d_km)

Örnek (2.4 GHz, 500m):
Path Loss = 32.45 + 20 × log₁₀(2400) + 20 × log₁₀(0.5)
         = 32.45 + 67.6 + (-6.02)
         = 94.03 dB
```

---

## 2. SX1280 Teknik Özellikleri

| Parametre | Değer | Açıklama |
|-----------|-------|----------|
| **TX Power** | +20 dBm (100mW) | Maksimum (PA+LNA) |
| **RX Sensitivity** | -98 dBm | SF7, 812kHz BW @ 100baud |
| **Modülasyon** | LoRa | Uzun menzil, düşük SNR |
| **Bandwidth** | 812 kHz | Orta genişlik |
| **Spreading Factor** | 7 | SF7 (speed-range trade-off) |
| **Frequency Range** | 2.400-2.5 GHz | ISM band (unlicensed) |
| **Antenna (dipole)** | 0 dBi | İdeal antenna gain |

---

## 3. Link Budget Hesaplaması

### Formül
```
RX Power (dBm) = TX Power - Path Loss - Losses + Gains
RX Power = 20 dBm - Path Loss - 3dB (cable) + 0dBi (ANT)

SNR (Signal-to-Noise) = RX Power - RX Sensitivity
Fade Margin = SNR - 10dB (practical reserve)
```

### Örnek: 2.4 GHz, 500m, Line-of-Sight

```
1. Path Loss Hesapla
   Path Loss = 32.45 + 20×log₁₀(2400) + 20×log₁₀(0.5)
            = 32.45 + 67.6 - 6.02
            = 94.03 dB

2. RX Power Hesapla
   RX Power = TX Power - Path Loss - Cable Loss
            = 20 dBm - 94.03 dB - 3 dB
            = -77.03 dBm

3. Sensitivity Check
   RX Sensitivity = -98 dBm
   SNR = -77.03 - (-98) = +20.97 dB
   
4. Fade Margin
   Fade Margin = SNR - 10 dB = 20.97 - 10 = 10.97 dB
   ✅ PASS (marjin yeterli!)
```

---

## 4. Frekans Etkisi (Path Loss)

Path Loss frekansla 20×log₁₀ oranında artar!

```
2.4 GHz vs 2.5 GHz Karşılaştırması (500m):

2.4 GHz:
  Path Loss = 32.45 + 20×log₁₀(2400) + 20×log₁₀(0.5)
           = 32.45 + 67.60 - 6.02 = 94.03 dB

2.5 GHz:
  Path Loss = 32.45 + 20×log₁₀(2500) + 20×log₁₀(0.5)
           = 32.45 + 67.96 - 6.02 = 94.39 dB

Fark: 94.39 - 94.03 = 0.36 dB (minimal)
Sonuç: Frekans 2.4→2.5 GHz değişiminde yalnızca 0.36 dB kayıp!
```

---

## 5. Mesafe vs Path Loss (2.4 GHz)

Path Loss mesafeyle 20×log₁₀ oranında artar:

```
Mesafe (m) | Path Loss (dB) | RX Power (dBm) | SNR (dB) | Status
-----------|----------------|----------------|----------|--------
100        | 80.05          | -63.05         | +34.95   | ✅ GOOD
250        | 88.02          | -71.02         | +26.98   | ✅ GOOD
500        | 94.03          | -77.03         | +20.97   | ✅ GOOD
1000       | 100.04         | -83.04         | +14.96   | ⚠️ MARGINAL
2000       | 106.06         | -89.06         | +8.94    | ❌ POOR
5000       | 114.03         | -97.03         | +0.97    | ❌ FAIL

Fade Margin = SNR - 10 dB

500m'de fade margin = +10.97 dB (yeterli)
1000m'de fade margin = +4.96 dB (düşük, obstrüksiyon risk)
2000m'de fade margin = -1.06 dB (failsafe trigger!)
```

---

## 6. Pratik Hesap Rehberi

### Online Calculator
```
Friis Path Loss Calculator:
https://www.everythingrf.com/rf-calculators/friis-transmission-equation

Input:
- TX Power: 20 dBm
- RX Sensitivity: -98 dBm
- TX Frequency: 2400 MHz
- TX-RX Distance: 500 m
- TX Antenna Gain: 0 dBi
- RX Antenna Gain: 0 dBi
- Transmission Line Loss: 3 dB

Output: Link Margin (dB)
```

### Excel Formula
```
=32.45 + 20*LOG10(2400) + 20*LOG10(0.5)  → Path Loss dB
=20 - (Path Loss) - 3                     → RX Power dBm
=(RX Power) - (-98)                       → SNR dB
```

---

## 7. Link Margin Taflosu

```
Scenario                       | Path Loss | RX Power | SNR  | Margin | Status
-------------------------------|-----------|----------|------|--------|--------
Optimal (100m, LoS, no fade)   | 80.05 dB  | -63 dBm  | +35  | +25 dB | ✅ Excellent
Good (250m, LoS, clear sky)    | 88.02 dB  | -71 dBm  | +27  | +17 dB | ✅ Good
Fair (500m, LoS, some fade)    | 94.03 dB  | -77 dBm  | +21  | +11 dB | ✅ OK
Marginal (1000m, partial obs.) | 100 dB    | -83 dBm  | +15  | +5 dB  | ⚠️ Risky
Poor (2000m, multiple obs.)    | 106 dB    | -89 dBm  | +9   | -1 dB  | ❌ FAIL
```

---

## 8. Haberleşeme Kriterleri

### ✅ GÜVENLİ HABERLEŞİM (Margin > 10 dB)
- **Mesafe:** < 800m line-of-sight
- **Şartlar:** Clear sky, no obstacles
- **Fade:** Günlük atmospheric fading tolerate
- **Örnek:** Parkta kumanda, açık hava

### ⚠️ SINIRLI HABERLEŞİM (Margin 5-10 dB)
- **Mesafe:** 800-1500m
- **Şartlar:** Some vegetation, urban area
- **Fade:** Signal fluctuation riski
- **Örnek:** Şehirde kumanda, hafif obstrüksiyon

### ❌ RISKLI HABERLEŞİM (Margin < 5 dB)
- **Mesafe:** > 1500m
- **Şartlar:** Buildings, dense forest
- **Fade:** Frequent dropouts
- **Örnek:** Şehir içinde veya orman

---

## 9. Fade Margin'i Arttırma Yöntemleri

| Yöntem | Kazanç | Zorluk | Maliyeti |
|--------|--------|--------|----------|
| TX Power ↑ (5 dBm) | +5 dB | Düşük | Modülde built-in |
| RX Antenna ↑ (3 dBi) | +3 dB | Orta | Dış anten |
| Spreading Factor ↑ (SF8) | -5 dB speed, +2.5 dB SNR | Yüksek | Code change |
| Relay/Repeater | +20+ dB | Çok Yüksek | 2. modul |
| Error Correction Code | ~3 dB | Düşük | Software |

**Tavsiye:** 2.4 GHz ile +20 dBm default, obstacle kontrolü yap.

---

## 10. Frekans Ayarlaması (2.4-2.5 GHz)

### SX1280 Frekans Aralığı
```
Minimum: 2.400 GHz (2400 MHz)
Maksimum: 2.500 GHz (2500 MHz)
Adım: 200 kHz (0.0002 GHz)

Örnek frekanslar:
- 2.400 GHz (Bluetooth start)
- 2.410 GHz (WiFi ch1)
- 2.440 GHz (WiFi ch6, center)
- 2.480 GHz (WiFi ch13)
- 2.500 GHz (5GHz transition)
```

### Path Loss Karşılaştırması
```
2.400 GHz: 94.03 dB (500m)
2.410 GHz: 94.06 dB (minimal difference)
2.440 GHz: 94.14 dB (0.11 dB daha yüksek)
2.500 GHz: 94.39 dB (0.36 dB daha yüksek)

SONUÇ: Frekans değişimi mesafedeki etkisinden çok daha küçük!
```

---

## 11. Kod Parametreleri

### shared/config.h
```c
// Frekans ayarlama (MHz)
#define FREQUENCY_MIN    2400   // 2.4 GHz minimum
#define FREQUENCY_MAX    2500   // 2.5 GHz maximum
#define FREQUENCY_DEFAULT 2440  // 2.440 GHz (WiFi ch6)

// TX Power (dBm)
#define TX_POWER         20     // +20 dBm (maksimum)
#define TX_POWER_MIN     0      // 0 dBm minimum (1mW)

// RX Sensitivity (dBm)
#define RX_SENSITIVITY   -98    // -98 dBm @ SF7

// Path Loss Hesaplama (macro)
// Path Loss (dB) = 32.45 + 20*log10(f_MHz) + 20*log10(d_km)
```

### RX/src/main.cpp (Frekans Ayarlama)
```cpp
// Runtime frekans değiştirme
void setFrequency(uint32_t freq_mhz) {
    if (freq_mhz >= FREQUENCY_MIN && freq_mhz <= FREQUENCY_MAX) {
        frequency_mhz = freq_mhz;
        // SX1280 register update
        // sx1280_setFrequency(freq_mhz);
    }
}

// Path Loss hesapla
float getPathLoss(uint32_t freq_mhz, float distance_km) {
    return 32.45 + 20 * log10(freq_mhz) + 20 * log10(distance_km);
}

// SNR hesapla
float getSNR(float path_loss_db) {
    float rx_power = TX_POWER - path_loss_db - 3; // 3dB cable loss
    return rx_power - RX_SENSITIVITY;
}
```

---

## 12. Referans ve Linkler

```
1. Friis Transmission Equation:
   https://en.wikipedia.org/wiki/Friis_transmission_equation

2. SX1280 Datasheet:
   https://www.semtech.com/products/wireless-rf/lora-connect/sx1280

3. Free Space Path Loss:
   https://www.everythingrf.com/rf-calculators/friis-transmission-equation

4. LoRa Range Calculator:
   https://www.semtech.com/lora-tools/coverage

5. ISM Band Frequencies:
   https://en.wikipedia.org/wiki/ISM_band
```

---

## Özet

| Parametre | Değer | Sonuç |
|-----------|-------|-------|
| **Teorik Menzil (LoS)** | ~500m | ✅ Project spec |
| **Guaranteed Menzil** | ~300m | ⚠️ Obstacles için |
| **Maximum Menzil** | ~800m | ❌ Risky margin |
| **Frekans Etkisi** | 0.36 dB/100MHz | ✅ Minimal |
| **Best Practice** | 2.440 GHz | ✅ WiFi ch6 avoid |

**SONUÇ:** 2.4-2.5 GHz aralığında frekans seçimi mesafe hesaplamasını çok az etkiler. Haberleşemenin kalitesi **TX power, antenna, ortam obstrüksiyonu**'na daha bağımlıdır.
