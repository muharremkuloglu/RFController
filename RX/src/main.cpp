// ============================================================================
// 🛰️ RX/src/main.cpp - RF-SBUS BRIDGE (İHA'ya Monte Edilen Alıcı)
// ============================================================================
//
// İŞLEV:
//   - SX1280 RF modülü ile 2.4GHz paketleri KONTINYU AL
//   - ControlPacket'ı SBUS protokolüne dönüştür
//   - UART1 / GPIO43 üzerinden Orange Cube autopilot'a gönder (100kbaud, INVERTED!)
//   - Status LED blink (RF bağlantı göstergesi)
//
// FLOW:
//   1. SX1280 RX mode'da başlat (kontinyu duş)
//   2. Interrupt (GPIO5=DIO1) yla paket alındığında işle
//   3. ControlPacket struct'ı parse et
//   4. Her 40ms'de SBUS frame oluştur (25Hz rate)
//   5. GPIO43 (UART1_TX) → INVERTED UART → Orange Cube SBUS IN
//   6. Signal loss detect (500ms timeout → failsafe flag)
//
// SBUS FRAME FORMAT (25 byte):
//   [0]    = 0x0F (start byte)
//   [1-22] = 16 channel × 11-bit data (176 bits, complex bit packing)
//   [23]   = 0x00 (flags/status)
//   [24]   = 0x00 (end byte)
//   Baudrate: 100,000 bps (100 kbaud - NON-STANDARD!)
//   Format: SERIAL_8E2 (8 data, Even parity, 2 stop)
//   Signal: INVERTED (idle=LOW, active=HIGH) ← KRITIK!
//
// KRİTİK NOKTALAR:
//   ⚠️ GPIO43 UART1_TX INVERTED olMALI:
//      uart_set_line_inverse(UART_NUM_1, UART_INVERSE_TXD);
//   ⚠️ 100kbaud = 100,000 bps (not 115200!) → timing hassas
//   ⚠️ 11-bit channel encoding = kompleks bit packing
//   ⚠️ SBUS rate = 25Hz ANCAK TX rate = 50Hz (farklı!)
//   ⚠️ Signal loss timeout = 500ms → failsafe flag set
//
// EKSIK KODLAR (⚠️ YAPILACAK):
//   - encodeSBUSFrame() fonksiyonu tamamlanmadı
//   - sendSBUSFrame() UART1 implementasyonu eksik
//   - uart_set_line_inverse() setup kodu dikkatle test lazım
//   - Servo PWM backup mode (analog PWM if UART fails)
//
// İLGİLİ DOSYALAR:
//   - shared/config.h          → PIN tanımları (especially GPIO43)
//   - docs/HARDWARE.md         → RX pin haritası, Orange Cube bağlantı
//   - docs/SBUS_PROTOCOL.md    → 25-byte frame encode/decode örnekleri
//   - TX/src/main.cpp          → RF sender (ControlPacket source)
//   - RX/lib/config/config.h   → RX-specific ayarlar (şu an boş)
//
// ============================================================================

#include <Arduino.h>
#include <SPI.h>
#include "config.h"
#include "protocol.h"

// ============ GLOBALS ============
ControlPacket rxPacket;
ControlPacket lastValidPacket;
unsigned long lastPacketTime = 0;
const unsigned long PACKET_TIMEOUT = 500;  // 500ms timeout

bool radioInitialized = false;
volatile bool rxDone = false;
uint8_t packetCount = 0;
uint32_t lostPackets = 0;
uint32_t totalPackets = 0;

// Servo PWM pins (Channel mapping)
const uint8_t servoPin[7] = {
    GPIO_SERVO_THROTTLE,  // Ch1: Throttle
    GPIO_SERVO_YAW,       // Ch2: Yaw
    GPIO_SERVO_ROLL,      // Ch3: Roll
    GPIO_SERVO_PITCH,     // Ch4: Pitch
    GPIO_SERVO_AUX1,      // Ch5: Aux1
    GPIO_SERVO_AUX2,      // Ch6: Aux2
    GPIO_SERVO_AUX3       // Ch7: Aux3
};

// Servo PWM values
uint16_t servoPwm[7];
unsigned long lastServoUpdate = 0;
const unsigned long SERVO_UPDATE_INTERVAL = 20;  // 50Hz

