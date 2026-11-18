#pragma once
// Status effect yang menempel pada unit (buff/debuff/DoT).

enum class StatusType {
    Burn,        // damage tiap awal giliran pemilik
    DefUp,       // DEF naik sebesar value (persen, 0.4 = +40%)
    SpdUp,       // SPD naik sebesar value (persen) → Action Value berubah
    Taunt,       // musuh wajib menyerang unit ini
    Vulnerable   // damage yang diterima naik sebesar value (persen)
};

struct Status {
    StatusType type  = StatusType::Burn;
    int        turns = 0;     // sisa durasi (berkurang di akhir giliran pemilik)
    float      value = 0.0f;  // besaran efek (Burn: damage absolut per tick)
};

inline const char* statusName(StatusType t) {
    switch (t) {
        case StatusType::Burn:       return "Burn";
        case StatusType::DefUp:      return "DEF+";
        case StatusType::SpdUp:      return "SPD+";
        case StatusType::Taunt:      return "Taunt";
        case StatusType::Vulnerable: return "Vuln";
    }
    return "?";
}
