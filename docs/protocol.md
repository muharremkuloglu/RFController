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
