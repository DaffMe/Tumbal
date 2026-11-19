#pragma once
// ============================================================================
// Timeline — Doubly Linked List terurut berdasarkan sisa waktu sampai giliran.
// Struktur data utama Mekanik M1 (battle ATB + antrean giliran).
//
// Setiap node menyimpan satu battler beserta sisa waktunya (field `av`, dalam
// tick ATB). List selalu terurut
// menaik: head = sisa waktu terkecil = battler yang bertindak berikutnya.
//
// Kenapa Doubly Linked List:
//   - Stun/haste/rapal mengubah sisa waktu battler di tengah list → node harus dipindah.
//     Dengan linked list cukup lepas (unlink) lalu sambungkan ulang (relink)
//     pointer; tidak ada pergeseran elemen seperti pada array.
//   - Pointer prev membuat node bisa bergeser MUNDUR (ke arah head) saat
//     di-advance tanpa mengulang traversal dari head.
//   - Unit mati dihapus dari posisi mana pun; summon disisipkan terurut.
//
// T adalah tipe payload (Battler). Timeline hanya menyimpan pointer,
// tidak memiliki (own) objek unitnya.
// ============================================================================

#include <cstddef>

template <typename T>
class Timeline {
public:
    struct Node {
        T*    item;
        float av;     // sisa Action Value sebelum unit ini bertindak
        Node* prev;
        Node* next;
    };

    Timeline() : head_(nullptr), tail_(nullptr), size_(0) {}
    ~Timeline() { clear(); }

