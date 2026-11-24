#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <utility>

using std::cout;
using std::endl;

class Node {
public:
    int key;
    int value;
    Node *next;

    Node(int k, int v) : key(k), value(v), next(nullptr) {
    }
};
// *this обращение к этому
// создать class LinkedList и рефакторить код под него чтобы каждя buckets[i] содержала LinkedList(плохая идея)

class HashTable {
    Node **buckets;
    size_t bucket_count;
    // #upd переименовал count в element_count, иначе конфликт с методом count(int)
    size_t element_count;

    // #upd find ищет узел по ключу, если нет то nullptr, его юзают contains count operator[] и insert
    Node *find(int key) const {
        // #upd после move bucket_count = 0, а на 0 делить нельзя, значит ключа точно нет
        if (bucket_count == 0) {
            return nullptr;
        }
        auto index = hash(key) % bucket_count;
        Node *current = buckets[index];
        while (current != nullptr) {
            if (current->key == key) {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }

public:
    // итератор идет по бакетам слева направо, внутри бакета по списку
    // end() это node == nullptr и bucket == bucket_count
    class iterator {
    private:
        HashTable *table;
        size_t bucket;
        Node *node;

    public:
        // эти typedef нужны std::reverse_iterator (для rbegin/rend)
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = int;
        using difference_type = std::ptrdiff_t;
        using pointer = int *;
        using reference = int &;

        iterator() : table(nullptr), bucket(0), node(nullptr) {
        }

        iterator(HashTable *table, size_t bucket, Node *node) : table(table), bucket(bucket), node(node) {
        }

        iterator(const iterator &) = default;

        iterator &operator=(const iterator &) = default;

        // следующий узел в списке, если список кончился то первый узел следующего непустого бакета
        iterator &operator++() {
            node = node->next;
            while (node == nullptr && ++bucket < table->bucket_count) {
                node = table->buckets[bucket];
            }
            return *this;
        }

        iterator operator++(int) {
            iterator old = *this;
            ++*this;
            return old;
        }

        // список односвязный, поэтому предыдущий ищем с головы бакета,
        // а если мы голова (или end) то идем в последний узел предыдущего непустого бакета
        iterator &operator--() {
            if (node != nullptr && table->buckets[bucket] != node) {
                Node *prev = table->buckets[bucket];
                while (prev->next != node) {
                    prev = prev->next;
                }
                node = prev;
                return *this;
            }
            do {
                --bucket;
            } while (table->buckets[bucket] == nullptr);
            node = table->buckets[bucket];
            while (node->next != nullptr) {
                node = node->next;
            }
            return *this;
        }

        iterator operator--(int) {
            iterator old = *this;
            --*this;
            return old;
        }

        // как в unordered_map: значение менять можно, ключ нельзя, поэтому * дает value, а ключ через key()
        int &operator*() {
            return node->value;
        }

        int key() const {
            return node->key;
        }

        friend bool operator==(const iterator &a, const iterator &b) {
            return a.node == b.node;
        }

        friend bool operator!=(const iterator &a, const iterator &b) {
            return !(a == b);
        }
    };

    iterator begin() {
        for (size_t i = 0; i < bucket_count; i++) {
            if (buckets[i] != nullptr) {
                return iterator(this, i, buckets[i]);
            }
        }
        return end();
    }

    iterator end() {
        return iterator(this, bucket_count, nullptr);
    }

    // shortcut: const версии отдают обычный iterator, через него можно поменять value у const таблицы; если нужно честно то писать отдельный const_iterator
    iterator begin() const {
        return const_cast<HashTable *>(this)->begin();
    }

    iterator end() const {
        return const_cast<HashTable *>(this)->end();
    }

    auto rbegin() {
        return std::reverse_iterator<iterator>(end());
    }

    auto rend() {
        return std::reverse_iterator<iterator>(begin());
    }

    HashTable(std::initializer_list<std::pair<int, int> > list) : HashTable() {
        for (const auto &p: list) {
            insert(p.first, p.second);
        }
    }

    HashTable(iterator first, iterator last) : HashTable() {
        for (; first != last; ++first) {
            insert(first.key(), *first);
        }
    }

    // через tmp + swap, чтобы t.assign(t.begin(), t.end()) не стер сам себя до копирования
    void assign(iterator first, iterator last) {
        HashTable tmp(first, last);
        swap(tmp);
    }

    HashTable() {
        bucket_count = 10;
        element_count = 0;
        buckets = new Node *[bucket_count]();
    }

    // #upd если передали 0 бакетов (например rehash(0)) то делаем 1, иначе потом % 0 и падение
    HashTable(size_t bucket_count) : bucket_count(bucket_count == 0 ? 1 : bucket_count) {
        element_count = 0;
        buckets = new Node *[this->bucket_count]();
    }
    // ## переделал под итераторы
    // конструктор копирования - создаем столько же бакетов и вставляем все пары other через итератор
    HashTable(const HashTable &other) : HashTable(other.bucket_count) {
        for (auto it = other.begin(); it != other.end(); ++it) {
            insert(it.key(), *it);
        }
    }

    /* старая версия
    // конструктор копирования - в новый бакет идут значения старого бакета
    HashTable(const HashTable &other) : bucket_count(other.bucket_count), element_count(other.element_count) {
        buckets = new Node *[other.bucket_count]();
        for (size_t i = 0; i < bucket_count; i++) {
            Node *current_bucket = other.buckets[i];
            Node *tail = nullptr;
            Node *next_bucket;
            while (current_bucket != nullptr) {
                next_bucket = current_bucket->next;
                Node *new_node = new Node(current_bucket->key, current_bucket->value);
                if (tail == nullptr) {
                    buckets[i] = new_node;
                } else {
                    tail->next = new_node;
                }
                tail = new_node;
                current_bucket = next_bucket;
            }
        }
    }
    */

    // move constructor
    HashTable(HashTable &&other) noexcept : buckets(other.buckets), bucket_count(other.bucket_count), element_count(other.element_count) {
        other.buckets = nullptr;
        other.bucket_count = 0;
        other.element_count = 0;
    }
// =
    //operator copy
    // для operator= можно использовать hashTable tmp(other); tmp.swap(*this); но надо написать свап (это и сделано)
    HashTable &operator=(const HashTable &other) {
        if (this == &other) {
            return *this;
        }
        HashTable tmp(other);
        tmp.swap(*this);
        return *this;
    }
    //operator move
    //
    //
    // #upd раньше старые buckets просто терялись и была утечка, теперь как в copy:
    // забираем other в tmp, меняемся с ним, и tmp в dtor удаляет наши старые данные
    // аналог конструктора мува
    HashTable &operator=(HashTable &&other) noexcept {
        if (this == &other) {
            return *this;
        }
        HashTable tmp(std::move(other));
        tmp.swap(*this);
        return *this;
    }

    // ## переделал под итераторы
    // #upd таблицы равны если у них одинаковые пары ключ-значение, порядок не важен
    // проходим итератором по a и ищем такой же ключ в b, если размеры равны и все нашлись то таблицы одинаковые
    friend bool operator==(const HashTable &a, const HashTable &b) {
        if (a.element_count != b.element_count) {
            return false;
        }
        for (auto it = a.begin(); it != a.end(); ++it) {
            Node *other = b.find(it.key());
            if (other == nullptr || other->value != *it) {
                return false;
            }
        }
        return true;
    }

    /* старая версия
    // #upd таблицы равны если у них одинаковые пары ключ-значение, порядок не важен
    // проходим по всем узлам a и ищем такой же ключ в b, если размеры равны и все нашлись то таблицы одинаковые
    friend bool operator==(const HashTable &a, const HashTable &b) {
        if (a.element_count != b.element_count) {
            return false;
        }
        for (size_t i = 0; i < a.bucket_count; i++) {
            for (Node *current = a.buckets[i]; current != nullptr; current = current->next) {
                Node *other = b.find(current->key);
                if (other == nullptr || other->value != current->value) {
                    return false;
                }
            }
        }
        return true;
    }
    */

    // #upd != это просто не ==
    friend bool operator!=(const HashTable &a, const HashTable &b) {
        return !(a == b);
    }

    // #upd в unordered_map нет < > потому что там нет порядка:
    // меньше та таблица в которой меньше элементов
    friend bool operator<(const HashTable &a, const HashTable &b) {
        return a.element_count < b.element_count;
    }

    // #upd a > b это то же самое что b < a
    friend bool operator>(const HashTable &a, const HashTable &b) {
        return b < a;
    }

    // #upd a <= b это когда a < b или они равны
    // (если размер одинаковый но элементы разные то не <, не > и не ==, поэтому не через !(a > b))
    friend bool operator<=(const HashTable &a, const HashTable &b) {
        return a < b || a == b;
    }

    // #upd a >= b это когда a > b или они равны
    friend bool operator>=(const HashTable &a, const HashTable &b) {
        return a > b || a == b;
    }

    // #upd как в unordered_map: если ключа нет то вставляем (key, 0) и возвращаем ссылку на value,
    // поэтому можно писать table[5] = 10
    int &operator[](int key) {
        Node *node = find(key);
        if (node == nullptr) {
            insert(key, 0);
            node = find(key);
        }
        return node->value;
    }

    // ## переделал под итераторы
    // #upd выводит таблицу в виде {key: value, key: value}
    friend std::ostream &operator<<(std::ostream &out, const HashTable &table) {
        out << "{";
        for (auto it = table.begin(); it != table.end(); ++it) {
            if (it != table.begin()) {
                out << ", ";
            }
            out << it.key() << ": " << *it;
        }
        out << "}";
        return out;
    }

    /* старая версия
    // #upd выводит таблицу в виде {key: value, key: value}
    friend std::ostream &operator<<(std::ostream &out, const HashTable &table) {
        out << "{";
        bool first = true;
        for (size_t i = 0; i < table.bucket_count; i++) {
            for (Node *current = table.buckets[i]; current != nullptr; current = current->next) {
                if (!first) {
                    out << ", ";
                }
                out << current->key << ": " << current->value;
                first = false;
            }
        }
        out << "}";
        return out;
    }
    */

    // #upd читает сначала кол-во пар n, потом n раз key value, старое содержимое таблицы стирается
    friend std::istream &operator>>(std::istream &in, HashTable &table) {
        size_t n;
        if (!(in >> n)) {
            return in;
        }
        table.clear();
        for (size_t i = 0; i < n; i++) {
            int key, value;
            if (!(in >> key >> value)) {
                break;
            }
            table.insert(key, value);
        }
        return in;
    }

    // #upd true если такой ключ есть в таблице
    bool contains(int key) const {
        return find(key) != nullptr;
    }

    // #upd сколько элементов с таким ключом, ключи не повторяются поэтому только 0 или 1
    size_t count(int key) const {
        return contains(key) ? 1 : 0;
    }

    //dtor
    ~HashTable() {
        for (size_t i = 0; i < bucket_count; i++) {
            Node *current_bucket = buckets[i];
            Node *next_bucket;
            while (current_bucket != nullptr) {
                next_bucket = current_bucket->next;
                delete(current_bucket);
                current_bucket = next_bucket;
            }
        }
        delete[] buckets;
        bucket_count = 0;
        element_count = 0;
    }
        // нужно добавить условие при котором увеличится таблица
    // #upd если ключ уже есть то ничего не делаем как в unordered_map, раньше добавлялся дубликат
    void insert(int key, int value) {
        if (find(key) != nullptr) {
            return;
        }
        // #upd если таблицу перемещали то бакетов нет, создаем заново 10 как в обычном конструкторе
        if (bucket_count == 0) {
            rehash(10);
        }
        Node *new_node = new Node(key, value);
        auto index = hash(key) % bucket_count;
        new_node->next = buckets[index];
        buckets[index] = new_node;
        element_count++;
    }

    // #upd было int и key * 33 переполнялось на больших ключах (это UB), а size_t просто переполняется по кругу,
    // и отрицательные ключи тоже нормально становятся индексом
    size_t hash(int key) const {
        return static_cast<size_t>(key) * 33;
    }

    int at(int key) {
        // #upd после move бакетов нет, сразу кидаем исключение вместо деления на 0
        if (bucket_count == 0) {
            throw std::out_of_range("Key not found");
        }
        auto index = hash(key) % bucket_count;
        Node *current = buckets[index];
        while (current != nullptr) {
            if (current->key == key) {
                return current->value;
            }
            current = current->next;
        }
        throw std::out_of_range("Key not found");
    }

    void clear() {
        for (size_t i = 0; i < bucket_count; i++) {
            Node *current_bucket = buckets[i];
            Node *next_bucket;
            while (current_bucket != nullptr) {
                next_bucket = current_bucket->next;
                delete(current_bucket);
                current_bucket = next_bucket;
            }
        }
        element_count = 0;
        delete[] buckets;
        buckets = new Node *[bucket_count]();
    };

    // ## переделал под итераторы
    //(resize переимееновал в rehash аналог cpp reference)
    //изменить размер бакетов + нужно бцдет перехешировать все
    void rehash(size_t new_bucket_count) {
        HashTable newTable(new_bucket_count);
        for (auto it = begin(); it != end(); ++it) {
            newTable.insert(it.key(), *it);
        }
        swap(newTable); //уничтожится dtor
    }

    /* старая версия
    //(resize переимееновал в rehash аналог cpp reference)
    //изменить размер бакетов + нужно бцдет перехешировать все
    void rehash(size_t new_bucket_count) {
        HashTable newTable(new_bucket_count);
        for (size_t i = 0; i < this->bucket_count; i++) {
            Node *current_bucket = buckets[i];
            while (current_bucket != nullptr) {
                newTable.insert(current_bucket->key, current_bucket-> value);
                current_bucket = current_bucket->next;
            }
        }
        swap(newTable); //уничтожится dtor
    };
    */

    size_t size() {
        return this->element_count;
    }

    bool empty() {
        return element_count == 0;
    };

    void swap(HashTable &other) {
        // должен поменять содержимое таблиц местами
        std::swap(this->bucket_count, other.bucket_count);
        std::swap(this->element_count, other.element_count);
        std::swap(this->buckets, other.buckets);
    };

    //static bool is_equal(const HashTable &a, const HashTable &b);
    // не нужен для хеш таблие по  cppreference
    // должен вернуть если True если таблицы одинаковые и важен ли порядок и кол-во бакетов?

    // void assign(size_t, int); что должен делать (нет в cppreference)
};

int main() {
    HashTable table1;
    table1.insert(5, 20);
    table1.insert(62, 65);
    table1.insert(67, 34);
    HashTable table2(table1);
    HashTable table3;
    table3 = table2;
    table2.swap(table3);
    cout << table1.at(5) << endl;
    cout << table1.at(62)<< endl;
    cout << table1.at(67)<< endl;

    // итераторы
    HashTable t{{1, 10}, {2, 20}, {12, 120}, {3, 30}};
    cout << t << endl;
    for (auto it = t.begin(); it != t.end(); ++it) {
        *it += 1; // value через итератор менять можно
    }
    cout << t << endl;
    for (auto rit = t.rbegin(); rit != t.rend(); ++rit) {
        cout << *rit << " ";
    }
    cout << endl;
    HashTable copy(t.begin(), t.end());
    cout << (copy == t) << endl;
    copy.assign(table1.begin(), table1.end());
    cout << copy << endl;
    return 0;
}