// Status LED
unsigned long lastStatusBlink = 0;
bool statusLedState = false;

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
#define SX1280_REG_IRQ_STATUS                0x0E02

// ============ FUNCTION PROTOTYPES ============
void initSPI();
void initRX();
void initServoOutputs();
void initSX1280();
void updateServoOutputs();
void handleReceivedPacket();
void transmitFailsafe();
void sx1280_spi_write(uint8_t cmd, uint8_t *data, uint8_t len);
void sx1280_spi_read(uint8_t cmd, uint8_t *data, uint8_t len);
void sx1280_reset();
void sx1280_set_frequency(uint32_t freq);  // Frekans ayarla (2400-2500 MHz)
void sx1280_set_rx_params(uint16_t timeout);
void sx1280_start_rx();
void sx1280_wait_ready();
uint8_t sx1280_read_irq_status();
void processReceivedData();
void blinkStatusLed();

// ============ RF LINK BUDGET HESAPLAMA FONKSİYONLARI ============
// Friis Path Loss Denklemi ile haberleşme analizi
float calculatePathLoss(uint32_t freq_mhz, float distance_km);     // Path Loss (dB)
float calculateRxPower(float tx_power_dbm, float path_loss_db);   // RX Power (dBm)
float calculateSNR(float rx_power_dbm, float rx_sensitivity_dbm); // SNR (dB)
float calculateFadeMargin(float snr_db);                          // Fade Margin (dB)
void printLinkBudget(uint32_t freq_mhz, float distance_km);       // Debug output
bool setFrequency(uint32_t freq_mhz);                             // Frekans güvenli set

// ============ SETUP & LOOP ============
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n╔════════════════════════════════╗");
    Serial.println("║  RF KUMANDA RX BAŞLATILIYOR    ║");
    Serial.println("║  Frekans: 2.4 GHz              ║");
    Serial.println("║  Modul: SX1280 E28-2G4M12S     ║");
    Serial.println("║  Çıkış: 7 Servo (PWM)          ║");
    Serial.println("╚════════════════════════════════╝\n");
    
    // Başlatma sırası
    Serial.println("[1/5] SPI başlatılıyor...");
    initSPI();
    delay(100);
    
    Serial.println("[2/5] Servo çıkışları hazırlanıyor...");
    initServoOutputs();
    delay(100);
    
    Serial.println("[3/5] Status LED hazırlanıyor...");
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);
    delay(100);
    
    Serial.println("[4/5] SX1280 başlatılıyor...");
    initSX1280();
    delay(500);
    
    if (radioInitialized) {
        Serial.println("[5/5] ✓ HAZIR!\n");
        
        // RX Haberleşme parametrelerini göster
        printLinkBudget(FREQUENCY_DEFAULT, 0.5);  // 500m mesafede göster
        
        delay(1000);
    } else {
        Serial.println("[5/5] ✗ Hata: Radio başlatılamadı!\n");
        while(1);  // Halt
    }
    
    // Failsafe durumu (Throttle = 1000, Yaw = 1500, vb.)
    for (int i = 0; i < 7; i++) {
        servoPwm[i] = 1500;  // Center/Safe position
    }
    lastPacketTime = millis();
}

void loop() {
    // RX Mode'de veri bekleniyor
    uint8_t irqStatus = sx1280_read_irq_status();
    
    if (irqStatus & 0x02) {  // RX done
        processReceivedData();
        sx1280_start_rx();  // Sürekli RX
    }
    
    // Timeout check: 500ms sonra failsafe
    if (millis() - lastPacketTime > PACKET_TIMEOUT) {
        lostPackets++;
        transmitFailsafe();
    }
    
    // Servo update (50Hz)
    if (millis() - lastServoUpdate >= SERVO_UPDATE_INTERVAL) {
        updateServoOutputs();
        lastServoUpdate = millis();
    }
    
    // Status LED blink
    blinkStatusLed();
    
    // CPU yükü azaltmak
    delayMicroseconds(100);
}

// ============ INITIALIZATION ============
void initSPI() {
    pinMode(SX1280_NSS, OUTPUT);
    pinMode(SX1280_RES, OUTPUT);
    pinMode(SX1280_BUSY, INPUT);
    
    SPI.begin();
    SPI.setFrequency(10000000);  // 10 MHz
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    
    digitalWrite(SX1280_NSS, HIGH);
    Serial.println("✓ SPI hazır (10 MHz)");
}