    // Deep copy — dipakai untuk snapshot (Stack rewind) dan prediksi giliran.
    Timeline(const Timeline& other) : head_(nullptr), tail_(nullptr), size_(0) {
        copyFrom(other);
    }
    Timeline& operator=(const Timeline& other) {
        if (this != &other) {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    // ---------------------------------------------------------------- insert

    // Sisipkan unit sesuai urutan AV (menaik). Jika AV sama, unit baru
    // ditaruh SETELAH unit yang sudah ada (adil: yang lebih dulu antre, duluan).
    Node* insertSorted(T* item, float av) {
        Node* n = new Node{item, av, nullptr, nullptr};
        linkSorted(n, head_);
        ++size_;
        return n;
    }

    // Sisipkan di paling depan (Interject / extra turn). AV dipaksa ≤ head
    // agar urutan tetap valid.
    Node* insertFront(T* item, float av = 0.0f) {
        if (head_ != nullptr && av > head_->av) av = head_->av;
        Node* n = new Node{item, av, nullptr, head_};
        if (head_ != nullptr) head_->prev = n; else tail_ = n;
        head_ = n;
        ++size_;
        return n;
    }

    // ---------------------------------------------------------------- delete

    // Hapus node milik unit tertentu (mis. unit mati). O(n) untuk mencari.
    bool remove(T* item) {
        Node* n = search(item);
        if (n == nullptr) return false;
        unlink(n);
        delete n;
        --size_;
        return true;
    }

    // Ambil unit terdepan (yang bertindak sekarang). O(1)
    bool popFront(T*& outItem, float& outAv) {
        if (head_ == nullptr) return false;
        Node* n = head_;
        outItem = n->item;
        outAv   = n->av;
        unlink(n);
        delete n;
        --size_;
        return true;
    }

    // ---------------------------------------------------------------- search

    Node* search(T* item) const {
        for (Node* n = head_; n != nullptr; n = n->next)
            if (n->item == item) return n;
        return nullptr;
    }

    bool contains(T* item) const { return search(item) != nullptr; }

    // ---------------------------------------------------------------- update

    // Ubah AV sebuah unit lalu pindahkan node-nya ke posisi yang benar.
    // Inilah operasi inti delay (AV naik → geser ke belakang) dan advance
    // (AV turun → geser ke depan).
    bool updateAV(T* item, float newAv) {
        Node* n = search(item);
        if (n == nullptr) return false;
        if (newAv < 0.0f) newAv = 0.0f;
        n->av = newAv;
        reposition(n);
        return true;
    }

    // Kurangi AV semua node sebesar delta (waktu berjalan). Urutan tidak
    // berubah karena semua dikurangi nilai yang sama. Traversal O(n).
    void subtractAll(float delta) {
        for (Node* n = head_; n != nullptr; n = n->next) {
            n->av -= delta;
            if (n->av < 0.0f) n->av = 0.0f;
        }
    }

    // ------------------------------------------------------------- traversal

    template <typename Fn>
    void forEach(Fn fn) const {
        for (Node* n = head_; n != nullptr; n = n->next) fn(*n);
    }

    template <typename Fn>
    void forEachReverse(Fn fn) const {
        for (Node* n = tail_; n != nullptr; n = n->prev) fn(*n);
    }

    Node*       front() const { return head_; }
    Node*       back()  const { return tail_; }
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

    // Cek invariant: terurut menaik dan pointer prev/next konsisten.
    // Dipakai unit test dan mode debug.
    bool isValid() const {
        std::size_t count = 0;
        Node* prev = nullptr;
        for (Node* n = head_; n != nullptr; n = n->next) {
            if (n->prev != prev) return false;
            if (prev != nullptr && prev->av > n->av) return false;
            prev = n;
            ++count;
        }
        return prev == tail_ && count == size_;
    }

private:
    Node*       head_;
    Node*       tail_;
    std::size_t size_;

    // Lepaskan node dari list tanpa menghapus memorinya.
    void unlink(Node* n) {
        if (n->prev != nullptr) n->prev->next = n->next; else head_ = n->next;
        if (n->next != nullptr) n->next->prev = n->prev; else tail_ = n->prev;
        n->prev = n->next = nullptr;
    }

    // Sambungkan node lepas ke posisi terurut, mulai mencari dari `from`.
    void linkSorted(Node* n, Node* from) {
        Node* cur = from;
        while (cur != nullptr && cur->av <= n->av) cur = cur->next;
        if (cur == nullptr) {               // taruh di ekor
            n->prev = tail_;
            n->next = nullptr;
            if (tail_ != nullptr) tail_->next = n; else head_ = n;
            tail_ = n;
        } else {                            // taruh sebelum cur
            n->next = cur;
            n->prev = cur->prev;
            if (cur->prev != nullptr) cur->prev->next = n; else head_ = n;
            cur->prev = n;
        }
    }

    // Pindahkan node yang AV-nya baru berubah. Arah pencarian memanfaatkan
    // pointer prev/next: maju untuk delay, mundur untuk advance.
    void reposition(Node* n) {
        bool tooBig   = (n->next != nullptr && n->next->av < n->av);
        bool tooSmall = (n->prev != nullptr && n->prev->av > n->av);
        if (!tooBig && !tooSmall) return;   // posisi sudah benar

        if (tooBig) {                       // DELAY: geser ke arah tail
            Node* start = n->next;
            unlink(n);
            linkSorted(n, start);
        } else {                            // ADVANCE: geser ke arah head
            Node* cur = n->prev;
            unlink(n);
            // mundur selama node sebelumnya masih lebih besar (strict >)
            // agar unit yang di-advance tidak menyalip unit ber-AV sama.
            while (cur != nullptr && cur->av > n->av) cur = cur->prev;
            if (cur == nullptr) {           // jadi head
                n->next = head_;
                if (head_ != nullptr) head_->prev = n; else tail_ = n;
                head_ = n;
            } else {                        // taruh setelah cur
                n->prev = cur;
                n->next = cur->next;
                if (cur->next != nullptr) cur->next->prev = n; else tail_ = n;
                cur->next = n;
            }
        }
    }

    void copyFrom(const Timeline& other) {
        for (Node* n = other.head_; n != nullptr; n = n->next) {
            Node* c = new Node{n->item, n->av, tail_, nullptr};
            if (tail_ != nullptr) tail_->next = c; else head_ = c;
            tail_ = c;
            ++size_;
        }
    }
};
