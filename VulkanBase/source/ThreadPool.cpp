#include "ThreadPool.hpp"

namespace par{
    ThreadPool::ThreadPool(unsigned int size) {
        for (unsigned int i = 0; i < size; i++) {
            m_workers.emplace_back([this]() { worker_loop(); });
        }
    }

    ThreadPool::~ThreadPool() {
        if (std::lock_guard lk(m_state.mtx); true) {
            m_state.aborting = true;
        }
        m_cv.notify_all();
        for (std::thread& t : m_workers) {
            t.join();
        }
    }

    void ThreadPool::enqueue(ThreadPool::UniqueFunction task)  const {
        if (std::lock_guard lk(m_state.mtx); true) {
            m_state.work_queue.push(std::move(task));
        }
        m_cv.notify_one();
    }

    void ThreadPool::worker_loop() {
        while (true) {
            std::unique_lock lk(m_state.mtx);
            while (m_state.work_queue.empty() && !m_state.aborting) {
                m_cv.wait(lk);
            }
            if (m_state.aborting) break;
            //	assert(!m_state.work_queue.empty() == true);
            UniqueFunction task = std::move(m_state.work_queue.front());
            m_state.work_queue.pop();

            lk.unlock();
            task();
        }
    }

    ThreadPool ThreadPool::_global = ThreadPool{ std::thread::hardware_concurrency() };
}

