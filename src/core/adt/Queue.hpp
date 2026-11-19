#pragma once
// ============================================================================
// Queue (antrean FIFO) berbasis linked node — dibuat manual, tanpa std::queue.
//
// Dipakai oleh:
//   - M1 Battle Event Queue : BattleSystem meng-enqueue kejadian (damage, rapal,
//                             buyar, ...) lalu GUI/console men-dequeue satu per
//                             satu untuk dianimasikan berurutan.
//   - M3 notifikasi         : Interpreter & Game ("Didapat: ...") ditampilkan
//                             satu per satu tanpa menghentikan permainan.
//   - Graph::bfs (M2)       : antrean vertex saat mencari Tungku terdekat dan
//                             daftar tujuan warp.
//
// Representasi: singly linked list dengan pointer head (depan) dan tail
// (belakang) sehingga enqueue dan dequeue sama-sama O(1).
// ============================================================================

#include <cstddef>

template <typename T>
class Queue {
public:
    struct Node {
        T     value;
        Node* next;
    };

    Queue() : head_(nullptr), tail_(nullptr), size_(0) {}
    ~Queue() { clear(); }

    // Queue berisi pointer milik node; copy dilarang agar tidak terjadi double free.
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    // Tambah elemen di belakang antrean. O(1)
    void enqueue(const T& value) {
        Node* n = new Node{value, nullptr};
        if (tail_ == nullptr) {
            head_ = tail_ = n;
        } else {
            tail_->next = n;
            tail_ = n;
        }
        ++size_;
    }

    // Ambil elemen terdepan. Mengembalikan false jika antrean kosong. O(1)
    bool dequeue(T& out) {
        if (head_ == nullptr) return false;
        Node* n = head_;
        out = n->value;
        head_ = n->next;
        if (head_ == nullptr) tail_ = nullptr;
        delete n;
        --size_;
        return true;
    }

    // Lihat elemen terdepan tanpa menghapusnya.
    bool peek(T& out) const {
        if (head_ == nullptr) return false;
        out = head_->value;
        return true;
    }

    bool        empty() const { return size_ == 0; }
    std::size_t size()  const { return size_; }

    void clear() {
        while (head_ != nullptr) {
            Node* n = head_;
            head_ = head_->next;
            delete n;
        }
        tail_ = nullptr;
        size_ = 0;
    }

    // Traversal depan → belakang (dipakai Inspector untuk menampilkan isi antrean).
    template <typename Fn>
    void forEach(Fn fn) const {
        for (Node* n = head_; n != nullptr; n = n->next) fn(n->value);
    }

    const Node* head() const { return head_; }

private:
    Node*       head_;
    Node*       tail_;
    std::size_t size_;
};
