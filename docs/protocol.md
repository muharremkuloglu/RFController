<!--
================================================================================
📜 docs/protocol.md - TX ↔ RX RF İletişim Protokolü
================================================================================

İŞLEV:
  - TX kumanda → SX1280 RF transmission
  - RX alıcı ← SX1280 RF reception
  - ControlPacket struct formatı ve bit layout
  - 32 byte veri paketi (kumandaya özgü)

NOT: Bu SBUS'tan FARKLI!
  - ControlPacket = TX/RX arasında RF üzerinden (SX1280 payload)
  - SBUS = RX → Orange Cube arasında UART üzerinden (GPIO43)

KRİTİK NOKTALAR:
  ⚠️ TX RATE = 50Hz (20ms aralıklar)
  ⚠️ Paket boyutu = 32 bytes
  ⚠️ Sync ID = 0xABCD (paket doğrulama)
  ⚠️ Packet ID = increment with each transmission (debug)
  ⚠️ CRC doğrulama = RX tarafında (geçerse SBUS encode)

PAKET LAYOUT (32 byte):
  [0-1]   : sync_id (0xABCD)
  [2-3]   : packet_id (0-65535)
  [4-5]   : throttle (0-2047)
  [6-7]   : yaw (0-2047)
  [8-9]   : roll (0-2047)
  [10-11] : pitch (0-2047)
  [12-13] : trim_roll (0-2047)
  [14-15] : trim_pitch (0-2047)
  [16-17] : trim_yaw (0-2047)
  [18]    : flight_mode (0=Auto, 1=FBWA)
  [19]    : trim_locked (0/1)
  [20-23] : reserved (future use)
  [24-31] : CRC+padding

İLGİLİ DOSYALAR:
  - TX/src/main.cpp → ControlPacket oluştur + gönder
  - RX/src/main.cpp → ControlPacket al + SBUS'a dönüştür
  - docs/SBUS_PROTOCOL.md → RX→Orange Cube (16 kanal mapping)
  - shared/config.h → 32-byte struct tanımı

REFERANS:
  - SX1280 bitrate: autoslect (depends on SF8/SF7)
  - CRC: Standard 16-bit CRC (optional)
  - Failsafe: Timeout > 500ms → Orange Cube failsafe trigger

================================================================================
-->

# RF Kumanda Protokolü

## Veri Paketi Yapısı

Toplam: 32 byte

```
Offset  | Boyut | Ad           | Açıklama
--------|-------|--------------|--------------------------------------
0-1     | 2B    | sync_id      | Senkronizasyon ID (0xABCD)
2-3     | 2B    | throttle     | Gaz değeri (1000-2000 µs)
4-5     | 2B    | yaw          | Yaw değeri (1000-2000 µs)
6-7     | 2B    | roll         | Roll değeri (1000-2000 µs)
8-9     | 2B    | pitch        | Pitch değeri (1000-2000 µs)
10-11   | 2B    | aux1         | Yardımcı kanal 1
12-13   | 2B    | aux2         | Yardımcı kanal 2
14-15   | 2B    | battery      | Pil voltajı (mV)
16      | 1B    | flags        | Bayraklar (bit maskeleri)
17-31   | 15B   | -            | Başlangıçta kullanılmayan
```

## PWM Değerleri

- **Minimum**: 1000 µs (0%)
- **Orta**: 1500 µs (50%)
- **Maksimum**: 2000 µs (100%)

## Senkronizasyon

- **Sync ID**: 0xABCD (sabit)
- **Kontrol Toplamı**: XOR toplama üzerine tüm byte'lar (Bayraklar hariç)

## Gönderim Oranı

- **TX Hızı**: 50 Hz (20 ms aralıklar)
- **Timeout**: 1000 ms (RX'de signal kaybı)

## Bayraklar (Offset 16)

```
Bit 0: Uydu cihazı modu (0=Normal, 1=Return to Home)
Bit 1: Acil durum durduruyor
Bit 2: Pil uyarısı
Bit 3-7: Ayrılmış
```

## Hata Kontrol

- Sync ID uyuşmazlığı → Paket atılır
- Checksum uyuşmazlığı → Paket atılır
- Timeout → Signal kaybı uyarısı

## Genişletme

- 31 byte kalan alan kullanılabilir
- Gelecekteki özellikler için ayrılmış
