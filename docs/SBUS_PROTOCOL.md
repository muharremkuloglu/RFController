# 📡 SBUS Protokolü - ESP32-RX → Orange Cube

## Genel Bakış

SBUS, 16 kanallı RC kumanda sinyalini seri veri olarak iletişim kuran bir protokoldür. ESP32-RX'teki SX1280'den alınan RF kumandasını Orange Cube (Pixhawk 4) tarafından anlaşılacak SBUS formatına dönüştürür.

## SBUS Frame Yapısı

```
┌──────────────────────────────────────────────────────┐
│ Byte 0 │ Bytes 1-22 │ Byte 23 │ Byte 24            │
│ START  │ Ch Data    │ FLAGS   │ END                │
│ 0x0F   │ (176 bits) │ 0x00    │ 0x00               │
└──────────────────────────────────────────────────────┘
Total: 25 bytes
```

## Haberleşme Parametreleri

| Parametre | Değer |
|-----------|-------|
| **Baudrate** | 100,000 bps (100 kbaud) |
| **Data Bits** | 8 |
| **Parity** | Even (E) |
| **Stop Bits** | 2 |
| **Toplam Format** | SERIAL_8E2 |
| **Signal Type** | **INVERTED** (logic ters) |
| **Frame Rate** | 25 Hz (40ms interval) |
| **Kanallar** | 16 × 11-bit resolution |

## Inverted UART (Ters Sinyal)

SBUS özel bir özelliği vardır - seri sinyal ters çevrilmiştir:

```
Normal TTL UART:
- Idle (veri yok): HIGH (3.3V)
- Aktif veri: LOW (0V)
- Bit TIME: ~10µs @ 100kbaud

SBUS UART (Inverted):
- Idle: LOW (0V) ← TERS!
- Aktif: HIGH (3.3V)
- Bit TIME: ~10µs @ 100kbaud
```

### ESP32-S3 UART Inversiyonu

ESP32-S3 doğrudan inverted UART desteğine sahiptir:

```cpp
// UART1 - 100kbaud, inverted
Serial1.begin(100000, SERIAL_8E2, -1, 43);  // RX=-1 (unused), TX=GPIO43

// Inverted mode enable (pin mode control)
uint32_t protocol = UART_HWFC_DISABLE | UART_PARITY_EVEN;
uart_set_line_inverse(UART_NUM_1, UART_INVERSE_TXD);  // TX'i ters çevir
```

## Kanal Veri Formatı (11-bit × 16)

420 mil içinde 16 kanal için 176 bit veri:

```
CH1:  Bytes 1-2,   Bits 0-10
CH2:  Bytes 2-3,   Bits 3-13  
CH3:  Bytes 4-5,   Bits 6-16
... (benzer şekilde CH4-CH16)

Aralık:
- Min: 0 (~988µs PWM)
- Center: 1024 (~1500µs PWM)
- Max: 2047 (~2012µs PWM)
```

## PWM Dönüşümü

RF kumanda değerini SBUS kanalına dönüştürme:

```cpp
// TX konum (1000-2000 µs) → SBUS kanal (0-2047)
uint16_t rxPos = 1500;  // örnek: 1000-2000 µs aralığı
uint16_t sbusValue = map(rxPos, 1000, 2000, 0, 2047);

// SBUS kanalı → PWM (988-2012 µs) dönüşümü (Orange Cube içinde)
uint16_t pwmMicros = 988 + (sbusValue * 1024 / 2048);
```

## SBUS Frame Encoding

Basit bit-level encoding örneği:

```cpp
// 16 channel values (11-bit each) SBUS 22 byte'a sığdırmak
void encodeSBUSFrame(uint16_t channels[16], uint8_t frame[25]) {
    frame[0] = 0x0F;  // Start byte
    
    // Channel 1 & 2 (11 bits each = 22 bits = 1 byte + bits)
    frame[1] = (channels[0] & 0x07) << 5 | (channels[1] & 0x1F);
    frame[2] = (channels[1] & 0xC0) >> 5 | (channels[2] & 0x0F) << 3;
    // ... (continues for CH3-CH16)
    
    frame[23] = 0x00;  // Flags (failsafe, signal loss)
    frame[24] = 0x00;  // End byte
}
```

