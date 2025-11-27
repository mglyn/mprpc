#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

template <typename T>
class LockQueue {
public:

    template <typename U>
    void push(U &&data){
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(std::forward<U>(data));
        cond.notify_one();
    }

    T pop(){
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, [this](){return queue.size();});
        T res = std::move(queue.front());
        queue.pop();
        return res;
    }
private:
    std::queue<T> queue; // 队列
    std::mutex mtx; // 互斥锁
    std::condition_variable cond; // 条件变量
};