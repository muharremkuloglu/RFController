#include <Arduino.h>
#include <SPI.h>

// ============ INLINE CONFIG ============
#define SX1280_NSS   5
#define SX1280_RES   17
#define SX1280_BUSY  4
#define SX1280_DIO1  2
#define GPIO_SERVO_THROTTLE  14
#define GPIO_SERVO_YAW       13
#define GPIO_SERVO_ROLL      12
#define GPIO_SERVO_PITCH     27
#define GPIO_SERVO_AUX1      25
#define GPIO_SERVO_AUX2      26
#define GPIO_SERVO_AUX3      11
#define STATUS_LED           15
#define PACKET_SIZE  32
#define SYNC_ID      0xABCD

// ============ PROTOCOL PACKET ============
struct ControlPacket {
    uint16_t sync_id;
    uint16_t throttle;
    uint16_t yaw;
    uint16_t roll;
    uint16_t pitch;
    uint16_t aux1;
    uint16_t aux2;
    uint16_t aux3;
    uint16_t battery;
    uint8_t switch_toggle1;
    uint8_t switch_toggle2;
    uint8_t switch_regular;
    uint8_t flags;
    uint8_t checksum;
} __attribute__((packed));

// ============ PROTOCOL HELPERS ============
uint8_t calculateChecksum(const uint8_t* data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum ^= data[i];
    }
    return sum;
}

bool validatePacket(const ControlPacket* packet) {
    if (packet->sync_id != SYNC_ID) return false;
    uint8_t checksum = calculateChecksum((uint8_t*)packet, sizeof(ControlPacket) - 1);
    return (checksum == packet->checksum);
}

// ============ GLOBALS ============
ControlPacket rxPacket;
ControlPacket lastValidPacket;
unsigned long lastPacketTime = 0;
const unsigned long PACKET_TIMEOUT = 500;

bool radioInitialized = false;
uint8_t packetCount = 0;
uint32_t lostPackets = 0;
uint32_t totalPackets = 0;

const uint8_t servoPin[7] = {
    GPIO_SERVO_THROTTLE, GPIO_SERVO_YAW, GPIO_SERVO_ROLL,
    GPIO_SERVO_PITCH, GPIO_SERVO_AUX1, GPIO_SERVO_AUX2, GPIO_SERVO_AUX3
};

uint16_t servoPwm[7];
unsigned long lastServoUpdate = 0;
const unsigned long SERVO_UPDATE_INTERVAL = 20;

unsigned long lastStatusBlink = 0;
bool statusLedState = false;

// ============ SX1280 REGISTER ADDRESSES ============
#define SX1280_CMD_RESET                     0x07
#define SX1280_CMD_SETPACKETTYPE             0x8A
#define SX1280_CMD_SETRFFREQUENCY            0x21
#define SX1280_CMD_SETMODULATIONPARAMS       0x22
#define SX1280_CMD_SETRXPARAMS               0x23
#define SX1280_CMD_SETDIOIRQPARAMS           0x08
#define SX1280_CMD_RX                        0x14
#define SX1280_CMD_READREGISTER              0x1D
#define SX1280_CMD_READBUFFER                0x1E
#define SX1280_CMD_GETSTATUS                 0xC0

// ============ FUNCTION PROTOTYPES ============
void initSPI();
void initServoOutputs();
void initSX1280();
void updateServoOutputs();
void handleReceivedPacket();
void transmitFailsafe();
void sx1280_spi_write(uint8_t cmd, uint8_t *data, uint8_t len);
void sx1280_spi_read(uint8_t cmd, uint8_t *data, uint8_t len);
void sx1280_reset();
void sx1280_set_frequency(uint32_t freq);
void sx1280_set_rx_params(uint16_t timeout);
void sx1280_start_rx();
void sx1280_wait_ready();
uint8_t sx1280_read_irq_status();
void processReceivedData();
void blinkStatusLed();

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
        delay(1000);
    } else {
        Serial.println("[5/5] ✗ Hata: Radio başlatılamadı!\n");
        while(1);
    }
    
    for (int i = 0; i < 7; i++) {
        servoPwm[i] = 1500;
    }
    lastPacketTime = millis();
}

void loop() {
    uint8_t irqStatus = sx1280_read_irq_status();
    
    if (irqStatus & 0x02) {
        processReceivedData();
        sx1280_start_rx();
    }
    
    if (millis() - lastPacketTime > PACKET_TIMEOUT) {
        lostPackets++;
        transmitFailsafe();
    }
    
    if (millis() - lastServoUpdate >= SERVO_UPDATE_INTERVAL) {
        updateServoOutputs();
        lastServoUpdate = millis();
    }
    
    blinkStatusLed();
    
    delayMicroseconds(100);
}

// ============ INITIALIZATION ============
void initSPI() {
    pinMode(SX1280_NSS, OUTPUT);
    pinMode(SX1280_RES, OUTPUT);
    pinMode(SX1280_BUSY, INPUT);
    
    SPI.begin();
    SPI.setFrequency(10000000);
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    
    digitalWrite(SX1280_NSS, HIGH);
    Serial.println("✓ SPI hazır (10 MHz)");
}

