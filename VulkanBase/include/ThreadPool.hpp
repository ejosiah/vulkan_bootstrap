#pragma once

#include "common.h"

namespace par{

    struct done{};

    class ThreadPool {
    public:
        using UniqueFunction = std::packaged_task<void()>;

        explicit ThreadPool(unsigned int size);

        ~ThreadPool();

        template<class F>
        auto async(F&& func)  const {
            using ResultType = std::invoke_result_t<std::decay_t<F>>;
            std::packaged_task<ResultType()> pt(std::forward<F>(func));
            std::future<ResultType> future = pt.get_future();

            auto capture = [](auto& p) {
                using T = std::decay_t<decltype(p)>;
                return std::make_shared<T>(std::move(p));
            };

            UniqueFunction task( [pt = capture(pt)] () mutable {
                pt->operator()();
            } );

            enqueue(std::move(task));

            return future;
        }

        static inline ThreadPool& global() {
            return _global;
        }

        template<typename T, typename F>
        auto onComplete(std::shared_future<T> future, F&& func, const std::string& errorMsg = ""){
            return
                async([=]() mutable {
                    try {
                        auto res = future.get();
                        func(res);
                    }catch (...){
                        // TODO add on error function
                        spdlog::info("background task error: {}", errorMsg);
                        throw;
                    }
                });
        }

    protected:
        void enqueue(UniqueFunction task) const;

        void worker_loop();

    private:
        mutable struct {
            std::mutex mtx;
            std::queue<UniqueFunction> work_queue;
            bool aborting = false;
        } m_state;
        std::vector<std::thread> m_workers;
        mutable std::condition_variable m_cv;
        static ThreadPool _global;
    };

    template<typename T>
    inline std::future<std::vector<T>> sequence(std::vector<std::future<T>>& futures) {
        return ThreadPool::global().async([&futures] {
            std::vector<T> result;
            for (auto& f : futures) {
                result.push_back(f.get());
            }
            return result;
        });
    }
}