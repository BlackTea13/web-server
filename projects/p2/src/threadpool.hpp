#pragma once

#include <queue>
#include <memory>
#include <atomic>
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>

// With help from here :)
// https://stackoverflow.com/questions/15752659/thread-pooling-in-c11

template <typename T>
class WorkQueue {
    private:
        struct Node {
            T data;
            Node* next;
            Node(T data) : data(data), next(nullptr) {}
        };

        std::atomic<Node*> head;
        std::atomic<Node*> tail;
        std::atomic_int size;

    public:
        WorkQueue() : head(nullptr), tail(nullptr) {}

        ~WorkQueue(){
            while (Node* node = head.load()) {
                head.store(node->next);
                free(node);
            }
        }

        int get_size() {
            return size.load();
        }

        void enqueue(const T item) {
            Node* newNode = new Node(item);
            newNode->next = nullptr;

            Node* curTail = tail.load();

            while (!tail.compare_exchange_weak(curTail, newNode)) {
                curTail = tail.load();
            }

            if (curTail != nullptr) {
                curTail->next = newNode;
            } else {
                head.store(newNode);
            }

            size++;
        }

        T dequeue() {
            Node* curHead = head.load();
            Node* curTail = tail.load();
            Node* nextNode = nullptr;
            T result = nullptr;
            while (curHead != curTail) {
                nextNode = curHead->next;
                if (head.compare_exchange_weak(curHead, nextNode)) {
                    result = curHead->data;
                    size--;
                    return result;
                }
            }
            return result;
        }

        bool empty() {
            return head.load() == nullptr && size.load() == 0;
        }
};

class ThreadPool {
    public:
        void start(size_t num_threads);
        void stop();
        void add_job(std::function<void()> job);
        bool busy();

    private:
        std::atomic_bool poison_pill;
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> work_queue;
        std::condition_variable condition;
        std::mutex mutex;
        void loop();
};
