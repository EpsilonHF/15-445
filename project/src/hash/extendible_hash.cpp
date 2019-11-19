#include <list>

#include "hash/extendible_hash.h"
#include "page/page.h"

namespace cmudb {

/*
 * constructor
 * array_size: fixed array size for each bucket
 */
template <typename K, typename V>
ExtendibleHash<K, V>::ExtendibleHash(size_t size)
    : globalDepth(0), bucketSize(size)
{
    bucketDict.push_back(std::make_shared<Bucket>(0));
}

/*
 * helper function to calculate the hashing address of input key
 */
template <typename K, typename V>
size_t ExtendibleHash<K, V>::HashKey(const K &key) {
    return std::hash<K>()(key);
}

/*
 * helper function to return global depth of hash table
 * NOTE: you must implement this function in order to pass test
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetGlobalDepth() const {
    return globalDepth;
}

/*
 * helper function to return local depth of one specific bucket
 * NOTE: you must implement this function in order to pass test
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetLocalDepth(int bucket_id) const {
    if (bucketDict[bucket_id]) {
        return bucketDict[bucket_id]->localDepth;
    }
    return -1;
}

/*
 * helper function to return current number of bucket in hash table
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetNumBuckets() const {
    return static_cast<int>(bucketDict.size());
}

/*
 * lookup function to find value associate with input key
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Find(const K &key, V &value) {
    std::lock_guard<std::mutex>lock(latch);
    size_t offset = HashKey(key) & ((1 << globalDepth) - 1);

    if (bucketDict[offset]) {
        if (bucketDict[offset]->items.find(key) != bucketDict[offset]->items.end()) {
            value = bucketDict[offset]->items[key];
            return true;
        }
    }

    return false;
}

/*
 * delete <key,value> entry in hash table
 * Shrink & Combination is not required for this project
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Remove(const K &key) {
    std::lock_guard<std::mutex> lock(latch);
    size_t offset = HashKey(key) & ((1 << globalDepth) - 1);
    size_t cnt = 0;

    if (bucketDict[offset]) {
        cnt = bucketDict[offset]->items.erase(key);
    }

    return cnt != 0;
}

/*
 * insert <key,value> entry in hash table
 * Split & Redistribute bucket when there is overflow and if necessary increase
 * global depth
 */
template <typename K, typename V>
void ExtendibleHash<K, V>::Insert(const K &key, const V &value) {
    std::lock_guard<std::mutex> lock(latch);
    size_t offset = HashKey(key) & ((1 << globalDepth) - 1);
    std::shared_ptr<Bucket> target = bucketDict[offset];

    while (target->items.size() == bucketSize) {
        // split bucket
        if (target->localDepth == globalDepth) {
            size_t length = bucketDict.size();
            for (size_t i = 0; i < length; i++) {
                bucketDict.push_back(bucketDict[i]);
            }
            ++globalDepth;
        }
        int mask = (1 << target->localDepth);

        auto a = std::make_shared<Bucket>(target->localDepth + 1);
        auto b = std::make_shared<Bucket>(target->localDepth + 1);

        for (auto item : target->items) {
            size_t newKey = HashKey(item.first);
            if (newKey & mask) {
                b->items.insert(item);
            }
            else {
                a->items.insert(item);
            }
        }

        for (size_t i = 0; i < bucketDict.size(); i++) {
            if (bucketDict[i] == target) {
                if (i & mask) {
                    bucketDict[i] = b;
                }
                else {
                    bucketDict[i] = a;
                }
            }
        }
        offset = HashKey(key) & ((1 << globalDepth) - 1);
        target = bucketDict[offset];
    }

    bucketDict[offset]->items[key] = value;
}

template class ExtendibleHash<page_id_t, Page *>;
template class ExtendibleHash<Page *, std::list<Page *>::iterator>;
// test purpose
template class ExtendibleHash<int, std::string>;
template class ExtendibleHash<int, std::list<int>::iterator>;
template class ExtendibleHash<int, int>;
} // namespace cmudb
