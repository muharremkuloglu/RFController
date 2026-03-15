#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// ============ INLINE CONFIG ============
#define SX1280_NSS   5
#define SX1280_RES   17
#define SX1280_BUSY  4 
#define SX1280_DIO1  2
#define POT_THROTTLE 34
#define POT_YAW      35
#define POT_ROLL     32
#define POT_PITCH    33
#define POT_AUX1     25
#define POT_AUX2     26
#define POT_AUX3     27
#define BATTERY_SENSE 36
#define SWITCH_TOGGLE_1  12
#define SWITCH_TOGGLE_2  13
#define SWITCH_REGULAR   14
#define SWITCH_POWER     15
#define OLED_SDA     21
#define OLED_SCL     22
#define OLED_ADDRESS 0x3C
#define ADC_MIN      0
#define ADC_MAX      4095
#define CHANNEL_MIN  1000
#define CHANNEL_MAX  2000
#define BATTERY_MIN  8.0
#define BATTERY_MAX  8.4
#define VOLTAGE_DIVIDER 2.0
#define PACKET_SIZE  32
#define SYNC_ID      0xABCD
#define TX_RATE      50

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

void updateChecksum(ControlPacket* packet) {
    packet->checksum = calculateChecksum((uint8_t*)packet, sizeof(ControlPacket) - 1);
}

// ============ GLOBALS ============
Adafruit_SSD1306 display(128, 64, &Wire, -1);
ControlPacket txPacket;
unsigned long lastTxTime = 0;
const unsigned long TX_INTERVAL = 1000 / TX_RATE;
bool radioInitialized = false;
uint8_t packetCount = 0;
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 200;

// ============ SX1280 REGISTER ADDRESSES & COMMANDS ============
#define SX1280_CMD_RESET                     0x07
#define SX1280_CMD_SETPACKETTYPE             0x8A
#define SX1280_CMD_SETRFFREQUENCY            0x21
#define SX1280_CMD_SETMODULATIONPARAMS       0x22 
#define SX1280_CMD_SETTXPARAMS               0x20
#define SX1280_CMD_SETDIOIRQPARAMS           0x08
#define SX1280_CMD_TX                        0x13
#define SX1280_CMD_READREGISTER              0x1D
#define SX1280_CMD_WRITEBUFFER               0x1A
#define SX1280_CMD_GETSTATUS                 0xC0

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
        while(1);
    }
}

void loop() {
    if (millis() - lastTxTime >= TX_INTERVAL) {
        readPotentiometers();
        readSwitches();
        transmitPacket();
        lastTxTime = millis();
    }
    
    if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        updateDisplay();
        lastDisplayUpdate = millis();
    }
    
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
    
    analogSetWidth(12);
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
    
    sx1280_set_tx_power(13);
    delay(10);
    
    uint8_t mod_params[3] = {0x34, 0x07, 0x01};
    sx1280_spi_write(SX1280_CMD_SETMODULATIONPARAMS, mod_params, 3);
    delay(10);
    
    uint8_t dio_params[8] = {0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    sx1280_spi_write(SX1280_CMD_SETDIOIRQPARAMS, dio_params, 8);
    delay(10);
    
    radioInitialized = true;
    Serial.println("✓ SX1280 (E28-2G4M12S) hazır");
    Serial.println("  • Frekans: 2400 MHz");
    Serial.println("  • Güç: 13 dBm");
    Serial.println("  • SF: 7, BW: 812kHz");
}

