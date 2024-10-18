#include "HashTable.h"
#include <thread>

LockFreeHashTable::LockFreeHashTable(uint32_t initCapacity, double initLoadFactor)
    : capacity(initCapacity)
    , size(0)
    , resizing(false)
    , resizeProgress(0)
    , newTable(nullptr)
    , resizeThreadCount(0)
    , loadFactor(initLoadFactor)
{
    table = new std::atomic<Node*>[capacity.load()];
    for(size_t i = 0; i < capacity.load(); i++) { table[i] = nullptr; }
}

LockFreeHashTable::~LockFreeHashTable()
{
    delete[] table;
    if(newTable != nullptr) { delete[] newTable; }
}

size_t LockFreeHashTable::hash(int key)
{
    return key % capacity.load();
}

bool LockFreeHashTable::startResize()
{
    if(resizing.exchange(true)) { return false; }

    size_t  oldCapacity = capacity.load();
    size_t  newCapacity = oldCapacity * 2;

    newTableInitialized.store(false);

    newTable = new std::atomic<Node*>[newCapacity];
    for(size_t i = 0; i < newCapacity; ++i) { newTable[i] = nullptr; }

    newTableInitialized.store(true);
    resizeProgress.store(0);
    resizeThreadCount.store(0);

    return true;
}

bool LockFreeHashTable::finishResize() {
    size_t oldCapacity = capacity.load();
    delete[] table;

    table = newTable;
    capacity.store(oldCapacity * 2);

    newTableInitialized.store(false);
    newTable = nullptr;
    resizing.store(false);
    finishing.store(false);

    return true;
}

void LockFreeHashTable::helpResize()
{
    if(finishing.load())                { return; }

    while(insertThreadCount.load() > 0) { std::this_thread::yield(); }

    resizeThreadCount.fetch_add(1);

    if(finishing.load() || !resizing.load())
    {
        resizeThreadCount.fetch_sub(1);
        return;
    }

    while(!newTableInitialized.load()) { std::this_thread::yield(); }

    size_t  oldCapacity = capacity.load();
    size_t  newCapacity = oldCapacity * 2;
    size_t  bucketToMove;
    while((bucketToMove = resizeProgress.fetch_add(1)) < oldCapacity)
    {
        Node* head = table[bucketToMove].load();
        while(head != nullptr)
        {
            Node*   next        = head->next.load();
            size_t  newIndex    = head->key % newCapacity;
            Node*   newHead     = newTable[newIndex].load();
            do { head->next = newHead; }
            while(!newTable[newIndex].compare_exchange_weak(newHead, head));
            head = next;
        }
    }

    int remainingThreads = resizeThreadCount.fetch_sub(1);
    if(remainingThreads == 1 && !finishing.exchange(true)) { finishResize(); }
}

void LockFreeHashTable::insert(int key, Message* value)
{
    if(resizing.load())     { helpResize(); }

    while(resizing.load())  { std::this_thread::yield(); }

    insertThreadCount.fetch_add(1);

    size_t  index   = hash(key);
    Node*   newNode = new Node(key, value);
    while(true)
    {
        Node* head = table[index].load();
        newNode->next = head;
        if(table[index].compare_exchange_weak(head, newNode))
        {
            size.fetch_add(1);
            insertThreadCount.fetch_sub(1);
            if(size.load() > capacity.load() * loadFactor)
            {
                startResize();
                helpResize();
            }
            break;
        }
    }
}

bool LockFreeHashTable::find(int key) {
    size_t  index   = hash(key);
    Node*   head    = table[index].load();
    while(head)
    {
        if(head->key == key) { return true; }
        head = head->next.load();
    }

    return false;
}