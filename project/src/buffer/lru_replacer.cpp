/**
 * LRU implementation
 */
#include "buffer/lru_replacer.h"
#include "page/page.h"

namespace cmudb {

template <typename T> LRUReplacer<T>::LRUReplacer() {
  head = nullptr;
  tail = nullptr;
}

template <typename T>
LRUReplacer<T>::~LRUReplacer() {
  std::shared_ptr<ListNode> cur = head;
  std::shared_ptr<ListNode> next;
  while (cur) {
    next = head->next;
    cur = next;
  }
}

/*
 * Insert value into LRU
 */
template <typename T>
    void LRUReplacer<T>::Insert(const T &value) {}

/* If LRU is non-empty, pop the head member from LRU to argument "value", and
 * return true. If LRU is empty, return false
 */
template <typename T>
    bool LRUReplacer<T>::Victim(T &value) {
      if (valueSequence.size() == 0) return false;
      
  return false;
}

/*
 * Remove value from LRU. If removal is successful, return true, otherwise
 * return false
 */
template <typename T> bool LRUReplacer<T>::Erase(const T &value) {
  return false;
}

template <typename T> size_t LRUReplacer<T>::Size() { return 0; }

template class LRUReplacer<Page *>;
// test only
template class LRUReplacer<int>;

} // namespace cmudb
