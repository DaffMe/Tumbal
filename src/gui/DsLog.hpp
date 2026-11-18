#pragma once
// Catatan operasi struktur data terbaru (dari event DsOp) untuk Inspector.
// Hanya buffer tampilan di GUI — bukan bagian dari ADT yang dinilai.

#include <deque>
#include <string>

struct DsLog {
    std::deque<std::string> lines;
    std::size_t             max = 16;
    int                     total = 0;

    void push(const std::string& s) {
        lines.push_back(s);
        ++total;
        while (lines.size() > max) lines.pop_front();
    }
};
