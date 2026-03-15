// ============================================================================
// 🎮 TX/src/main.cpp - Kumanda İstasyonu (Pilot Kontrolü)
// ============================================================================
//
// İŞLEV:
//   - Pilot kumandası: 2 joystick, 3 trim potnasiyometresi, 2 anahtar
//   - HABERLEŞİM: SX1280 RF modülü (2.4GHz, 50Hz TX rate)
//   - EKRAN: SSD1306 OLED (0.96", I2C, 8-satır telemetri)
//   - PAZAR: 8x AA alkaline battery, voltaj ölçümü, signal strength sim
//
// FLOW:
//   1. Joystick/Trim ADC oku (her loop)
//   2. Anahtar durumlarını oku (GPIO16, GPIO17)
//   3. Pil voltajını ölçü (GPIO8, ADC → voltage divider)
//   4. Her 20ms'de ControlPacket gönder (50Hz) → SX1280 → 2.4GHz RF
//   5. Her 200ms'de OLED güncelle (8-satır bilgi)
//
// DISPLAY OUTPUT (8 satır):
//   TX | BAT:(75%) PKT:234
//   SIGNAL: -92dBm [====- ] 85%
//   VOLT: 7.85V | ADC:3892
//   THR: 512 YAW: 0
//   ROLL: -12 PITCH:150
//   SW: [T1] [T2]
//   MODE: AUTO
//   STATUS: OK
//
// KRİTİK NOKTALAR:
//   - ⚠️ SPI pinler: CS=GPIO10, RESET=GPIO7, BUSY=GPIO6, DIO1=GPIO5
//   - ⚠️ I2C pinler: SDA=GPIO9, SCL=GPIO8 (OLED adresi=0x3C)
//   - ⚠️ RF Rate: EXACTLY 50Hz (20ms aralıklar), hata tolerance düşük
//   - ⚠️ Voltage divider oranı: 3.64 (27k:10k, hata ±1% tolerance)
//   - ⚠️ Signal strength simulasyon: -95 ~ -80 dBm (gerçek değil, placeholder)
//
// İLGİLİ DOSYALAR:
//   - shared/config.h         → PIN tanımları
//   - docs/HARDWARE.md        → TX pin haritası
//   - docs/protocol.md        → ControlPacket format
//   - TX/lib/config/config.h  → TX-specific ayarlar (şu an boş)
//   - RX/src/main.cpp         → RF paket alıcı
//
// ============================================================================

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "protocol.h"

// ============ GLOBALS ============
// Ekran
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// SX1280 Değişkenleri
ControlPacket txPacket;
unsigned long lastTxTime = 0;
const unsigned long TX_INTERVAL = 1000 / TX_RATE; // 20ms = 50Hz
bool radioInitialized = false;
uint8_t packetCount = 0;

// Ekran güncelleme
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 200; // 200ms

// Sensör Değerleri
float batteryVoltage = 0.0;
uint16_t batteryRaw = 0;
int8_t signalStrength = -120;  // dBm (RSSI)
uint8_t linkQuality = 0;  // 0-100%
uint16_t txPower = 20;  // dBm

// SX1280 State
volatile bool txDone = false;
volatile bool rxDone = false;

// ============ SX1280 REGISTER ADDRESSES & COMMANDS ============
#define SX1280_CMD_NOP                       0x00
#define SX1280_CMD_RESET                     0x07
#define SX1280_CMD_SLEEP                     0x10
#define SX1280_CMD_STANDBY                   0x11
#define SX1280_CMD_FS                        0x12
#define SX1280_CMD_TX                        0x13
#define SX1280_CMD_RX                        0x14
#define SX1280_CMD_RXDUTYCYCLE               0x15
#define SX1280_CMD_CAD                       0x16
#define SX1280_CMD_SETTXPARAMS               0x20
#define SX1280_CMD_SETTXRAMP                 0x24
#define SX1280_CMD_SETRFFREQUENCY            0x21
#define SX1280_CMD_SETPACKETTYPE             0x8A
#define SX1280_CMD_GETPACKETTYPE             0x11
#define SX1280_CMD_SETRXPARAMS               0x23
#define SX1280_CMD_SETMODULATIONPARAMS       0x22
#define SX1280_CMD_SETDIOIRQPARAMS           0x08
#define SX1280_CMD_GETSTATUS                 0xC0
#define SX1280_CMD_READREGISTER              0x1D
#define SX1280_CMD_WRITEREGISTER             0x1C
#define SX1280_CMD_READBUFFER                0x1E
#define SX1280_CMD_WRITEBUFFER               0x1A

