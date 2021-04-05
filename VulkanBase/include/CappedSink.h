#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/base_sink-inl.h>
#include <array>
#include <string>
#include <memory>
#include <deque>

template<typename Mutex, size_t size>
class CappedSink : public spdlog::sinks::base_sink<Mutex>{
public:
    std::deque<std::string> logs;

    ~CappedSink() override = default;

protected:
    void sink_it_(const spdlog::details::log_msg &msg) final {
        if(!logs.empty() && logs.size() == size){
            logs.pop_back();
        }
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        logs.push_front(fmt::to_string(formatted));
    }

    void flush_() override {

    }


private:
};