void initServoOutputs() {
    // PWM çıkışları
    for (int i = 0; i < 7; i++) {
        pinMode(servoPin[i], OUTPUT);
        digitalWrite(servoPin[i], LOW);
        servoPwm[i] = 1500;  // Center position
    }
    
    Serial.println("✓ 7 x Servo çıkışı hazır");
    Serial.println("  CH1(Motor): GPIO 14, CH2(Yaw): GPIO 13");
    Serial.println("  CH3(Roll): GPIO 12, CH4(Pitch): GPIO 27");
    Serial.println("  CH5-7: GPIO 25,26,11");
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
    
    // RF Frequency: 2400 MHz (TX ile same)
    sx1280_set_frequency(2400);
    delay(10);
    
    // Modulation: LoRa (BW=812kHz, SF=7) - TX ile same
    uint8_t mod_params[3] = {
        0x34,  // Bandwidth 812 kHz
        0x07,  // Spreading Factor 7
        0x01   // Coding Rate 4/5
    };
    sx1280_spi_write(SX1280_CMD_SETMODULATIONPARAMS, mod_params, 3);
    delay(10);
    
    // RX Parameters
    sx1280_set_rx_params(0xFFFF);  // Infinite RX timeout
    delay(10);
    
    // DIO IRQ (RX done)
    uint8_t dio_params[8] = {
        0x02, 0x00,  // Rx done IRQ
        0x00, 0x00,  // Tx done IRQ
        0x00, 0x00,  // Rx timeout
        0x00, 0x00   // Sync address
    };
    sx1280_spi_write(SX1280_CMD_SETDIOIRQPARAMS, dio_params, 8);
    delay(10);
    
    radioInitialized = true;
    Serial.println("✓ SX1280 (E28-2G4M12S) hazır");
    Serial.println("  • Frekans: 2400 MHz");
    Serial.println("  • SF: 7, BW: 812kHz");
    Serial.println("  • Mode: RX (Continuous)");
    
    // Start continuous RX
    sx1280_start_rx();
}

// ============ DATA PROCESSING ============
void processReceivedData() {
    // Read from RX buffer
    uint8_t buffer[PACKET_SIZE];
    
    digitalWrite(SX1280_NSS, LOW);
    SPI.transfer(SX1280_CMD_READBUFFER);
    SPI.transfer(0x00);  // Offset
    for (int i = 0; i < PACKET_SIZE; i++) {
        buffer[i] = SPI.transfer(0x00);
    }
    digitalWrite(SX1280_NSS, HIGH);
    delayMicroseconds(100);
    
    // Copy to packet
    memcpy((uint8_t*)&rxPacket, buffer, sizeof(rxPacket));
    
    // Validate checksum
    if (validatePacket(&rxPacket)) {
        lastValidPacket = rxPacket;
        lastPacketTime = millis();
        totalPackets++;
        packetCount++;
        
        // Sync ID kontrolü
        if (rxPacket.sync_id != SYNC_ID) {
            Serial.println("[RX] Uyarı: Sync ID mismatch!");
            return;
        }
        
        handleReceivedPacket();
        
        // Debug output (every 2 secs)
        static unsigned long lastDebugTime = 0;
        if (millis() - lastDebugTime >= 2000) {
            Serial.print("[RX] Paket #");
            Serial.print(packetCount);
            Serial.print(" | THR:");
            Serial.print(rxPacket.throttle);
            Serial.print(" BATT:");
            Serial.print(rxPacket.battery / 1000.0, 2);
            Serial.print("V | Loss: ");
            if (totalPackets > 0) {
                Serial.print((lostPackets * 100) / totalPackets);
                Serial.println("%");
            }
            lastDebugTime = millis();
        }
    } else {
        Serial.println("[RX] ✗ Checksum hatası!");
        lostPackets++;
    }
}

