#ifndef CONFIG_H
#define CONFIG_H

// ============ SPI PİNLERİ ============
#define SX1280_NSS   5      // Chip Select
#define SX1280_RES   17     // Reset
#define SX1280_BUSY  4      // Busy Pin
#define SX1280_DIO1  2      // DIO1

// ============ ADC PİNLERİ (POTANSİYOMETRELER) ============
#define POT_THROTTLE 34     // Throttle potansiyometri
#define POT_YAW      35     // Yaw potansiyometri
#define POT_ROLL     32     // Roll potansiyometri
#define POT_PITCH    33     // Pitch potansiyometri
#define POT_AUX1     25     // Yardımcı kanal 1
#define POT_AUX2     26     // Yardımcı kanal 2
#define POT_AUX3     27     // Yardımcı kanal 3

// ============ VOLTAJ SENSÖRÜ (ADC) ============
#define BATTERY_SENSE 36    // VP pin - Pil voltajı okuması

// ============ SWITCH PİNLERİ ============
#define SWITCH_TOGGLE_1  12    // Toggle switch 1
#define SWITCH_TOGGLE_2  13    // Toggle switch 2
#define SWITCH_REGULAR   14    // Normal switch
#define SWITCH_POWER     15    // Güç on/off switch

// ============ EKRAN (SSD1306 I2C) ============
#define OLED_SDA     21     // I2C SDA
#define OLED_SCL     22     // I2C SCL
#define OLED_ADDRESS 0x3C   // SSD1306 I2C adresi

// ============ RX SERVO ÇIKIŞLARI (PWM) ============
#define GPIO_SERVO_THROTTLE  14     // CH1: Throttle/Motor
#define GPIO_SERVO_YAW       13     // CH2: Yaw
#define GPIO_SERVO_ROLL      12     // CH3: Roll
#define GPIO_SERVO_PITCH     27     // CH4: Pitch
#define GPIO_SERVO_AUX1      25     // CH5: Auxiliary 1
#define GPIO_SERVO_AUX2      26     // CH6: Auxiliary 2
#define GPIO_SERVO_AUX3      11     // CH7: Auxiliary 3

// ============ RX STATUS LED ============
#define STATUS_LED           15     // Status LED (OK = blink slow, ERROR = blink fast)

// ============ FREKANS VE DEĞİŞKENLER ============
#define FREQUENCY    2400   // MHz (2.4 GHz)
#define TX_POWER     13     // dBm (13 max for SX1280)
#define BANDWIDTH    812    // KHz
#define SPREADING    7      // Spreading Factor

// ============ KALIBRASYONU ============
#define ADC_MIN      0      // Potansiyometrenin minimum değeri
#define ADC_MAX      4095   // ESP32 ADC max (12-bit)
#define CHANNEL_MIN  1000   // Servo minimum µs
#define CHANNEL_MAX  2000   // Servo maximum µs
// ============ PİL VOLTAJI ============
#define BATTERY_MIN  8.0    // Minimum pil voltajı (2S LiPo)
#define BATTERY_MAX  8.4    // Maksimum pil voltajı (2S LiPo dolu)
#define VOLTAGE_DIVIDER 2.0 // Voltaj bölücü oranı (shunt için)
// ============ PROTOKOL ============
#define PACKET_SIZE  32     // Paket boyutu
#define SYNC_ID      0xABCD // Senkronizasyon ID'si
#define TX_RATE      50     // Gönderim hızı (Hz)

#endif
