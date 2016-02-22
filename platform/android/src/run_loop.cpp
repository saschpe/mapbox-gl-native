#include <mbgl/util/run_loop.hpp>
#include <mbgl/util/async_task.hpp>
#include <mbgl/util/thread_local.hpp>

#include <jni/jni.hpp>
#include <jni/class.hpp>
#include <jni/method.hpp>

#include <jni.hpp>
#include <jni.h>

#include <cassert>

namespace mbgl {
namespace util {

static ThreadLocal<RunLoop>& current = *new ThreadLocal<RunLoop>;
static RunLoop mainRunLoop;

class RunLoop::Impl {
public:
    jni::JavaVM& vm { *mbgl::android::theJVM };
    jni::UniqueEnv env { jni::AttachCurrentThread(vm) };

    struct Looper { static constexpr auto path = "android/os/Looper"; };
    struct Handler { static constexpr auto path = "android/os/Handler"; };
    struct Runnable { static constexpr auto path = "java/lang/Runnable"; };

    jni::Class<Looper> looperClass { *env };

    jni::StaticMethod<Looper, void ()> prepare
        { looperClass, "prepare" };
    jni::StaticMethod<Looper, void ()> loop
        { looperClass, "loop" };
    jni::StaticMethod<Looper, jni::Object<Looper> ()> myLooper
        { looperClass, "myLooper" };
    jni::Method<Looper, void ()> quitSafely
        { looperClass, "quitSafely" };

    jni::Class<Handler> handlerClass { *env };

    jni::Constructor<Handler, jni::Object<Looper>> newHandler
        { handlerClass };
    jni::Method<Handler, bool (jni::Object<Runnable>)> post
        { handlerClass, "post" };
    jni::Method<Handler, bool (jni::Object<Runnable>)> postDelayed
        { handlerClass, "postDelayed" };
    jni::Method<Handler, void (jni::Object<Runnable>)> removeCallbacks
        { handlerClass, "removeCallbacks" };

    jni::Object<Looper> looper    { ( prepare(), myLooper() ) };
    jni::Object<Handler> handler  { newHandler(looper) };

    std::unique_ptr<AsyncTask> async;

    void run() {
        loop();
    }

    void stop() {
        quitSafely(looper);
    }
};

RunLoop* RunLoop::Get() {
    assert(current.get());
    return current.get();
}

RunLoop::RunLoop(Type)
  : impl(std::make_unique<Impl>()) {
    current.set(this);
    impl->async = std::make_unique<AsyncTask>(std::bind(&RunLoop::process, this));
}

RunLoop::~RunLoop() {
    current.set(nullptr);
}

void RunLoop::push(std::shared_ptr<WorkTask> task) {
    withMutex([&] { queue.push(std::move(task)); });
    impl->async->send();
}

void RunLoop::run() {
    impl->run();
}

void RunLoop::stop() {
    invoke([&] { impl->stop(); });
}

} // namespace util
} // namespace mbgl
