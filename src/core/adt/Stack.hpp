#pragma once
// ============================================================================
// Stack (tumpukan LIFO) berbasis linked node — dibuat manual, tanpa std::stack.
//
// Dipakai oleh:
//   - M3 Interpreter event : Stack<Frame>; masuk cabang pilihan / if / skrip
//                            lain = push, daftar perintah habis = pop (LIFO),
//                            sehingga eksekusi kembali ke baris pemanggil.
//   - Graph::dijkstra      : rekonstruksi jalur (dari target mundur ke start)
//                            di-push lalu di-pop sehingga urutannya start -> target
//                            (dipakai unit test; mekanik memakai BFS).
//
// Representasi: singly linked list, top = node pertama. push/pop O(1).
// ============================================================================

#include <cstddef>

template <typename T>
class Stack {
public:
    struct Node {
        T     value;
        Node* next;   // node di bawahnya
    };

    Stack() : top_(nullptr), size_(0) {}
    ~Stack() { clear(); }

    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    // Taruh elemen di puncak tumpukan. O(1)
    void push(const T& value) {
        top_ = new Node{value, top_};
        ++size_;
    }

    // Ambil elemen puncak. Mengembalikan false jika kosong. O(1)
    bool pop(T& out) {
        if (top_ == nullptr) return false;
        Node* n = top_;
        out = n->value;
        top_ = n->next;
        delete n;
        --size_;
        return true;
    }

    // Lihat elemen puncak tanpa menghapusnya.
    bool peek(T& out) const {
        if (top_ == nullptr) return false;
        out = top_->value;
        return true;
    }

    bool        empty() const { return size_ == 0; }
    std::size_t size()  const { return size_; }

    void clear() {
        while (top_ != nullptr) {
            Node* n = top_;
            top_ = top_->next;
            delete n;
        }
        size_ = 0;
    }

    // Traversal puncak → dasar.
    template <typename Fn>
    void forEach(Fn fn) const {
        for (Node* n = top_; n != nullptr; n = n->next) fn(n->value);
    }

    const Node* top() const { return top_; }

private:
    Node*       top_;
    std::size_t size_;
};
