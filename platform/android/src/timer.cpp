#include <mbgl/util/timer.hpp>

#include <mbgl/jni/thread_attachment.hpp>

#include <jni.hpp>
#include <jni.h>

namespace mbgl {
namespace util {

class Timer::Impl {
public:
    Impl(Duration timeout, Duration repeat, std::function<void()>&& fn)
        : task(std::move(fn)) {
        env.CallBooleanMethod(handler, postDelayed, runnable, timeout);
    }

    ~Impl() {
        env.CallVoidMethod(handler, removeCallbacks, runnable);
    }

private:
    std::function<void()> task;
};

Timer::Timer() = default;
Timer::~Timer() = default;

void Timer::start(Duration timeout, Duration repeat, std::function<void()>&& cb) {
    impl = std::make_unique<Impl>(timeout, repeat, std::move(cb));
}

void Timer::stop() {
    impl.reset();
}

} // namespace util
} // namespace mbgl
