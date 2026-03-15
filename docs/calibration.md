# Kalibrasyonu Rehberi

## Potansiyometre Kalibrasyonu

### Adım 1: Minimum Değer Ölçümü
1. Tüm potansiyometreleri 0 pozisyonuna ayarla
2. Serial monitörü aç (115200 baud)
3. Okunan ADC değerlerini not et
4. Bu değeri `ADC_MIN` yerine `config.h` yapıştır

### Adım 2: Maksimum Değer Ölçümü
1. Tüm potansiyometreleri 100% pozisyonuna ayarla
2. Okunan maksimum ADC değerini not et
3. Bu değeri `ADC_MAX` yerine `config.h` yapıştır

### Adım 3: Doğru Kurulumunu Kontrol Et
1. Potansiyometreleri hareket ettir
2. Çıkış değerleri 1000-2000 µs aralığında olmalı
3. Çapraz eğrilik varsa, potansiyometre bağlantısını kontrol et

## Radio Kalibrasyonu

### TX Tarafı Testi
1. Sırayla TX'i başlat
2. Serial monitörü açık tut
3. Potansiyometreleri hareket ettir
4. Değerlerin 1000-2000 µs aralığında olduğunu doğrula

### RX Tarafı Testi
1. TX'i başlat ve etkinleştir
2. RX'i başlat
3. Serial monitörü aç
4. RX'in TX'den paket aldığını doğrula
5. Değerler eşleşiyor mu kontrol et

### Aralık Testi
1. TX ve RX'i farklı mesafelerde test et
2. Başlangıçta 1-2 metre mesafede başla
3. Mesafeyi kademeli olarak artır
4. Sinyal kaybı olmadığını kontrol et

## Dış Ölçüm (Spektrum Analizörü)

1. Spektrum analizörünü 2.4 GHz'e ayarla
2. Sinyal gücünü ve bant genişliğini ölç
3. İçinde uygun sınırlar olduğunu doğrula

## Sorun Giderme

### Paket Kaybı
- SPI hızını düşür
- Devre bağlantılarını kontrol et
- Gücü kontrol et

### Yanlış Değerler
- Potansiyometre bağlantılarını kontrol et
- ADC MIN/MAX doğru mu? daha

### Zayıf Sinyal
- Anten bağlantılarını kontrol et
- TX gücünü artır
- Başka 2.4 GHz cihazlardan uzak olun
