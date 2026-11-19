#include "Script.hpp"

#include "../data/GameData.hpp"
#include "../util/Json.hpp"

void freeCommands(Command* head) {
    while (head != nullptr) {
        Command* n = head->next;
        for (int i = 0; i < head->childCount; ++i) freeCommands(head->child[i]);
        delete head;
        head = n;
    }
}

EventLibrary::~EventLibrary() {
    while (head_ != nullptr) {
        Script* n = head_->next;
        freeCommands(head_->head);
        delete head_;
        head_ = n;
    }
}

static int opFromName(const std::string& s) {
    static const char* names[CMD_COUNT] = {"text", "choice", "item", "sukma", "flag", "if_flag", "if_item",
                                           "battle", "call", "heal", "unlock", "transfer", "notice", "end"};
    for (int i = 0; i < CMD_COUNT; ++i)
        if (s == names[i]) return i;
    return -1;
}

static int itemFrom(const JsonValue* c) {
    const JsonValue* v = c->get("item");
    if (v == nullptr) return 0;
    if (v->type() == JsonValue::String) return itemIdByKey(v->asString().c_str());
    return v->asInt();
}

static Command* parseList(const JsonValue* arr, std::string* error);

static Command* parseOne(const JsonValue* c, std::string* error) {
    int op = opFromName(c->getString("cmd"));
    if (op < 0) {
        if (error) *error = "perintah event tidak dikenal: " + c->getString("cmd");
        return nullptr;
    }
    Command* cmd = new Command();
    cmd->op = op;
    switch (op) {
        case CMD_TEXT:
            cmd->s1 = c->getString("who");
            cmd->s2 = c->getString("text");
            break;
        case CMD_CHOICE: {
            const JsonValue* opts = c->get("options");
            const JsonValue* br = c->get("branches");
            int i = 0;
            for (const JsonValue* o = opts ? opts->first() : nullptr; o && i < Command::MAX_CHILD; o = o->next(), ++i)
                cmd->opt[i] = o->asString();
            cmd->childCount = i;
            int k = 0;
            for (const JsonValue* b = br ? br->first() : nullptr; b && k < i; b = b->next(), ++k)
                cmd->child[k] = parseList(b, error);
            break;
        }
        case CMD_ITEM:
            cmd->n1 = itemFrom(c);
            cmd->n2 = c->getInt("n", 1);
            break;
        case CMD_SUKMA: cmd->n1 = c->getInt("n"); break;
        case CMD_FLAG:
            cmd->s1 = c->getString("name");
            cmd->n1 = c->getInt("value", 1);
            break;
        case CMD_IF_FLAG:
        case CMD_IF_ITEM:
            if (op == CMD_IF_FLAG) {
                cmd->s1 = c->getString("name");
                cmd->n1 = c->getInt("value", 1);
            } else {
                cmd->n1 = itemFrom(c);
                cmd->n2 = c->getInt("n", 1);
            }
            cmd->childCount = 2;
            cmd->child[0] = parseList(c->get("then"), error);
            cmd->child[1] = parseList(c->get("else"), error);
            break;
        case CMD_BATTLE: cmd->n1 = c->getInt("troop"); break;
        case CMD_CALL: cmd->s1 = c->getString("event"); break;
        case CMD_UNLOCK:
            cmd->n1 = c->getInt("a");
            cmd->n2 = c->getInt("b");
            break;
        case CMD_TRANSFER:
            cmd->n1 = c->getInt("map");
            cmd->s1 = c->getString("spawn");
            break;
        case CMD_NOTICE: cmd->s1 = c->getString("text"); break;
        default: break;
    }
    return cmd;
}

static Command* parseList(const JsonValue* arr, std::string* error) {
    if (arr == nullptr || !arr->isArray()) return nullptr;
    Command* head = nullptr;
    Command* tail = nullptr;
    for (const JsonValue* c = arr->first(); c != nullptr; c = c->next()) {
        Command* cmd = parseOne(c, error);
        if (cmd == nullptr) continue;
        if (tail == nullptr) head = cmd; else tail->next = cmd;
        tail = cmd;
    }
    return head;
}

bool EventLibrary::loadRoot(const JsonValue* root, std::string* error) {
    if (root == nullptr || !root->isObject()) {
        if (error && error->empty()) *error = "berkas event harus berupa object";
        return false;
    }
    for (const JsonValue* e = root->first(); e != nullptr; e = e->next()) {
        Script* s = new Script();
        s->name = e->key();
        s->head = parseList(e, error);
        s->next = head_;
        head_ = s;
        ++count_;
    }
    return true;
}

bool EventLibrary::loadFile(const std::string& path, std::string* error) {
    JsonValue* root = JsonValue::parseFile(path, error);
    bool ok = loadRoot(root, error);
    delete root;
    return ok;
}

bool EventLibrary::loadText(const std::string& json, std::string* error) {
    JsonValue* root = JsonValue::parse(json, error);
    bool ok = loadRoot(root, error);
    delete root;
    return ok;
}

const Script* EventLibrary::find(const std::string& name) const {
    for (const Script* s = head_; s != nullptr; s = s->next)
        if (s->name == name) return s;
    return nullptr;
}