void readPotentiometers() {
    uint16_t raw_throttle = analogRead(POT_THROTTLE);
    uint16_t raw_yaw = analogRead(POT_YAW);
    uint16_t raw_roll = analogRead(POT_ROLL);
    uint16_t raw_pitch = analogRead(POT_PITCH);
    uint16_t raw_aux1 = analogRead(POT_AUX1);
    uint16_t raw_aux2 = analogRead(POT_AUX2);
    uint16_t raw_aux3 = analogRead(POT_AUX3);
    
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
    txPacket.switch_toggle1 = !digitalRead(SWITCH_TOGGLE_1);
    txPacket.switch_toggle2 = !digitalRead(SWITCH_TOGGLE_2);
    txPacket.switch_regular = !digitalRead(SWITCH_REGULAR);
    
    uint16_t raw_battery = analogRead(BATTERY_SENSE);
    float battery_voltage = (raw_battery / 4095.0) * 3.3 * VOLTAGE_DIVIDER;
    txPacket.battery = (uint16_t)(battery_voltage * 1000);
    
    txPacket.flags = 0;
    
    if (battery_voltage < BATTERY_MIN) {
        txPacket.flags |= 0x04;
    }
    
    if (digitalRead(SWITCH_POWER)) {
        txPacket.flags |= 0x80;
    }
}

void transmitPacket() {
    updateChecksum(&txPacket);
    
    uint8_t buffer[PACKET_SIZE];
    memcpy(buffer, (uint8_t*)&txPacket, sizeof(txPacket));
    
    digitalWrite(SX1280_NSS, LOW);
    SPI.transfer(SX1280_CMD_WRITEBUFFER);
    SPI.transfer(0x00);
    for (int i = 0; i < PACKET_SIZE; i++) {
        SPI.transfer(buffer[i]);
    }
    digitalWrite(SX1280_NSS, HIGH);
    delayMicroseconds(100);
    
    sx1280_start_tx();
    
    packetCount++;
    
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

void updateDisplay() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("RF KUMANDA TX | PKT:");
    display.println(packetCount);
    
    display.setCursor(0, 10);
    display.print("THR:");
    display.print(txPacket.throttle - 1000);
    display.print(" YAW:");
    display.print(txPacket.yaw - 1000);
    display.print(" ROLL:");
    display.println(txPacket.roll - 1000);
    
    display.setCursor(0, 20);
    display.print("PITCH:");
    display.print(txPacket.pitch - 1000);
    display.print(" AUX1:");
    display.print(txPacket.aux1 - 1000);
    display.print(" AUX2:");
    display.println(txPacket.aux2 - 1000);
    
    display.setCursor(0, 30);
    display.print("AUX3:");
    display.print(txPacket.aux3 - 1000);
    display.print(" | SW: ");
    display.print(txPacket.switch_toggle1 ? "T1 " : "-- ");
    display.print(txPacket.switch_toggle2 ? "T2 " : "-- ");
    display.println(txPacket.switch_regular ? "SW" : "--");
    
    float batt_v = txPacket.battery / 1000.0;
    display.setCursor(0, 40);
    display.print("BATT: ");
    if (txPacket.flags & 0x04) {
        display.print("LOW!");
    } else {
        display.print(batt_v, 2);
        display.print("V");
    }
    
    display.setCursor(0, 50);
    if (radioInitialized) {
        display.print("STATUS: OK");
    } else {
        display.print("STATUS: ERROR");
    }
    
    display.setCursor(100, 50);
    if (digitalRead(SWITCH_POWER)) {
        display.print("PWR!");
    } else {
        display.print("PWR");
    }
    
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
    
    uint8_t freq_data[3] = {
        (freq >> 16) & 0xFF,
        (freq >> 8) & 0xFF,
        freq & 0xFF
    };
    
    sx1280_spi_write(SX1280_CMD_SETRFFREQUENCY, freq_data, 3);
}

void sx1280_set_tx_power(int8_t power) {
    uint8_t pwr_data[2] = {
        (uint8_t)power,
        0x02
    };
    
    sx1280_spi_write(SX1280_CMD_SETTXPARAMS, pwr_data, 2);
}

void sx1280_start_tx() {
    uint8_t tx_params[3] = {0x00, 0x00, 0x00};
    sx1280_spi_write(SX1280_CMD_TX, tx_params, 3);
}
