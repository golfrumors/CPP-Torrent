#ifndef SHAREDQUEUE_H
#define SHAREDQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <ostream>

//thread safe queue
template <typename T>
class SharedQueue {
    public:
        SharedQueue();
        ~SharedQueue();

        T& front();
        T& pop_front();

        void push_back(const T& item);
        void push_back(T&& item);
        void clear();

        int size();
        bool empty();

    private:
        std::deque<T> _queue;
        std::mutex _mutex;
        std::condition_variable _cond;
};

template <typename T>
SharedQueue<T>::SharedQueue() = default;

template <typename T>
SharedQueue<T>::~SharedQueue() = default;

template <typename T>
T& SharedQueue<T>::front() {
    std::unique_lock<std::mutex> mlock(_mutex);
    while (_queue.empty()) {
        _cond.wait(mlock);
    }
    return _queue.front();
}

template <typename T>
T& SharedQueue<T>::pop_front() {
    std::unique_lock<std::mutex> mlock(_mutex);
    while (_queue.empty()) {
        _cond.wait(mlock);
    }
    T& front = _queue.front();
    _queue.pop_front();
    return front;
}

template <typename T>
void SharedQueue<T>::push_back(const T& item) {
    std::unique_lock<std::mutex> mlock(_mutex);
    _queue.push_back(item);
    mlock.unlock();
    _cond.notify_one();
}

template <typename T>
void SharedQueue<T>::push_back(T&& item) {
    std::unique_lock<std::mutex> mlock(_mutex);
    _queue.push_back(std::move(item));
    mlock.unlock();
    _cond.notify_one();
}

//empty the queue
template <typename T>
void SharedQueue<T>::clear() {
    std::unique_lock<std::mutex> mlock(_mutex);
    std::deque<T>().swap(_queue);
    mlock.unlock();
    _cond.notify_one();
}

template <typename T>
int SharedQueue<T>::size() {
    std::unique_lock<std::mutex> mlock(_mutex);
    int size = _queue.size();
    mlock.unlock();
    return size;
}

template <typename T>
bool SharedQueue<T>::empty() {
    return size() == 0;
}

#endif // SHAREDQUEUE_H