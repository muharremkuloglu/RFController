/// ============================================================================
/// RF LINK BUDGET HESAPLAMA FONKSİYONLARI (TX KUMANDA TARAFINDA)
/// ============================================================================
/// 
/// TX kumanda istasyonunun RF haberleşme parametreleri
/// Verilen TX gücü ve frekansa göre teorik menzil hesapla

#include <cmath>

/// **calculatePathLoss()** - Serbest uzayda yol kaybı hesapla (aynen RX gibi)
float calculatePathLoss(uint32_t freq_mhz, float distance_km) {
    float path_loss = 32.45;
    path_loss += 20.0 * log10((float)freq_mhz);
    path_loss += 20.0 * log10(distance_km);
    return path_loss;
}

/// **calculateTheoreticalRange()** - Teorik max menzili hesapla
/// @param tx_power_dbm: TX gücü (dBm)
/// @param freq_mhz: Frekans (MHz)
/// @param rx_sensitivity_dbm: RX'in hassasiyeti (dBm)
/// @param fade_margin_db: Güvenlik marjı (dB) - default 10dB
/// @return Maksimum mesafe (meter)
float calculateTheoreticalRange(float tx_power_dbm, uint32_t freq_mhz, 
                               float rx_sensitivity_dbm, float fade_margin_db = 10.0) {
    // Maksimum path loss toleransı
    float max_path_loss = tx_power_dbm - rx_sensitivity_dbm - 3 - fade_margin_db;
    
    // Friis formülünden distance çöz:
    // PL = 32.45 + 20*log10(f) + 20*log10(d)
    // d = 10^((PL - 32.45 - 20*log10(f)) / 20)
    
    float numerator = max_path_loss - 32.45 - 20.0 * log10((float)freq_mhz);
    float distance_km = pow(10.0, numerator / 20.0);
    
    return distance_km * 1000.0;  // meter cinsinden döndür
}

/// **printTxLinkBudget()** - TX haberleşeme parametrelerini göster
void printTxLinkBudget(float tx_power_dbm, uint32_t freq_mhz) {
    float range_no_margin = calculateTheoreticalRange(tx_power_dbm, freq_mhz, -98, 0);
    float range_with_margin = calculateTheoreticalRange(tx_power_dbm, freq_mhz, -98, 10);
    
    Serial.println("\n╔═══════════════════════════════════════════╗");
    Serial.println("║        TX HABERLEŞİME PARÇETRELERİ        ║");
    Serial.println("╚═══════════════════════════════════════════╝");
    Serial.printf("📡 Frekans:         %d MHz\n", freq_mhz);
    Serial.printf("📊 TX Power:        %d dBm (%d mW)\n", 
                  (int)tx_power_dbm, (int)pow(10.0, tx_power_dbm / 10.0));
    Serial.printf("📥 RX Sensitivity:  -98 dBm (SX1280 SF7)\n");
    Serial.printf("🛡️ Fade Margin:     10 dB\n");
    Serial.printf("\n✅ Teorik Menzil (marjinSIZ): %.0f meter (%.1f km)\n", 
                  range_no_margin, range_no_margin / 1000.0);
    Serial.printf("✅ Güvenli Menzil (marjINLı): %.0f meter (%.1f km)\n", 
                  range_with_margin, range_with_margin / 1000.0);
    Serial.printf("⚠️ Önerilir Mesafe: < %.0f meter\n\n", range_with_margin * 0.8);
}

/// **freqToString()** - Frekansı insan tarafından okunur şekle çevir
const char* freqToString(uint32_t freq_mhz) {
    static char buffer[20];
    snprintf(buffer, sizeof(buffer), "%.2f GHz", freq_mhz / 1000.0);
    return buffer;
}

/// **txPowerToMw()** - dBm'den milliwatt'a çevir
float txPowerToMw(float dbm) {
    return pow(10.0, dbm / 10.0);
}

/// **displayLinkQuality()** - RSSI değerinden haberleşme kalitesini göster
const char* displayLinkQuality(int8_t rssi_dbm) {
    if (rssi_dbm >= -70) return "🟢 EXCELLENT";
    else if (rssi_dbm >= -80) return "🟢 GOOD";
    else if (rssi_dbm >= -90) return "🟡 FAIR";
    else if (rssi_dbm >= -100) return "🔴 POOR";
    else return "🔴 CRITICAL";
}