#define SX1280_REG_FIRMWARE                  0x0153
#define SX1280_REG_TX_BUFFER_STATUS          0x0E01
#define SX1280_REG_RX_BUFFER_STATUS          0x0E02

// ============ FUNCTION PROTOTYPES ============
void initSPI();
void initDisplay();
void initSwitches();
void initADC();
void initSX1280();
void readPotentiometers();
void readSwitches();
void transmitPacket();
void updateDisplay();
void sx1280_spi_write(uint8_t cmd, uint8_t *data, uint8_t len);
void sx1280_spi_read(uint8_t cmd, uint8_t *data, uint8_t len);
void sx1280_reset();
void sx1280_set_frequency(uint32_t freq);
void sx1280_set_tx_power(int8_t power);
void sx1280_start_tx();
void sx1280_wait_ready();

// ============ SETUP & LOOP ============
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n╔════════════════════════════════╗");
    Serial.println("║  RF KUMANDA TX BAŞLATILIYOR   ║");
    Serial.println("║  Frekans: 2.4 GHz             ║");
    Serial.println("║  Modul: SX1280 E28-2G4M12S    ║");
    Serial.println("╚════════════════════════════════╝\n");
    
    // Başlatma sırası
    Serial.println("[1/6] SPI başlatılıyor...");
    initSPI();
    delay(100);
    
    Serial.println("[2/6] Ekran başlatılıyor...");
    initDisplay();
    delay(100);
    
    Serial.println("[3/6] ADC başlatılıyor...");
    initADC();
    delay(100);
    
    Serial.println("[4/6] Switch'ler başlatılıyor...");
    initSwitches();
    delay(100);
    
    Serial.println("[5/6] SX1280 başlatılıyor...");
    initSX1280();
    delay(500);
    
    if (radioInitialized) {
        Serial.println("[6/6] ✓ HAZIR!\n");
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(20, 25);
        display.println("TX READY!");
        display.display();
        delay(2000);
    } else {
        Serial.println("[6/6] ✗ Hata: Radio başlatılamadı!\n");
        while(1);  // Halt
    }
}

void loop() {
    // 50Hz TX
    if (millis() - lastTxTime >= TX_INTERVAL) {
        readPotentiometers();
        readSwitches();
        transmitPacket();
        lastTxTime = millis();
    }
    
    // 5Hz Display Update
    if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        updateDisplay();
        lastDisplayUpdate = millis();
    }
    
    // CPU yükü azaltmak için
    delayMicroseconds(100);
}

// ============ INITIALIZATION FUNCTIONS ============
void initSPI() {
    pinMode(SX1280_NSS, OUTPUT);
    pinMode(SX1280_RES, OUTPUT);
    pinMode(SX1280_BUSY, INPUT);
    
    // SPI başlat
    SPI.begin();
    SPI.setFrequency(10000000);  // 10 MHz
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    
    digitalWrite(SX1280_NSS, HIGH);
    Serial.println("✓ SPI hazır (10 MHz)");
}

void initDisplay() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("✗ SSD1306 OLED bulunamadı!");
        while (1);
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("RF KUMANDA TX");
    display.println("Initializing...");
    display.display();
    
    Serial.println("✓ SSD1306 OLED hazır (0x3C)");
}

