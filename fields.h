#pragma once

#include <mutex>
#include <shared_mutex>
#include <string>

class ThreeFields
{
public:
    ThreeFields() = default;
    ThreeFields(const ThreeFields &) = delete;
    ThreeFields &operator=(const ThreeFields &) = delete;
    ~ThreeFields() = default;

    int get0() const
    {
        std::lock_guard lock(m0);
        return f0;
    }
    void set0(int value)
    {
        std::lock_guard lock(m0);
        f0 = value;
    }

    int get1() const
    {
        std::shared_lock lock(m1);
        return f1;
    }
    void set1(int value)
    {
        std::lock_guard lock(m1);
        f1 = value;
    }

    int get2() const
    {
        std::lock_guard lock(m2);
        return f2;
    }
    void set2(int value)
    {
        std::lock_guard lock(m2);
        f2 = value;
    }

    operator std::string() const
    {
        int v0, v1, v2;
        {
            std::unique_lock lock0(m0, std::defer_lock);
            std::shared_lock lock1(m1, std::defer_lock);
            std::unique_lock lock2(m2, std::defer_lock);
            std::lock(lock0, lock1, lock2);

            v0 = f0;
            v1 = f1;
            v2 = f2;
        }
        return "[" + std::to_string(v0) + ", " + std::to_string(v1) + ", " + std::to_string(v2) + "]";
    }

private:
    int f0 = 0;
    int f1 = 0;
    int f2 = 0;

    mutable std::mutex m0;
    mutable std::shared_mutex m1;
    mutable std::mutex m2;
};
