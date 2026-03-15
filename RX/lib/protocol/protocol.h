#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// Veri Paketi Yapısı
struct ControlPacket {
    uint16_t sync_id;      // Senkronizasyon ID (0xABCD)
    uint16_t throttle;     // 1000-2000 µs
    uint16_t yaw;          // 1000-2000 µs
    uint16_t roll;         // 1000-2000 µs
    uint16_t pitch;        // 1000-2000 µs
    uint16_t aux1;         // Yardımcı kanal 1 (POT_AUX1)
    uint16_t aux2;         // Yardımcı kanal 2 (POT_AUX2)
    uint16_t aux3;         // Yardımcı kanal 3 (POT_AUX3)
    uint16_t battery;      // Pil voltajı (mV)
    uint8_t switch_toggle1;// Toggle switch 1 (0 veya 1)
    uint8_t switch_toggle2;// Toggle switch 2 (0 veya 1)
    uint8_t switch_regular;// Normal switch (0 veya 1)
    uint8_t flags;         // Bayraklar
    uint8_t checksum;      // Kontrol toplamı
} __attribute__((packed));

// Paket boyutu doğrulaması
_Static_assert(sizeof(ControlPacket) <= 32, "ControlPacket boyutu 32 byte'ı aşamaz!");

// ============ FONKSİYONLAR ============

uint8_t calculateChecksum(const uint8_t* data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum ^= data[i];
    }
    return sum;
}

bool validatePacket(const ControlPacket* packet) {
    // Senkronizasyon ID kontrolü
    if (packet->sync_id != 0xABCD) {
        return false;
    }
    
    // Checksum kontrolü
    uint8_t checksum = calculateChecksum((uint8_t*)packet, sizeof(ControlPacket) - 1);
    if (checksum != packet->checksum) {
        return false;
    }
    
    return true;
}

void updateChecksum(ControlPacket* packet) {
    packet->checksum = calculateChecksum((uint8_t*)packet, sizeof(ControlPacket) - 1);
}

#endif
