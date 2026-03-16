/// ============================================================================
/// RF LINK BUDGET HESAPLAMA FONKSİYONLARI
/// ============================================================================
/// 
/// Friis Path Loss Denklemi ile RF haberleşme analizi:
/// Path Loss (dB) = 32.45 + 20×log₁₀(f_MHz) + 20×log₁₀(d_km)
///
/// Link Budget = TX Power - Path Loss - Cable Loss + Antenna Gain
/// SNR = RX Power - RX Sensitivity
/// Fade Margin = SNR - 10dB (practical reserve)

#include <cmath>

/// **calculatePathLoss()** - Serbest uzayda yol kaybı hesapla
/// @param freq_mhz: Frekans (MHz) - 2400 ~ 2500
/// @param distance_km: Mesafe (km)
/// @return Path Loss (dB)
float calculatePathLoss(uint32_t freq_mhz, float distance_km) {
    // Friis Formula: PL = 32.45 + 20*log10(f_MHz) + 20*log10(d_km)
    float path_loss = 32.45;
    path_loss += 20.0 * log10((float)freq_mhz);
    path_loss += 20.0 * log10(distance_km);
    return path_loss;
}

/// **calculateRxPower()** - Alıcı gücünü hesapla
/// @param tx_power_dbm: TX gücü (dBm) - default +20dBm
/// @param path_loss_db: Yol kaybı (dB)
/// @return RX Power (dBm)
float calculateRxPower(float tx_power_dbm, float path_loss_db) {
    // RX Power = TX Power - Path Loss - Cable Loss
    return tx_power_dbm - path_loss_db - CABLE_LOSS;
}

/// **calculateSNR()** - Sinyal-Gürültü Oranı hesapla
/// @param rx_power_dbm: Alıcı gücü (dBm)
/// @param rx_sensitivity_dbm: RX hassasiyeti (dBm) - default -98dBm
/// @return SNR (dB)
float calculateSNR(float rx_power_dbm, float rx_sensitivity_dbm) {
    return rx_power_dbm - rx_sensitivity_dbm;
}

/// **calculateFadeMargin()** - Fade marjını hesapla
/// @param snr_db: SNR (dB)
/// @return Fade Margin (dB) - negatif = fail risk!
float calculateFadeMargin(float snr_db) {
    return snr_db - FADE_MARGIN;  // 10dB reserve
}

/// **printLinkBudget()** - Link bütçesini seri porta yazdır
/// @param freq_mhz: Frekans (MHz)
/// @param distance_km: Mesafe (km)
void printLinkBudget(uint32_t freq_mhz, float distance_km) {
    if (freq_mhz < FREQUENCY_MIN || freq_mhz > FREQUENCY_MAX) {
        Serial.printf("❌ HATA: Frekans aralığı dışı! (%d MHz)\n", freq_mhz);
        return;
    }
    
    float path_loss = calculatePathLoss(freq_mhz, distance_km);
    float rx_power = calculateRxPower(TX_POWER, path_loss);
    float snr = calculateSNR(rx_power, RX_SENSITIVITY);
    float fade_margin = calculateFadeMargin(snr);
    
    Serial.println("\n╔═══════════════════════════════════════════╗");
    Serial.println("║         RF LINK BUDGET ANALİZİ           ║");
    Serial.println("╚═══════════════════════════════════════════╝");
    Serial.printf("📡 Frekans:         %d MHz (2.4 → 2.5 GHz)\n", freq_mhz);
    Serial.printf("📏 Mesafe:          %.2f km (%.0f meter)\n", distance_km, distance_km * 1000);
    Serial.printf("📊 Path Loss:       %.2f dB\n", path_loss);
    Serial.printf("📥 RX Power:        %.2f dBm\n", rx_power);
    Serial.printf("📈 SNR:             %.2f dB\n", snr);
    Serial.printf("🛡️ Fade Margin:     %.2f dB (reserve)\n", fade_margin);
    
    Serial.println("\n┌─ Durum Değerlendirmesi ─────────────────┐");
    if (fade_margin > 10) {
        Serial.println("│ ✅ EXCELLENT - Haberleşme çok iyi      │");
    } else if (fade_margin > 5) {
        Serial.println("│ ✅ GOOD - Haberleşme iyi               │");
    } else if (fade_margin > 0) {
        Serial.println("│ ⚠️ FAIR - Obstrüksiyon riski var       │");
    } else if (fade_margin > -5) {
        Serial.println("│ ❌ POOR - Haberleşme zayıf, hata riski │");
    } else {
        Serial.println("│ ❌ FAIL - Haberleşme mümkün değil!     │");
    }
    Serial.println("└─────────────────────────────────────────┘");
    Serial.println();
}

