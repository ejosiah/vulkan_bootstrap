#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/base_sink-inl.h>
#include <array>
#include <string>
#include <memory>
#include <boost/circular_buffer.hpp>

template<typename Mutex, int Size>
class VulkanSink : public spdlog::sinks::base_sink<Mutex>{
public:

    ~VulkanSink() override = default;

    auto begin() const {
        return logs.begin();
    }

    auto end() const {
        return logs.end();
    }

protected:
    void sink_it_(const spdlog::details::log_msg &msg) final {

        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        logs.push_back(fmt::to_string(formatted));
    }

    void flush_() override {

    }


private:
    boost::circular_buffer<std::string> logs{Size};
};