# Dağıtım ve Test Rehberi

## ✅ Montaj Sonrası Kontrol Listesi

### TX (Kumanda İstasyonu)

- [ ] Tüm potansiyometreler mekanik olarak çalışıyor mu?
- [ ] 4 switch'in tamamı yanıt veriyor mu?
- [ ] SSD1306 OLED ekran görünüyor mu?
- [ ] Serial monitor 115200 baud'da bağlanıyor mu?
- [ ] TX kodunu compile ve upload et: `pio run -e tx --target upload`

### RX (İHA Alıcısı)

- [ ] 7 servo pin'i elektrik alıyor mu?
- [ ] Status LED yanıp sönüyor mu?
- [ ] Serial monitor 115200 baud'da bağlanıyor mu?
- [ ] RX kodunu compile ve upload et: `pio run -e rx --target upload`

---

## 🔧 Başlatma Prosesi

### Adım 1: TX Başlat
```bash
# Terminal açarak TX'e Serial Monitor bağla
pio device monitor -e tx --baud 115200
```

Bekle: `[6/6] ✓ HAZIR!` mesajını görene kadar.

### Adım 2: RX Başlat
```bash
# Ayrı bir terminal penceresi:
pio device monitor -e rx --baud 115200
```

Bekle: `[5/5] ✓ HAZIR!` mesajını görene kadar.

### Adım 3: İlk Paket Alımı
RX'in output'unda şunu görmelisin:
```
[RX] Paket #1 | THR:1500 BATT:8.23V | Loss: 0%
[RX] Paket #2 | THR:1500 BATT:8.23V | Loss: 0%
```

Eğer görmüyorsan:
- KBL: SPI wiring'i kontrol et
- KBL: SX1280'nin voltajı var mı?
- KBL: Reset pin (GPIO 17) doğru mu?

---

## 🎛️ Potansiyometre Kalibrasyonu

### Manuel Test
1. **TX'de** potansiyometreleri oku
2. **Serial Monitor** (TX):
   ```
   [TX] Paket #5 | THR:1000 YAW:1500 | BATT:8.23V | FLAGS:0x00
   ```

3. **Throttle Pot'u** tam aşağı → `THR:1000` olmalı
4. **Throttle Pot'u** ortaya → `THR:1500` olmalı
5. **Throttle Pot'u** tam yukarı → `THR:2000` olmalı

**Eğer dışında kalıyorsa:**
- config.h'de `ADC_MIN` ve `ADC_MAX` ayarla
- Veya `CHANNEL_MIN` ve `CHANNEL_MAX` değerini değiştir

### Tüm Kanalları Kontrol Et
Kalibration tablosu (RX servo çıkışı):

| Kanal | Pot Min | Pot Center | Pot Max | Not |
|-------|---------|-----------|---------|-----|
| CH1 Throttle | 1000 µs | 1500 µs | 2000 µs | ✓ |
| CH2 Yaw | 1000 µs | 1500 µs | 2000 µs | ✓ |
| CH3 Roll | 1000 µs | 1500 µs | 2000 µs | ✓ |
| CH4 Pitch | 1000 µs | 1500 µs | 2000 µs | ✓ |
| CH5 Aux1 | 1000 µs | 1500 µs | 2000 µs | ✓ |
| CH6 Aux2 | 1000 µs | 1500 µs | 2000 µs | ✓ |
| CH7 Aux3 | 1000 µs | 1500 µs | 2000 µs | ✓ |

---

## 📊 Pil Voltajı İzlemesi

### TX'de Pil Durumu Kontrol

**Serial Output:**
```
[TX] Paket #100 | THR:1500 | BATT:8.23V | FLAGS:0x00
                                      ↑
                                      OK (Normal)
```

**Alarm Durumları:**

| Voltaj | Durum | FLAGS | Ekran |
|---------|-------|-------|-------|
| > 8.4V | Tam | 0x00 | "8.23V" |
| 8.0V — 8.4V | Normal | 0x00 | "8.00V" |
| < 8.0V | **DÜŞÜK** | **0x04** | **"LOW!"** |

### Pil Testi
1. Fresh 2S LiPo bağla (8.4V)
2. Serial output: `BATT:8.40V | FLAGS:0x00`
3. Motoru yay başla / yükle
4. Voltaj düşmeyi (örn. 8.1V) gözle
5. < 8.0V olunca: "LOW!" görünecek

---

## ✈️ Menzil Testi

