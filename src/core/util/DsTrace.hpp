#pragma once
// ============================================================================
// DsTrace — catatan operasi struktur data yang sedang berjalan.
//
// Setiap kali mekanik memanggil operasi ADT (mis. Graph.findEdge,
// Timeline.updateAV), mekanik mencatatnya lewat dsOp("..."). Catatan ini
// ditampilkan di console (--console) dan panel Inspector GUI (F1) supaya
// hubungan aksi pemain → operasi struktur data bisa diamati saat demo.
//
// Disimpan di ring buffer berukuran tetap (array biasa), bukan container.
// ============================================================================

#include <cstdio>
#include <string>

class DsTrace {
public:
    static constexpr int CAPACITY = 64;

    static DsTrace& instance() {
        static DsTrace t;
        return t;
    }

    void add(const std::string& line) {
        lines_[head_] = line;
        head_ = (head_ + 1) % CAPACITY;
        if (count_ < CAPACITY) ++count_;
        ++total_;
        if (echo_) std::printf("  [DS] %s\n", line.c_str());
    }

    // i = 0 → catatan terbaru.
    const std::string& recent(int i) const {
        int idx = (head_ - 1 - i + CAPACITY * 2) % CAPACITY;
        return lines_[idx];
    }
    int count() const { return count_; }
    long total() const { return total_; }
    void setEcho(bool on) { echo_ = on; }
    void clear() { head_ = 0; count_ = 0; }

private:
    std::string lines_[CAPACITY];
    int  head_ = 0;
    int  count_ = 0;
    long total_ = 0;
    bool echo_ = false;
};

inline void dsOp(const std::string& line) { DsTrace::instance().add(line); }
