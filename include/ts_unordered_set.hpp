#pragma once

#include <shared_mutex>
#include <unordered_set>

template<typename Value>
class ts_unordered_set {
  private:
    std::unordered_set<Value> _set;
    mutable std::shared_mutex mutex_;

  public:
    // A shared mutex is used to enable mutiple concurrent reads
    auto size() const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return _set.size();
    }

    auto empty() const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return _set.empty();
    }

    bool exists(const Value& value) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return _set.find(value) != _set.end();
    }

    // Exclusive lock to enable single write in the bucket
    void emplace(const Value& value)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        _set.emplace(value);
    }

    void emplace(Value&& value)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        _set.emplace(value);
    }

    void clear()
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        _set.clear();
    }
};