## Orange Cube Bağlantısı

```
ESP32-S3              Orange Cube
═════════════════════════════════════

GPIO 43 (UART1_TX)  ──────→  SBUS IN (RCV port, Pin 1)
GND                 ──────→  GND (RCV port, Pin 2)

⚠️ Not: Ters sinyal kullanılıyor!
```

## Gönderme Prosedürü

```cpp
#include <Arduino.h>

uint16_t sbusDecode[16] = {
    1500, 1500, 1500, 1500,  // CH1-4 (Throttle, Yaw, Roll, Pitch)
    1500, 1500, 1500, 1500,  // CH5-8 (Aux + trimmers)
    0, 0, 0, 0,              // CH9-12 (kullanılmayan)
    0, 0, 0, 0               // CH13-16 (kullanılmayan)
};

uint8_t sbusFrame[25];

void setupSBUS() {
    // UART1: 100kbaud, inverted TX on GPIO43
    Serial1.begin(100000, SERIAL_8E2, -1, 43);
    
    // TX pin inversiyonu enable
    uart_set_line_inverse(UART_NUM_1, UART_INVERSE_TXD);
}

void sendSBUSFrame() {
    encodeSBUSFrame(sbusDecode, sbusFrame);
    Serial1.write(sbusFrame, 25);
}

void loop() {
    // 25Hz SBUS output (40ms interval)
    static unsigned long lastTime = 0;
    unsigned long now = millis();
    
    if (now - lastTime >= 40) {
        // RF paketinden yeni kanal değerleri oku
        sbusDecode[0] = rxPacket.throttle;
        sbusDecode[1] = rxPacket.yaw;
        sbusDecode[2] = rxPacket.roll;
        sbusDecode[3] = rxPacket.pitch;
        
        sendSBUSFrame();
        lastTime = now;
    }
}
```

## Sorun Giderme

### Problem: Orange Cube SBUS sinyali almıyor

**Çözüm Listesi:**
1. ✅ Baudrate 100kbps kontrol et (115200 değil!)
2. ✅ GPIO43 doğru pin mi?
3. ✅ UART inversiyonu enabled mi?
4. ✅ GND bağlantısı var mı?
5. ✅ Oscilloscope ile sinyal şeklini kontrol et (idle=LOW, active=HIGH)

### Problem: İlk kanal çalışıyor ama diğerleri değil

**Çözün:** Frame encoding bit shifts yanlış olabilir. Kanal 2+ için bit alignment kontrol et.

### Problem: Servo titremesi (jitter)

**Olası nedenler:**
- Frame rate tutarsız (40ms ≠ 25Hz)
- SBUS frame corrupted (START/END bytes yanlış)
- RF bağlantı zayıf (data loss)

## QGroundControl Verification

Orange Cube üzerinde QGroundControl'de kontrol:

```
Vehicle Setup → Radio → RC Input
├─ Channel 1 (Throttle): 988-2012 µs aralığı
├─ Channel 2 (Yaw): 988-2012 µs aralığı
├─ Channel 3 (Roll): 988-2012 µs aralığı
├─ Channel 4 (Pitch): 988-2012 µs aralığı
├─ ... (CH5-16)
└─ Signal indicator: Yeşil (bağlı), Kırmızı (bağlı değil)
```

Grafikte gerçek zamanlı servo pozisyonlarını gözlemleyebilirsiniz.

## Referanslar

- [SBUS Protokol Spesifikasyonu](https://www.futabausa.com/servos-and-accessories)
- [Orange Cube (Pixhawk 4) SBUS Desteği](https://docs.px4.io/main/en/flight_controller/pixhawk4.html)
- [ESP32-S3 UART Ters Çevirme](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/uart.html)