void initServoOutputs() {
    for (int i = 0; i < 7; i++) {
        pinMode(servoPin[i], OUTPUT);
        digitalWrite(servoPin[i], LOW);
        servoPwm[i] = 1500;
    }
    
    Serial.println("✓ 7 x Servo çıkışı hazır");
    Serial.println("  CH1(Motor): GPIO 14, CH2(Yaw): GPIO 13");
    Serial.println("  CH3(Roll): GPIO 12, CH4(Pitch): GPIO 27");
    Serial.println("  CH5-7: GPIO 25,26,11");
}

void initSX1280() {
    sx1280_reset();
    delay(100);
    sx1280_wait_ready();
    
    uint8_t fw[2];
    sx1280_spi_read(SX1280_CMD_READREGISTER, fw, 2);
    Serial.print("  FW Version: 0x");
    Serial.println(fw[0], HEX);
    
    uint8_t pkt_type = 0x01;
    sx1280_spi_write(SX1280_CMD_SETPACKETTYPE, &pkt_type, 1);
    delay(10);
    
    sx1280_set_frequency(2400);
    delay(10);
    
    uint8_t mod_params[3] = {0x34, 0x07, 0x01};
    sx1280_spi_write(SX1280_CMD_SETMODULATIONPARAMS, mod_params, 3);
    delay(10);
    
    sx1280_set_rx_params(0xFFFF);
    delay(10);
    
    uint8_t dio_params[8] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    sx1280_spi_write(SX1280_CMD_SETDIOIRQPARAMS, dio_params, 8);
    delay(10);
    
    radioInitialized = true;
    Serial.println("✓ SX1280 (E28-2G4M12S) hazır");
    Serial.println("  • Frekans: 2400 MHz");
    Serial.println("  • SF: 7, BW: 812kHz");
    Serial.println("  • Mode: RX (Continuous)");
    
    sx1280_start_rx();
}

void processReceivedData() {
    uint8_t buffer[PACKET_SIZE];
    
    digitalWrite(SX1280_NSS, LOW);
    SPI.transfer(SX1280_CMD_READBUFFER);
    SPI.transfer(0x00);
    for (int i = 0; i < PACKET_SIZE; i++) {
        buffer[i] = SPI.transfer(0x00);
    }
    digitalWrite(SX1280_NSS, HIGH);
    delayMicroseconds(100);
    
    memcpy((uint8_t*)&rxPacket, buffer, sizeof(rxPacket));
    
    if (validatePacket(&rxPacket)) {
        lastValidPacket = rxPacket;
        lastPacketTime = millis();
        totalPackets++;
        packetCount++;
        
        if (rxPacket.sync_id != SYNC_ID) {
            Serial.println("[RX] Uyarı: Sync ID mismatch!");
            return;
        }
        
        handleReceivedPacket();
        
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
    servoPwm[0] = rxPacket.throttle;
    servoPwm[1] = rxPacket.yaw;
    servoPwm[2] = rxPacket.roll;
    servoPwm[3] = rxPacket.pitch;
    servoPwm[4] = rxPacket.aux1;
    servoPwm[5] = rxPacket.aux2;
    servoPwm[6] = rxPacket.aux3;
    
    if (rxPacket.flags & 0x04) {
        Serial.println("[RX] TX Bataryası Düşük!");
    }
    
    if (rxPacket.flags & 0x80) {
        Serial.println("[RX] TX Power Uyarısı!");
    }
}

void updateServoOutputs() {
    const uint32_t framePeriod = 20000;
    
    for (int i = 0; i < 7; i++) {
        digitalWrite(servoPin[i], HIGH);
        delayMicroseconds(servoPwm[i]);
        digitalWrite(servoPin[i], LOW);
        delayMicroseconds(framePeriod - servoPwm[i]);
    }
}

void transmitFailsafe() {
    servoPwm[0] = 1000;
    for (int i = 1; i < 7; i++) {
        servoPwm[i] = 1500;
    }
    digitalWrite(STATUS_LED, HIGH);
}

void blinkStatusLed() {
    if (millis() - lastStatusBlink >= 500) {
        statusLedState = !statusLedState;
        
        if (millis() - lastPacketTime > PACKET_TIMEOUT) {
            digitalWrite(STATUS_LED, statusLedState);
        } else {
            digitalWrite(STATUS_LED, statusLedState);
        }
        lastStatusBlink = millis();
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
    SPI.transfer(0x00);
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
    uint8_t freq_data[3] = {(freq >> 16) & 0xFF, (freq >> 8) & 0xFF, freq & 0xFF};
    sx1280_spi_write(SX1280_CMD_SETRFFREQUENCY, freq_data, 3);
}

void sx1280_set_rx_params(uint16_t timeout) {
    uint8_t rx_params[2] = {(timeout >> 8) & 0xFF, timeout & 0xFF};
    sx1280_spi_write(SX1280_CMD_SETRXPARAMS, rx_params, 2);
}

void sx1280_start_rx() {
    uint8_t rx_cmd[3] = {0xFF, 0xFF, 0x00};
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
    
    return status & 0x0F;
}