void handleReceivedPacket() {
    // Servos'a aktarma (1000-2000 µs PWM)
    servoPwm[0] = rxPacket.throttle;
    servoPwm[1] = rxPacket.yaw;
    servoPwm[2] = rxPacket.roll;
    servoPwm[3] = rxPacket.pitch;
    servoPwm[4] = rxPacket.aux1;
    servoPwm[5] = rxPacket.aux2;
    servoPwm[6] = rxPacket.aux3;
    
    // Battery monitoring
    if (rxPacket.flags & 0x04) {
        Serial.println("[RX] TX Bataryası Düşük!");
    }
    
    if (rxPacket.flags & 0x80) {
        Serial.println("[RX] TX Power Uyarısı!");
    }
}

void updateServoOutputs() {
    // Software PWM - 50Hz
    // Her servo için: HIGH dur, 1000-2000µs dur, LOW dur
    // 20ms frame'de: (PWM_value - 1000) = HIGH duration microseconds
    
    const uint32_t framePeriod = 20000;  // 20ms
    
    for (int i = 0; i < 7; i++) {
        digitalWrite(servoPin[i], HIGH);
        delayMicroseconds(servoPwm[i]);
        digitalWrite(servoPin[i], LOW);
        delayMicroseconds(framePeriod - servoPwm[i]);
    }
}

void transmitFailsafe() {
    // Motor kapalı: Throttle = 1000 (safe value)
    servoPwm[0] = 1000;
    
    // Diğer channels center
    for (int i = 1; i < 7; i++) {
        servoPwm[i] = 1500;
    }
    
    digitalWrite(STATUS_LED, HIGH);  // LED on to indicate failsafe
}

void blinkStatusLed() {
    // LED blink pattern
    if (millis() - lastStatusBlink >= 500) {
        statusLedState = !statusLedState;
        
        if (millis() - lastPacketTime > PACKET_TIMEOUT) {
            // Failsafe mode: fast blink
            digitalWrite(STATUS_LED, statusLedState);
            lastStatusBlink = millis();
        } else {
            // Normal mode: slow blink
            digitalWrite(STATUS_LED, statusLedState);
            lastStatusBlink = millis();
        }
    }
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
    uint32_t freq = (freq_mhz << 17) / 1000 / 61;
    
    uint8_t freq_data[3] = {
        (freq >> 16) & 0xFF,
        (freq >> 8) & 0xFF,
        freq & 0xFF
    };
    
    sx1280_spi_write(SX1280_CMD_SETRFFREQUENCY, freq_data, 3);
}

void sx1280_set_rx_params(uint16_t timeout) {
    uint8_t rx_params[2] = {
        (timeout >> 8) & 0xFF,
        timeout & 0xFF
    };
    
    sx1280_spi_write(SX1280_CMD_SETRXPARAMS, rx_params, 2);
}

void sx1280_start_rx() {
    uint8_t rx_cmd[3] = {
        0xFF,  // Timeout = max
        0xFF,
        0x00
    };
    
    sx1280_spi_write(SX1280_CMD_RX, rx_cmd, 3);
}

uint8_t sx1280_read_irq_status() {
    uint8_t status;
    
    sx1280_wait_ready();
    
    digitalWrite(SX1280_NSS, LOW);
    SPI.transfer(SX1280_CMD_GETSTATUS);
    status = SPI.transfer(0x00);
    digitalWrite(SX1280_NSS, HIGH);
    
    delayMicroseconds(100);
    
    return status & 0x0F;  // Return only IRQ bits
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
    
    // Frekans, bant genişliği vs. konfigürasyonu
    // TODO: SX1280 radyo ayarlarını buraya ekle
    
    Serial.println("Radyo hazır - Alıcı modunda...");
}

// ============ ALICI LOOP ============
void receiverLoop() {
    // TODO: SX1280 veri alma kodu
    // Şu an placeholder
    delay(10);
}

// ============ PAKET İŞLE ============
void processPacket() {
    // Paketi doğrula
    if (!validatePacket(&rxPacket)) {
        Serial.println("Hata: Sistem kontrol toplamı!");
        return;
    }
    
    lastRxTime = millis();
    
    // PWM sinyalleri oluştur veya servoya gönder
    // TODO: Servo kütüphanesi ile PWM sinyallerini oluştur
    
    Serial.print("RX: Throttle=");
    Serial.print(rxPacket.throttle);
    Serial.print(" Yaw=");
    Serial.print(rxPacket.yaw);
    Serial.print(" Roll=");
    Serial.print(rxPacket.roll);
    Serial.print(" Pitch=");
    Serial.println(rxPacket.pitch);
}
