#include "Interpreter.hpp"

#include "../game/GameState.hpp"
#include "../util/DsTrace.hpp"
#include "../util/Lang.hpp"

bool Interpreter::runEvent(const std::string& name) {
    if (busy()) return false;
    const Script* s = lib_.find(name);
    if (s == nullptr) return false;
    stack_.clear();
    pushList(s->head, ("skrip " + name).c_str());
    state_ = Running;
    update();
    return true;
}

void Interpreter::pushList(const Command* head, const char* why) {
    if (head == nullptr) return;
    stack_.push(Frame{head});
    dsOp(std::string("Stack.push(") + why + ") -> kedalaman " + std::to_string(stack_.size()));
}

void Interpreter::update() {
    steps_ = 0;
    while (state_ == Running) {
        Frame f;
        if (!stack_.pop(f)) {           // semua daftar selesai
            state_ = Idle;
            return;
        }
        if (f.pc == nullptr) {
            dsOp("Stack.pop() -> kembali ke kedalaman " + std::to_string(stack_.size()));
            continue;
        }
        const Command* c = f.pc;
        // lanjutan daftar ini dikembalikan ke stack SEBELUM cabang didorong,
        // sehingga cabang dijalankan dulu (LIFO) lalu kembali ke sini
        stack_.push(Frame{c->next});
        execute(c);
        if (++steps_ > 10000) {         // pengaman skrip berulang tanpa henti
            stack_.clear();
            state_ = Idle;
            return;
        }
    }
}

void Interpreter::resume() {
    if (state_ == WaitMessage || state_ == WaitBattle || state_ == WaitTransfer) {
        state_ = Running;
        update();
    }
}

void Interpreter::choose(int i) {
    if (state_ != WaitChoice || choice_ == nullptr) return;
    if (i < 0 || i >= choice_->childCount) i = choice_->childCount - 1;   // batal = opsi terakhir
    const Command* c = choice_;
    choice_ = nullptr;
    state_ = Running;
    pushList(c->child[i], ("pilihan \"" + c->opt[i] + "\"").c_str());
    update();
}

// M3 subprogram: jalankan satu perintah.
void Interpreter::execute(const Command* c) {
    switch (c->op) {
        case CMD_TEXT:
            whoKey_ = c->s1;
            who_ = tr(c->s1);      // teks cerita diterjemahkan saat ditampilkan (kunci = teks Indonesia)
            text_ = tr(c->s2);
            state_ = WaitMessage;
            break;
        case CMD_CHOICE:
            choice_ = c;
            state_ = WaitChoice;
            break;
        case CMD_ITEM: {
            const ItemDef* d = itemDef(c->n1);
            if (d == nullptr) break;
            if (c->n2 >= 0) {
                gs_.addItem(c->n1, c->n2);
                std::string msg = c->n2 > 1 ? trf("Didapat: %s x%d", tr(d->name).c_str(), c->n2)
                                            : trf("Didapat: %s", tr(d->name).c_str());
                notices_.enqueue(msg);
                dsOp("Queue.enqueue(\"" + msg + "\")");
            } else {
                gs_.removeItem(c->n1, -c->n2);
                notices_.enqueue(trf("Diserahkan: %s", tr(d->name).c_str()));
            }
            break;
        }
        case CMD_SUKMA:
            gs_.addSukma(c->n1);
            notices_.enqueue(c->n1 >= 0 ? trf("Sukma +%d", c->n1) : trf("Sukma %d", c->n1));
            break;
        case CMD_FLAG: gs_.setFlag(c->s1, c->n1); break;
        case CMD_IF_FLAG:
            pushList(c->child[gs_.flag(c->s1) == c->n1 ? 0 : 1], ("if " + c->s1).c_str());
            break;
        case CMD_IF_ITEM:
            pushList(c->child[gs_.count(c->n1) >= c->n2 ? 0 : 1], "if barang");
            break;
        case CMD_BATTLE:
            battleTroop_ = c->n1;
            state_ = WaitBattle;
            host_.hostStartBattle(c->n1);
            break;
        case CMD_CALL:
            if (const Script* s = lib_.find(c->s1)) pushList(s->head, ("panggil " + c->s1).c_str());
            break;
        case CMD_HEAL: host_.hostHeal(); break;
        case CMD_UNLOCK: host_.hostUnlock(c->n1, c->n2); break;
        case CMD_TRANSFER:
            state_ = WaitTransfer;
            host_.hostTransfer(c->n1, c->s1);
            break;
        case CMD_NOTICE: notices_.enqueue(tr(c->s1)); break;
        case CMD_SHOP:
            host_.hostOpenShop(c->s1);
            dsOp("Interpreter: buka menu " + c->s1);
            break;
        case CMD_JOIN:
            gs_.joinCompanion();
            dsOp("GameState.joinCompanion() -> party 2 orang");
            break;
        case CMD_END:
            stack_.clear();
            state_ = Idle;
            break;
        default: break;
    }
}

std::string Interpreter::option(int i) const { return tr(choice_->opt[i]); }