/// **setFrequency()** - Frekansı güvenli şekilde ayarla
/// @param freq_mhz: Frekans (MHz) - 2400~2500 aralığında
/// @return true if valid, false if out of range
bool setFrequency(uint32_t freq_mhz) {
    // Güvenlik kontrolü
    if (freq_mhz < FREQUENCY_MIN || freq_mhz > FREQUENCY_MAX) {
        Serial.printf("❌ HATA: Frekans '%d MHz' aralığı dışı!\n", freq_mhz);
        Serial.printf("   Geçerli aralık: %d - %d MHz\n", FREQUENCY_MIN, FREQUENCY_MAX);
        return false;
    }
    
    Serial.printf("✅ Frekans ayarlanıyor: %d MHz\n", freq_mhz);
    
    // SX1280 modülüne set et
    sx1280_set_frequency(freq_mhz);
    
    // Debug: Link budget göster
    printLinkBudget(freq_mhz, 0.5);  // 500m için örnek
    
    return true;
}

/// **setTxPower()** - TX gücünü güvenli şekilde ayarla
/// @param power_dbm: TX gücü (dBm) - 0~20 aralığında
/// @return true if valid, false if out of range
bool setTxPower(uint8_t power_dbm) {
    if (power_dbm < TX_POWER_MIN || power_dbm > TX_POWER_MAX) {
        Serial.printf("❌ HATA: TX gücü '%d dBm' aralığı dışı!\n", power_dbm);
        Serial.printf("   Geçerli aralık: %d - %d dBm\n", TX_POWER_MIN, TX_POWER_MAX);
        return false;
    }
    
    Serial.printf("✅ TX gücü ayarlanıyor: %d dBm (%d mW)\n", 
                  power_dbm, (int)pow(10.0, power_dbm / 10.0));
    
    // SX1280 modülüne set et
    // sx1280_set_tx_power(power_dbm);
    
    return true;
}

/// **diagnosticsLinkBudget()** - Tam link bütçesi tanı (seri command ile)
/// Frekans değiştir: AT+FREQ=2410
/// TX gücü değiştir: AT+TXPOWER=15
void diagnosticsLinkBudget() {
    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║  RF LINK BUDGET TANI TABLOSU               ║");
    Serial.println("╠════════════════════════════════════════════╣");
    
    // Farklı mesafelerde analiz (2.4 GHz referans)
    uint32_t test_distances[] = {100, 250, 500, 1000, 2000};  // meter
    
    for (int i = 0; i < 5; i++) {
        float dist_km = test_distances[i] / 1000.0;
        float pl = calculatePathLoss(2400, dist_km);
        float rx = calculateRxPower(20, pl);
        float snr = calculateSNR(rx, -98);
        float margin = calculateFadeMargin(snr);
        
        Serial.printf("Mesafe: %4d m | PL: %6.2f dB | RX: %7.2f dBm | SNR: %6.2f dB | Margin: %+6.2f dB",
                     test_distances[i], pl, rx, snr, margin);
        
        if (margin > 10) Serial.print(" ✅\n");
        else if (margin > 5) Serial.print(" ✅\n");
        else if (margin > 0) Serial.print(" ⚠️\n");
        else Serial.print(" ❌\n");
    }
    Serial.println("╚════════════════════════════════════════════╝\n");
}
