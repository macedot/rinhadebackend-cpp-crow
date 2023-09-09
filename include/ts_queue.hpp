#pragma once

#include <shared_mutex>
#include <queue>

template<typename Item>
class ts_queue {
  private:
    std::queue<Item>          _q;
    mutable std::shared_mutex mutex_;

  public:
    // A shared mutex is used to enable mutiple concurrent reads
    auto size() const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return _q.size();
    }

    auto empty() const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return _q.empty();
    }

    // Exclusive lock to enable single write in the bucket
    void push(Item&& key)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        _q.push_back(key);
    }

    void push(const Item& key)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        _q.push(key);
    }

    Item pop()
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);

        auto item = _q.front();
        _q.pop();
        return item;
    }

    void clear()
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        _q.clear();
    }
};
