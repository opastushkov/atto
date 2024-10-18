#pragma once

#include <atomic>
#include "../Common/Message.h"

struct Node
{
    int                 key;
    Message*            value;
    std::atomic<Node*>  next;

    Node(int k, Message* v) : key(k), value(v), next(nullptr) {}
};

class LockFreeHashTable
{
private:
    std::atomic<Node*>* table;
    std::atomic<size_t> capacity;
    std::atomic<size_t> size;
    std::atomic<bool>   resizing;
    std::atomic<bool>   finishing;
    std::atomic<Node*>* newTable;
    std::atomic<size_t> resizeProgress;
    std::atomic<size_t> resizeThreadCount;
    std::atomic<size_t> insertThreadCount;
    std::atomic<bool>   newTableInitialized;
    const double        loadFactor;

public:
    LockFreeHashTable(uint32_t initCapacity = 10, double initLoadFactor = 0.75);
    ~LockFreeHashTable();

    size_t  hash(int key);
    bool    startResize();
    bool    finishResize();
    void    helpResize();
    void    insert(int key, Message* value);
    bool    find(int key);
};