// ============================================================================
// ⚠️ TX/lib/config/config.h - TX-SPECIFIC KONFIGÜRASYONLAR
// ============================================================================
//
// ⚠️ ÖNEMLİ: Bu dosya DEPRECATED! shared/config.h kullan!
//
// İŞLEV:
//   - TX-specific ayarlar (şu an boş)
//   - ORTAK ayarlar için → shared/config.h include et
//
// DURUM:
//   - shared/config.h zaten TÜM pinleri tanımlıyor (TX + RX)
//   - Bu dosya gelecekte TX-only ayarlar için ayrılmış (vb: max TX power)
//
// PROBLEMLER:
//   ⚠️ Şu an shared/config.h'nin KOPYASI (DUPLICATE CODE!)
//   ⚠️ Değişim bir yerde yapılırsa diğer iki yerde senkron kaybı
//   ⚠️ Refactoring lazım: TX/lib/config/config.h silip shared/config.h use
//
// DOĞRU KULLANIM:
//   TX/src/main.cpp'de:
//   #include "../../../shared/config.h"  // ORTAK pinler
//   #include "config.h"                  // TX-specific (varsa)
//
// İLGİLİ DOSYALAR:
//   - shared/config.h    → MASTER SOURCE (ortak pinler)
//   - RX/lib/config/config.h → RX version (duplicate)
//
// ============================================================================

#ifndef CONFIG_H
#define CONFIG_H

// ⚠️ UYARI: Aşağıdaki ayarlar deprecated ve shared/config.h'de de var!
// Değişim yaparsan HER İKİ yerde yap veya shared/config.h'yi include et

// ============ SPI PİNLERİ (SX1280 RF Modülü) ============
#define SX1280_NSS   10     // Chip Select (CS)
#define SX1280_RES   7      // Reset
#define SX1280_BUSY  6      // Busy Pin
#define SX1280_DIO1  5      // DIO1 Interrupt

// ============ ADC PİNLERİ - JOYSTİCKLER (2x Analog Joystick = 4 Eksen) ============
#define JOYSTICK1_X  1      // X ekseni (Roll/Aileron)
#define JOYSTICK1_Y  2      // Y ekseni (Pitch/Elevator)
#define JOYSTICK2_X  3      // X ekseni (Yaw/Rudder)
#define JOYSTICK2_Y  4      // Y ekseni (Throttle/Gaz)

// ============ ADC PİNLERİ - TRİM POTANSİYOMETRELERİ (3x Pot) ============
#define TRIM_ROLL    42     // Roll Trim Potansiyometresi
#define TRIM_PITCH   41     // Pitch Trim Potansiyometresi
#define TRIM_YAW     40     // Yaw Trim Potansiyometresi

// ============ VOLTAJ SENSÖRÜ (ADC) ============
#define BATTERY_SENSE 8     // Pil voltajı okuması (ADC1)

// ============ SWITCH PİNLERİ (GPIO) ============
#define SWITCH_FLIGHT_MODE   16    // Uçuş Modu (AUTO/FBWA geçişi)
#define SWITCH_TRIM_LOCK     17    // Trim Lock (Trim kilidi)

// ============ EKRAN (SSD1306 I2C) ============
#define OLED_SDA     9      // I2C SDA
#define OLED_SCL     8      // I2C SCL
#define OLED_ADDRESS 0x3C   // SSD1306 I2C adresi

// ============ RX SERVO ÇIKIŞLARI (PWM) - İHA Alıcı Tarafı ============
#define GPIO_SERVO_THROTTLE  14     // CH1: Throttle/Motor
#define GPIO_SERVO_YAW       13     // CH2: Yaw
#define GPIO_SERVO_ROLL      12     // CH3: Roll
#define GPIO_SERVO_PITCH     11     // CH4: Pitch
#define GPIO_SERVO_AUX1      37     // CH5: Auxiliary 1
#define GPIO_SERVO_AUX2      36     // CH6: Auxiliary 2
#define GPIO_SERVO_AUX3      35     // CH7: Auxiliary 3

// ============ RX STATUS LED ============
#define STATUS_LED           15     // Status LED

// ============ FREKANS VE PARAMETERLER ============
#define FREQUENCY    2400   // MHz (2.4 GHz)
#define TX_POWER     20     // dBm (+20 dBm max for SX1280 PA+LNA)
#define BANDWIDTH    812    // KHz
#define SPREADING    7      // Spreading Factor

// ============ ADC KALİBRASYONU ============
#define ADC_MIN      0      // Joystick/Pot minimum
#define ADC_MAX      4095   // ESP32-S3 ADC max (12-bit)
#define CHANNEL_MIN  1000   // Servo minimum µs
#define CHANNEL_MAX  2000   // Servo maximum µs

// ============ PİL VOLTAJI KALİBRASYONU (8x AA Alkaline = 12V) ============
// Gerilim Bölücü: 27kΩ (R1) + 10kΩ (R2) → 3.3V ADC max input
#define BATTERY_MIN  8.0    // Minimum voltaj (8V - 1V per cell warning)
#define BATTERY_MAX  12.0   // Maksimum voltaj (12V - 1.5V per cell)
#define VOLTAGE_DIVIDER 3.64 // Gerilim bölücü oranı (27k:10k = 3.636 ≈ 3.64)

// ============ İLETİŞİM PROTOKOLÜ ============
#define PACKET_SIZE  32     // Paket boyutu (bytes)
#define SYNC_ID      0xABCD // Senkronizasyon ID'si
#define TX_RATE      50     // Gönderim hızı (Hz = 20ms)

// ============ ESP32-S3 SPI AYARLARI ============
#define SPI_CLOCK    10000000  // 10 MHz SPI saat hızı

#endif
