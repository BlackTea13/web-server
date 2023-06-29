#include <mutex>
#include <iostream>
#include "threadpool.hpp"


void ThreadPool::start(size_t num_threads){
    if (num_threads > std::thread::hardware_concurrency()){
        num_threads = std::thread::hardware_concurrency();
    }
    workers.resize(num_threads);
    for(size_t i = 0; i < num_threads; i++){
        workers.at(i) = std::thread(&ThreadPool::loop, this);
    }
    poison_pill = false;
}

void ThreadPool::loop(){
   while (true) {
        std::function<void()> job;
        {   
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [this] { return !work_queue.empty() || poison_pill; });
            if(poison_pill) {return;}
            job = work_queue.front();
            work_queue.pop();
        }
        job();
    }
}

void ThreadPool::add_job(std::function<void()> job){ 
    {
        std::unique_lock<std::mutex> lock(mutex);
        work_queue.push(job);
    }
    condition.notify_one();
}

bool ThreadPool::busy(){
    return work_queue.empty();
}

void ThreadPool::stop(){
    {
        std::unique_lock<std::mutex> lock(mutex);
        poison_pill = true;
    }
    condition.notify_all();
    for(size_t i = 0; i < workers.size(); i++){
        workers.at(i).join();
    }
    workers.clear();
}