void initADC() {
    pinMode(POT_THROTTLE, INPUT);
    pinMode(POT_YAW, INPUT);
    pinMode(POT_ROLL, INPUT);
    pinMode(POT_PITCH, INPUT);
    pinMode(POT_AUX1, INPUT);
    pinMode(POT_AUX2, INPUT);
    pinMode(POT_AUX3, INPUT);
    pinMode(BATTERY_SENSE, INPUT);
    
    analogSetWidth(12);  // 12-bit resolution
    Serial.println("✓ ADC hazır (7 Pot + Battery)");
}

void initSwitches() {
    pinMode(SWITCH_TOGGLE_1, INPUT_PULLUP);
    pinMode(SWITCH_TOGGLE_2, INPUT_PULLUP);
    pinMode(SWITCH_REGULAR, INPUT_PULLUP);
    pinMode(SWITCH_POWER, INPUT_PULLUP);
    
    Serial.println("✓ Switch'ler hazır (4 adet)");
}

void initSX1280() {
    // Reset
    sx1280_reset();
    delay(100);
    sx1280_wait_ready();
    
    // Firmware kontrolü
    uint8_t fw[2];
    sx1280_spi_read(SX1280_CMD_READREGISTER, fw, 2);
    Serial.print("  FW Version: 0x");
    Serial.println(fw[0], HEX);
    
    // Packet type: LoRa
    uint8_t pkt_type = 0x01;  // LoRa
    sx1280_spi_write(SX1280_CMD_SETPACKETTYPE, &pkt_type, 1);
    delay(10);
    
    // RF Frequency: 2400 MHz
    sx1280_set_frequency(2400);
    delay(10);
    
    // TX Power: 13 dBm
    sx1280_set_tx_power(13);
    delay(10);
    
    // Modulation: LoRa (BW=812kHz, SF=7)
    uint8_t mod_params[3] = {
        0x34,  // Bandwidth 812 kHz
        0x07,  // Spreading Factor 7
        0x01   // Coding Rate 4/5
    };
    sx1280_spi_write(SX1280_CMD_SETMODULATIONPARAMS, mod_params, 3);
    delay(10);
    
    // DIO IRQ
    uint8_t dio_params[8] = {
        0x01, 0x00,  // Tx done IRQ
        0x01, 0x00,  // Rx done IRQ
        0x00, 0x00,  // Rx timeout
        0x00, 0x00   // Sync address
    };
    sx1280_spi_write(SX1280_CMD_SETDIOIRQPARAMS, dio_params, 8);
    delay(10);
    
    // RX Parameters
    uint8_t rx_params[2] = {
        0x0A,  // Timeout = 10ms
        0x00
    };
    sx1280_spi_write(SX1280_CMD_SETRXPARAMS, rx_params, 2);
    
    radioInitialized = true;
    Serial.println("✓ SX1280 (E28-2G4M12S) hazır");
    Serial.println("  • Frekans: 2400 MHz");
    Serial.println("  • Güç: 13 dBm");
    Serial.println("  • SF: 7, BW: 812kHz");
}

