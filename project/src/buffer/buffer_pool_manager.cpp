#include "buffer/buffer_pool_manager.h"

namespace cmudb {

/*
 * BufferPoolManager Constructor
 * When log_manager is nullptr, logging is disabled (for test purpose)
 * WARNING: Do Not Edit This Function
 */
BufferPoolManager::BufferPoolManager(size_t pool_size,
        DiskManager *disk_manager,
        LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager),
      log_manager_(log_manager) {
    // a consecutive memory space for buffer pool
    pages_ = new Page[pool_size_];
    page_table_ = new ExtendibleHash<page_id_t, Page *>(BUCKET_SIZE);
    replacer_ = new LRUReplacer<Page *>;
    free_list_ = new std::list<Page *>;

    // put all the pages into free list
    for (size_t i = 0; i < pool_size_; ++i) {
        free_list_->push_back(&pages_[i]);
    }
}

/*
 * BufferPoolManager Deconstructor
 * WARNING: Do Not Edit This Function
 */
BufferPoolManager::~BufferPoolManager() {
    delete[] pages_;
    delete page_table_;
    delete replacer_;
    delete free_list_;
}

/**
 * 1. search hash table.
 *  1.1 if exist, pin the page and return immediately
 *  1.2 if no exist, find a replacement entry from either free list or lru
 *      replacer. (NOTE: always find from free list first)
 * 2. If the entry chosen for replacement is dirty, write it back to disk.
 * 3. Delete the entry for the old page from the hash table and insert an
 * entry for the new page.
 * 4. Update page metadata, read page content from disk file and return page
 * pointer
 */
Page *BufferPoolManager::FetchPage(page_id_t page_id) {
    std::lock_guard<std::mutex> lock(latch_);

    Page *target = nullptr;
    if (page_id == INVALID_PAGE_ID) {
        return target;
    }

    // search hash table
    if (page_table_->Find(page_id, target)) {
        target->pin_count_++;
        return target;
    }
    else if (!free_list_->empty()) {
        target = *free_list_->begin();
        free_list_->pop_front();
    }
    else {
        if (!replacer_->Victim(target) || target->pin_count_ > 0) {
            return nullptr;
        }
    }

    // if dirty, write back
    assert(target->pin_count_ == 0);
    if (target->is_dirty_) {
        disk_manager_->WritePage(target->GetPageId(), target->GetData());
        target->is_dirty_ = false;
    }

    // delete entry for old page and insert new page
    page_table_->Remove(target->GetPageId());
    page_table_->Insert(page_id, target);
    target->page_id_ = page_id;
    target->pin_count_ = 1;

    // update page
    target->ResetMemory();
    disk_manager_->ReadPage(page_id, target->GetData());

    return target;
}

/*
 * Implementation of unpin page
 * if pin_count>0, decrement it and if it becomes zero, put it back to
 * replacer if pin_count<=0 before this call, return false. is_dirty: set the
 * dirty flag of this page
 */
bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {
    std::lock_guard<std::mutex> lock(latch_);

    Page *target = nullptr;
    if (!page_table_->Find(page_id, target) || target->pin_count_ <= 0) {
        return false;
    }

    target->pin_count_--;
    if (target->pin_count_ == 0) {
        replacer_->Insert(target);
    }

    if (is_dirty) {
        target->is_dirty_ = true;
    }

    return true;
}

/*
 * Used to flush a particular page of the buffer pool to disk. Should call the
 * write_page method of the disk manager
 * if page is not found in page table, return false
 * NOTE: make sure page_id != INVALID_PAGE_ID
 */
bool BufferPoolManager::FlushPage(page_id_t page_id) {
    std::lock_guard<std::mutex> lock(latch_);

    Page *target = nullptr;
    if (page_id == INVALID_PAGE_ID || !page_table_->Find(page_id, target)) {
        return false;
    }

    disk_manager_->WritePage(page_id, target->GetData());
    target->is_dirty_ = false;

    return true;
}

/**
 * User should call this method for deleting a page. This routine will call
 * disk manager to deallocate the page. First, if page is found within page
 * table, buffer pool manager should be reponsible for removing this entry out
 * of page table, reseting page metadata and adding back to free list. Second,
 * call disk manager's DeallocatePage() method to delete from disk file. If
 * the page is found within page table, but pin_count != 0, return false
 */
bool BufferPoolManager::DeletePage(page_id_t page_id) {
    std::lock_guard<std::mutex> lock(latch_);

    Page *target = nullptr;
    if (page_id == INVALID_PAGE_ID || !page_table_->Find(page_id, target)) {
        return false;
    }

    if (target->GetPinCount() !=  0) {
        return false;
    }

    auto flag = page_table_->Remove(page_id);
    assert(flag);
    flag = replacer_->Erase(target);
    assert(flag);
    target->is_dirty_ = false;
    target->page_id_ = INVALID_PAGE_ID;
    target->ResetMemory();
    free_list_->push_back(target);
    disk_manager_->DeallocatePage(page_id);

    return true;
}

/**
 * User should call this method if needs to create a new page. This routine
 * will call disk manager to allocate a page.
 * Buffer pool manager should be responsible to choose a victim page either
 * from free list or lru replacer(NOTE: always choose from free list first),
 * update new page's metadata, zero out memory and add corresponding entry
 * into page table. return nullptr if all the pages in pool are pinned
 */
Page *BufferPoolManager::NewPage(page_id_t &page_id) {
    std::lock_guard<std::mutex> lock(latch_);

    Page *target = nullptr;
    if (!free_list_->empty()) {
        target = *free_list_->begin();
        free_list_->pop_front();
    }
    else {
        if (!replacer_->Victim(target) || target->pin_count_ > 0) {
            return nullptr;
        }
        if (target->is_dirty_) {
            disk_manager_->WritePage(target->GetPageId(), target->GetData());
            target->is_dirty_ = false;
        }
        target->ResetMemory();
        page_table_->Remove(target->GetPageId());
    }

    page_id = disk_manager_->AllocatePage();
    page_table_->Insert(page_id, target);
    target->page_id_ = page_id;
    target->pin_count_ = 1;

    return target;
}
} // namespace cmudb
