#include <iostream>
#include <cstddef>
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
    size_t count;

public:
    HashTable() {
        bucket_count = 10;
        count = 0;
        buckets = new Node *[bucket_count]();
    }

    HashTable(size_t bucket_count) : bucket_count(bucket_count) {
        count = 0;
        buckets = new Node *[bucket_count]();
    }
    // конструктор копирования
    HashTable(const HashTable &other) : count(other.count), bucket_count(other.bucket_count) {
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

    // move constructor
    HashTable(HashTable &&other) : buckets(other.buckets), bucket_count(other.bucket_count), count(other.count) {
        other.buckets = nullptr;
        other.bucket_count = 0;
        other.count = 0;
    }
// =
    //operator copy
    // для operator= можно использовать hashTable tmp(other); tmp.swap(*this); но надо написать свап
    HashTable &operator=(const HashTable &other) {
        if (this == &other) {
            return *this;
        }
        HashTable tmp(other);
        tmp.swap(*this);
        return *this;
    }
    //operator move
    HashTable &operator=(HashTable &&other) {
        if (this == &other) {
            return *this;
        }
        this->bucket_count = other.bucket_count;
        this->count = other.count;
        this->buckets = other.buckets;

        other.buckets = nullptr;
        other.bucket_count = 0;
        other.count = 0;
        return *this;
    }
    bool operator==(const HashTable &) const {}

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
        count = 0;
    }

    void insert(int key, int value) {
        Node *new_node = new Node(key, value);
        auto index = hash(key) % bucket_count;
        new_node->next = buckets[index];
        buckets[index] = new_node;
        count++;
    }

    int hash(int key) {
        return key * 33;
    }

    int at(int key) {
        // нужно добавить условие при котором увеличится таблица
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
        count = 0;
        delete[] buckets;
        buckets = new Node *[bucket_count]();
    };

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

    size_t size() {
        return this->count;
    }

    bool empty() {
        return count == 0;
    };

    void swap(HashTable &other) {
        // должен поменять содержимое таблиц местами
        std::swap(this->bucket_count, other.bucket_count);
        std::swap(this->count, other.count);
        std::swap(this->buckets, other.buckets);
    };

    //static bool is_equal(const HashTable &a, const HashTable &b);
    // не нужен для хеш таблие по  cppreference
    // должен вернуть если True если таблицы одинаковые и важен ли порядок и кол-во бакетов?

    // void assign(size_t, int); что должен делать (нет в cppreference)
};
// добавил рехеш свап и операторы копи и свап .. вопрос насчет остальных операторов кроме == и
//friend std::ostream &operator<<(std::ostream&, const example&);
//friend std::istream &operator>>(std::istream&, example&);

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
    return 0;
}