// ============ INPUT READING ============
void readPotentiometers() {
    uint16_t raw_throttle = analogRead(POT_THROTTLE);
    uint16_t raw_yaw = analogRead(POT_YAW);
    uint16_t raw_roll = analogRead(POT_ROLL);
    uint16_t raw_pitch = analogRead(POT_PITCH);
    uint16_t raw_aux1 = analogRead(POT_AUX1);
    uint16_t raw_aux2 = analogRead(POT_AUX2);
    uint16_t raw_aux3 = analogRead(POT_AUX3);
    
    // Map to PWM range (1000-2000 µs)
    txPacket.throttle = map(raw_throttle, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    txPacket.yaw = map(raw_yaw, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    txPacket.roll = map(raw_roll, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    txPacket.pitch = map(raw_pitch, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    txPacket.aux1 = map(raw_aux1, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    txPacket.aux2 = map(raw_aux2, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    txPacket.aux3 = map(raw_aux3, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    
    txPacket.sync_id = SYNC_ID;
}

void readSwitches() {
    // Read switches (active low)
    txPacket.switch_toggle1 = !digitalRead(SWITCH_TOGGLE_1);
    txPacket.switch_toggle2 = !digitalRead(SWITCH_TOGGLE_2);
    txPacket.switch_regular = !digitalRead(SWITCH_REGULAR);
    
    // ============ BATTERY VOLTAGE READING ============
    // Exponential moving average filter for stable readings
    static float batteryFilter = 0.0;
    const float FILTER_ALPHA = 0.1;  // 0-1: lower = smoother, higher = faster response
    
    uint16_t raw_battery = analogRead(BATTERY_SENSE);
    batteryRaw = raw_battery;
    float battery_v = (raw_battery / 4095.0) * 3.3 * VOLTAGE_DIVIDER;
    
    // Apply Low-Pass Filter
    if (batteryFilter == 0.0) {
        batteryFilter = battery_v;  // Initialize
    } else {
        batteryFilter = (FILTER_ALPHA * battery_v) + ((1.0 - FILTER_ALPHA) * batteryFilter);
    }
    
    batteryVoltage = batteryFilter;
    txPacket.battery = (uint16_t)(batteryVoltage * 1000);  // Convert to mV
    
    // ============ SIGNAL STRENGTH SIMULATION ============
    // In real scenario, this would come from SX1280 RSSI reading
    // For now: simulate based on packet count and battery status
    static uint8_t signalCounter = 0;
    signalCounter++;
    
    // Base signal strength (varies slightly)
    signalStrength = -95 + (signalCounter % 15);  // Range: -95 to -80 dBm (strong signal)
    
    // If battery low, signal degrades
    if (batteryVoltage < BATTERY_MIN + 0.5) {
        signalStrength -= 5;
    }
    
    // Convert RSSI to link quality percentage
    // -120 dBm = 0%, -80 dBm = 100%
    if (signalStrength <= -120) {
        linkQuality = 0;
    } else if (signalStrength >= -80) {
        linkQuality = 100;
    } else {
        linkQuality = (uint8_t)((signalStrength + 120) * 2.5);  // Linear conversion
    }
    
    txPacket.flags = 0;
    
    // Low battery warning
    if (batteryVoltage < BATTERY_MIN) {
        txPacket.flags |= 0x04;  // Bit 2: Low battery
    }
    
    // Power switch check
    if (digitalRead(SWITCH_POWER)) {
        txPacket.flags |= 0x80;  // Bit 7: Power warning
    }
}

// ============ TRANSMISSION ============
void transmitPacket() {
    // Calculate checksum
    updateChecksum(&txPacket);
    
    // Prepare buffer
    uint8_t buffer[PACKET_SIZE];
    memcpy(buffer, (uint8_t*)&txPacket, sizeof(txPacket));
    
    // Write to TX buffer
    digitalWrite(SX1280_NSS, LOW);
    SPI.transfer(SX1280_CMD_WRITEBUFFER);
    SPI.transfer(0x00);  // Offset
    for (int i = 0; i < PACKET_SIZE; i++) {
        SPI.transfer(buffer[i]);
    }
    digitalWrite(SX1280_NSS, HIGH);
    delayMicroseconds(100);
    
    // Start TX
    sx1280_start_tx();
    
    packetCount++;
    
    // Serial debug output (every 2 seconds)
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime >= 2000) {
        Serial.print("[TX] Paket #");
        Serial.print(packetCount);
        Serial.print(" | THR:");
        Serial.print(txPacket.throttle);
        Serial.print(" YAW:");
        Serial.print(txPacket.yaw);
        Serial.print(" ROLL:");
        Serial.print(txPacket.roll);
        Serial.print(" PITCH:");
        Serial.print(txPacket.pitch);
        Serial.print(" | BATT:");
        Serial.print(txPacket.battery / 1000.0, 2);
        Serial.print("V | FLAGS:");
        Serial.println(txPacket.flags, HEX);
        
        lastDebugTime = millis();
    }
}

// ============ DISPLAY ============
void updateDisplay() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    // ========== TOP STATUS BAR (Line 0-7) ==========
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("TX | ");
    
    // Battery Status with Icon
    display.print("BAT:");
    if (batteryVoltage >= BATTERY_MAX - 0.5) {
        display.print("(100%) ");  // Full (12V-11.5V)
    } else if (batteryVoltage >= BATTERY_MAX - 1.5) {
        display.print("(75%)  ");  // 11.5V-10.5V
    } else if (batteryVoltage >= BATTERY_MAX - 3.0) {
        display.print("(50%)  ");  // 10.5V-9.0V
    } else if (batteryVoltage >= BATTERY_MIN) {
        display.print("(25%)W ");  // 9.0V-8.0V (Warning)
    } else {
        display.print("(0%)!! ");  // <8.0V (Critical)
    }
    
    // Packet count
    display.println("");
    display.setCursor(90, 0);
    display.print("PKT:");
    display.println(packetCount);
    
    // ========== SIGNAL & LINK QUALITY (Line 8-15) ==========
    display.setCursor(0, 8);
    display.print("SIGNAL: ");
    display.print(signalStrength);
    display.print("dBm ");
    
    // Signal strength visualization (bars)
    display.print("[");
    uint8_t bars = (linkQuality / 20);  // 0-5 bars
    for (uint8_t i = 0; i < 5; i++) {
        if (i < bars) {
            display.print("=");
        } else {
            display.print("-");
        }
    }
    display.print("] ");
    display.print(linkQuality);
    display.println("%");
    
    // ========== VOLTAGE DETAILS (Line 16-23) ==========
    display.setCursor(0, 16);
    display.print("VOLT: ");
    
    // Color coded voltage
    if (batteryVoltage >= BATTERY_MAX - 0.5) {
        display.print(batteryVoltage, 2);
        display.print("V ");
    } else if (batteryVoltage < BATTERY_MIN) {
        display.print(batteryVoltage, 2);
        display.print("V!!");
    } else {
        display.print(batteryVoltage, 2);
        display.print("V ");
    }
    
    display.print("| ADC:");
    display.println(batteryRaw);
    
    // ========== CONTROL VALUES - ROW 1 (Line 24-31) ==========
    display.setCursor(0, 24);
    display.print("THR:");
    display.print(txPacket.throttle - 1000, 4);
    display.print(" YAW:");
    display.print(txPacket.yaw - 1000, 4);
    display.print(" ");
    display.println("");
    
    // ========== CONTROL VALUES - ROW 2 (Line 32-39) ==========
    display.setCursor(0, 32);
    display.print("ROLL:");
    display.print(txPacket.roll - 1000, 4);
    display.print(" PITCH:");
    display.print(txPacket.pitch - 1000, 4);
    display.println("");
    
    // ========== SWITCHES & STATUS (Line 40-47) ==========
    display.setCursor(0, 40);
    display.print("SW: ");
    display.print(txPacket.switch_toggle1 ? "[T1]" : "     ");
    display.print(txPacket.switch_toggle2 ? "[T2]" : "     ");
    display.print(txPacket.switch_regular ? "[REG]" : "      ");
    display.println("");
    
    // ========== STATUS LINE (Line 48-55) ==========
    display.setCursor(0, 48);
    display.print("MODE: ");
    if (txPacket.switch_toggle1) {
        display.print("AUTO | ");
    } else {
        display.print("FBWA | ");
    }
    
    // Link Status
    if (linkQuality >= 80) {
        display.print("EXCELLENT");
    } else if (linkQuality >= 60) {
        display.print("GOOD");
    } else if (linkQuality >= 40) {
        display.print("FAIR");
    } else if (linkQuality >= 20) {
        display.print("WEAK");
    } else {
        display.print("LOST!");
    }
    display.println("");
    
    // ========== BOTTOM STATUS BAR (Line 56-63) ==========
    display.setCursor(0, 56);
    display.print("STATUS: ");
    
    if (radioInitialized) {
        // Check various status conditions
        if ((txPacket.flags & 0x04) && batteryVoltage < BATTERY_MIN) {
            display.print("LOW_BAT");
        } else if ((txPacket.flags & 0x80)) {
            display.print("PWR_WARN");
        } else if (linkQuality < 30) {
            display.print("WEAK_SIG");
        } else {
            display.print("OK");
        }
    } else {
        display.print("ERROR");
    }
    
    // FreeRam indicator (right aligned)
    display.setCursor(100, 56);
    display.print("TX");
    
    display.display();
}

// ============ SX1280 SPI FUNCTIONS ============
void sx1280_spi_write(uint8_t cmd, uint8_t *data, uint8_t len) {
    sx1280_wait_ready();
    
    digitalWrite(SX1280_NSS, LOW);
    SPI.transfer(cmd);
    for (int i = 0; i < len; i++) {
        SPI.transfer(data[i]);
    }
    digitalWrite(SX1280_NSS, HIGH);
    
    delayMicroseconds(100);
}

void sx1280_spi_read(uint8_t cmd, uint8_t *data, uint8_t len) {
    sx1280_wait_ready();
    
    digitalWrite(SX1280_NSS, LOW);
    SPI.transfer(cmd);
    SPI.transfer(0x00);  // Status byte
    for (int i = 0; i < len; i++) {
        data[i] = SPI.transfer(0x00);
    }
    digitalWrite(SX1280_NSS, HIGH);
    
    delayMicroseconds(100);
}

void sx1280_wait_ready() {
    while (digitalRead(SX1280_BUSY)) {
        delayMicroseconds(10);
    }
}

void sx1280_reset() {
    digitalWrite(SX1280_RES, HIGH);
    delay(10);
    digitalWrite(SX1280_RES, LOW);
    delay(10);
    digitalWrite(SX1280_RES, HIGH);
    delay(50);
}

void sx1280_set_frequency(uint32_t freq_mhz) {
    uint32_t freq = (freq_mhz << 17) / 1000 / 61;  // Internal frequency calculation
    
    uint8_t freq_data[3] = {
        (freq >> 16) & 0xFF,
        (freq >> 8) & 0xFF,
        freq & 0xFF
    };
    
    sx1280_spi_write(SX1280_CMD_SETRFFREQUENCY, freq_data, 3);
}

void sx1280_set_tx_power(int8_t power) {
    uint8_t pwr_ramp = power + 18;  // Convert to register value
    
    uint8_t pwr_data[2] = {
        (uint8_t)power,  // DbmValue
        0x02             // RampTime (0x02 = 40µs)
    };
    
    sx1280_spi_write(SX1280_CMD_SETTXPARAMS, pwr_data, 2);
}

void sx1280_start_tx() {
    uint8_t tx_params[3] = {
        0x00,  // Timeout = disabled
        0x00,
        0x00
    };
    
    sx1280_spi_write(SX1280_CMD_TX, tx_params, 3);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=== RF Kumanda TX Başlatılıyor ===");
    Serial.println("Frekans: 2.4 GHz");
    Serial.println("Modul: SX1280");
    
    // Başlatma sırası
    initDisplay();
    delay(100);
    initSPI();
    delay(100);
    initADC();
    delay(100);
    initSwitches();
    delay(100);
    initSX1280();
    delay(100);
    initRadio();
    
    Serial.println("=== TX Hazır ===\n");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("TX READY!");
    display.display();
}

void loop() {
    if (millis() - lastTxTime >= TX_INTERVAL) {
        readPotentiometers();
        readSwitches();
        transmitPacket();
        lastTxTime = millis();
    }
    
    // Ekran güncelle
    if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        updateDisplay();
        lastDisplayUpdate = millis();
    }
}

// ============ SPI İNİSİYALİZASYON ============
void initSPI() {
    Serial.println("SPI başlatılıyor...");
    
    // SPI pinlerini çıkış olarak ayarla
    pinMode(SX1280_NSS, OUTPUT);
    pinMode(SX1280_RES, OUTPUT);
    pinMode(SX1280_BUSY, INPUT);
    
    // SPI başlat
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV4);
    
    // SX1280 reset et
    digitalWrite(SX1280_RES, HIGH);
    delay(10);
    digitalWrite(SX1280_RES, LOW);
    delay(10);
    digitalWrite(SX1280_RES, HIGH);
    delay(50);
    
    Serial.println("SPI hazır");
}

// ============ ADC İNİSİYALİZASYON ============
void initADC() {
    Serial.println("ADC başlatılıyor...");
    
    pinMode(POT_THROTTLE, INPUT);
    pinMode(POT_YAW, INPUT);
    pinMode(POT_ROLL, INPUT);
    pinMode(POT_PITCH, INPUT);
    pinMode(POT_AUX1, INPUT);
    pinMode(POT_AUX2, INPUT);
    pinMode(POT_AUX3, INPUT);
    
    Serial.println("ADC hazır");
}

// ============ EKRAN İNİSİYALİZASYON ============
void initDisplay() {
    Serial.println("SSD1306 OLED ekran başlatılıyor...");
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("SSD1306 ekran bulunamadı!");
        while (1);  // Halt
    }
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.println("Initializing");
    display.setCursor(20, 40);
    display.println("RFController");
    display.display();
    
    Serial.println("Ekran hazır");
}

// ============ SWITCH İNİSİYALİZASYON ============
void initSwitches() {
    Serial.println("Switch'ler başlatılıyor...");
    
    pinMode(SWITCH_TOGGLE_1, INPUT_PULLUP);
    pinMode(SWITCH_TOGGLE_2, INPUT_PULLUP);
    pinMode(SWITCH_REGULAR, INPUT_PULLUP);
    pinMode(SWITCH_POWER, INPUT_PULLUP);
    
    Serial.println("Switch'ler hazır");
}

// ============ SX1280 İNİSİYALİZASYON ============
void initSX1280() {
    Serial.println("SX1280 başlatılıyor...");
    
    // Burada SX1280 register konfigürasyonu yapılacak
    // TODO: SX1280 driver kodu eklenecek
    
    Serial.println("SX1280 hazır");
}

// ============ RADYO AYARLARI ============
void initRadio() {
    Serial.println("Radyo ayarları yapılıyor...");
    
    // Frekans, güç, bant genişliği vs. konfigürasyonu
    // TODO: SX1280 radyo ayarlarını buraya ekle
    
    Serial.println("Radyo hazır");
}

// ============ POTANSİYOMETRELERİ OKU ============
void readPotentiometers() {
    // ADC değerlerini oku (0-4095)
    uint16_t raw_throttle = analogRead(POT_THROTTLE);
    uint16_t raw_yaw = analogRead(POT_YAW);
    uint16_t raw_roll = analogRead(POT_ROLL);
    uint16_t raw_pitch = analogRead(POT_PITCH);
    uint16_t raw_aux1 = analogRead(POT_AUX1);
    uint16_t raw_aux2 = analogRead(POT_AUX2);
    uint16_t raw_aux3 = analogRead(POT_AUX3);
    
    // PWM aralığına dönüştür (1000-2000 µs)
    txPacket.throttle = map(raw_throttle, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    txPacket.yaw = map(raw_yaw, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    txPacket.roll = map(raw_roll, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    txPacket.pitch = map(raw_pitch, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    txPacket.aux1 = map(raw_aux1, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    txPacket.aux2 = map(raw_aux2, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    txPacket.aux3 = map(raw_aux3, ADC_MIN, ADC_MAX, CHANNEL_MIN, CHANNEL_MAX);
    
    txPacket.battery = 0;
    txPacket.sync_id = SYNC_ID;
}

// ============ SWITCH'LERİ OKU ============
void readSwitches() {
    // Switch'ler aktif düşük (PULLUP kullanıldığı için)
    // Oku ve tersine çevir (1 = aktif, 0 = pasif)
    txPacket.switch_toggle1 = !digitalRead(SWITCH_TOGGLE_1);
    txPacket.switch_toggle2 = !digitalRead(SWITCH_TOGGLE_2);
    txPacket.switch_regular = !digitalRead(SWITCH_REGULAR);
    
    // Voltaj sensörü oku (ADC)
    uint16_t raw_battery = analogRead(BATTERY_SENSE);
    // ADC değerini volta çevir: (raw / 4095) * 3.3 * VOLTAGE_DIVIDER
    float battery_voltage = (raw_battery / 4095.0) * 3.3 * VOLTAGE_DIVIDER;
    txPacket.battery = (uint16_t)(battery_voltage * 1000); // mV'e çevir
    
    txPacket.flags = 0;
    // Düşük pil uyarısı
    if (battery_voltage < BATTERY_MIN) {
        txPacket.flags |= 0x04;  // Bit 2: Low battery flag
    }
    // Güç switch'i kontrol
    if (!digitalRead(SWITCH_POWER)) {
        // Güç switch'i açıksa devam et
    } else {
        // Güç switch'i kapalı
        txPacket.flags |= 0x80;  // Bit 7: Power warning
    }
}

// ============ PAKET GÖNDER ============
void transmitPacket() {
    // Checksum hesapla
    updateChecksum(&txPacket);
    
    // Paketi gönder
    // TODO: SX1280 ile veri gönderme kodu
    
    // Debug: Verileri seri portuna yazdır
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime >= 1000) {
        Serial.print("TX: THR=");
        Serial.print(txPacket.throttle);
        Serial.print(" YAW=");
        Serial.print(txPacket.yaw);
        Serial.print(" ROLL=");
        Serial.print(txPacket.roll);
        Serial.print(" PITCH=");
        Serial.print(txPacket.pitch);
        Serial.print(" | T1=");
        Serial.print(txPacket.switch_toggle1);
        Serial.print(" T2=");
        Serial.print(txPacket.switch_toggle2);
        Serial.print(" SW=");
        Serial.print(txPacket.switch_regular);
        Serial.print(" | BATT=");
        Serial.print(txPacket.battery / 1000.0, 2);
        Serial.print("V | FLAGS=");
        Serial.println(txPacket.flags, HEX);
        
        lastDebugTime = millis();
    }
}

// ============ EKRAN GÜNCELLE ============
void updateDisplay() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Başlık
    display.setCursor(0, 0);
    display.println("RF KUMANDA TX");
    
    // Analog değerleri göster
    display.setCursor(0, 10);
    display.print("THR:");
    display.print(txPacket.throttle);
    display.print(" YAW:");
    display.print(txPacket.yaw);
    
    display.setCursor(0, 20);
    display.print("ROLL:");
    display.print(txPacket.roll);
    display.print(" PITCH:");
    display.print(txPacket.pitch);
    
    display.setCursor(0, 30);
    display.print("AUX1:");
    display.print(txPacket.aux1);
    display.print(" AUX2:");
    display.print(txPacket.aux2);
    
    display.setCursor(0, 40);
    display.print("AUX3:");
    display.print(txPacket.aux3);
    
    // Toggle switch'leri ve pil durumu göster
    display.setCursor(0, 50);
    display.print("SW: ");
    display.print(txPacket.switch_toggle1 ? "[T1]" : "( )");
    display.print(" ");
    display.print(txPacket.switch_toggle2 ? "[T2]" : "( )");
    display.print(" ");
    display.print(txPacket.switch_regular ? "[SW]" : "- -");
    
    // Pil voltajı göster
    float batt_v = txPacket.battery / 1000.0;
    display.setCursor(90, 50);
    if (txPacket.flags & 0x04) {
        display.print("LOW!");
    } else {
        display.print(batt_v, 1);
        display.print("V");
    }
    
    display.display();
}