### Adım 1: Yakın Aralık (0-10m)

```
RX Serial Monitor:
[RX] Paket #1000 | Loss: 0%      ← Mükemmel
[RX] Paket #1001 | Loss: 0%      ← Mükemmel
```

### Adım 2: Orta Aralık (10-50m)

```
[RX] Paket #2000 | Loss: 1%      ← Kabul edilebilir
[RX] Paket #2001 | Loss: 1%
```

Paket kaybı **> 5%** olursa:
- TX güç'ü artır (13 dBm max)
- Spread Factor'ü artır (7 → 9) *Hızı düşür*
- Antenna konumu kontrol et

### Adım 3: Maksimum Aralık

- SX1280 SF=7, 13dBm: ~1-2 km açık alan
- SF=9: ~5+ km

**Test İçin:**
1. TX ve RX arasına mesafe koy
2. Potansiyometreleri kanallar
3. Serial loss rate'i kontrol et
4. Failsafe'i test et (haberleşme kesince motor kapanmalı)

---

## 🚨 Failsafe Testi

### Senaryolar

**Senaryo 1: TX Kapatma**
```
[RX] Paket #500 | Loss: 0%
[RX] ERROR: Paket timeout!
[RX] FAILSAFE: Motor kapalı! (Throttle = 1000µs)
```

Status LED blink pattern: **HIZLI** ← Failsafe

**Senaryo 2: Sinyal Kesme (500ms)**
1. RX, paket bekler...
2. 500ms geçince → failsafe
3. Throttle otomatik 1000µs (Motor safe)

### Failsafe Doğrulama
1. TX aktif, RX düzgün paket alıyor
2. TX'i kapalı tut → RX 500ms sonra failsafe
3. Status LED: **Hızlı blink** görülmeli
4. Servo output GPIO'larını multimetre ile kontrol et (PWM 1000µs)

---

## 🔊 Serial Debug Çıkışı Yorumu

### TX Debug (Her 2 saniye):
```
[TX] Paket #123 | THR:1234 YAW:1456 ROLL:1512 PITCH:1489 | T1=1 T2=0 | BATT=8.23 V | FLAGS=0x00
      ↑           ↑                                         ↑          ↑              ↑
  Sayaç      Kanal Değerleri                            Switches  Pil Durumu   Bayraklar
```

### RX Debug (Her 2 saniye):
```
[RX] Paket #123 | THR:1234 BATT:8.23V | Loss: 0%
      ↑           ↑        ↑           ↑
  Sayaç      Throttle  Pil Alındı   Paket Kaybı %
```

---

## 🧪 Hassas Testler

### Test 1: Checksum Validasyonu
- RX `[RX] ✗ Checksum hatası!` görmemelidir
- Karakteristik: 0 hata (< 0.1%)

### Test 2: Paket Hızı
- TX: 50 Hz (20ms) ✅
- RX: Timeout = 500ms ✅
- Loss < 1% normal aralık

### Test 3: Switch'ler
1. TX'de Toggle Switch'i aç
2. RX Serial'de `T1=1` görülmeli
3. Toggle Switch'i kapat
4. RX'de `T1=0` görülmeli

---

## 📋 Kontrol Listesi (Pre-Deployment)

### Yazılım
- [ ] TX compile hatası yok
- [ ] RX compile hatası yok
- [ ] Serial output clean (error yok)
- [ ] Paket kaybı < 1%

### Donanım
- [ ] Tüm pin bağlantıları kontrol edildi
- [ ] SPI wiring iki kez kontrol edildi
- [ ] Pil voltajı > 8.0V
- [ ] Tüm LED'ler yanıyor
- [ ] Tüm servo pin'leri GPIO output'unda

### Mekanik
- [ ] Potansiyometreler sorunsuz çalışıyor
- [ ] Switch'ler crackly olmadan çalışıyor
- [ ] SSD1306 ekran görülebilir
- [ ] Tüm kablolar güvenli

---

## 🎉 Başarılı Başlatma Belirtileri

✅ TX Serial: `[6/6] ✓ HAZIR!`  
✅ RX Serial: `[5/5] ✓ HAZIR!`  
✅ RX Debug: `[RX] Paket #1 | Loss: 0%`  
✅ Potansiyometreler→Servo çıkışı senkronize  
✅ Failsafe test geçti (Motor safe kapalı)  
✅ Menzil testi **> 1 km** (SF=7)  

---

**Status:** ✅ Kullanıma Hazır (v1.0)
