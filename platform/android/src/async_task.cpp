#include <mbgl/util/async_task.hpp>

#include <atomic>

namespace mbgl {
namespace util {

class AsyncTask::Impl {
public:
    Impl(std::function<void()>&& fn)
        : task(std::move(fn)) {
    }

    ~Impl() {
        env.CallVoidMethod(handler, removeCallbacks, runnable);
    }

    void maySend() {
        if (!queued.test_and_set()) {
            env.CallBooleanMethod(handler, post, runnable);
        }
    }

    void runTask() {
        queued.clear();
        task();
    }

private:
    std::function<void()> task;
    std::atomic_flag queued = ATOMIC_FLAG_INIT;
};

AsyncTask::AsyncTask(std::function<void()>&& fn)
    : impl(std::make_unique<Impl>(std::move(fn))) {
}

AsyncTask::~AsyncTask() = default;

void AsyncTask::send() {
    impl->maySend();
}

} // namespace util
} // namespace mbgl
