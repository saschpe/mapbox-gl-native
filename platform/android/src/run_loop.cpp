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
    Impl() {
    }

    void run() {
        jni::CallStaticMethod<void>(*env, *looperClass, loop);
    }

    void stop() {
        jni::CallMethod<void>(*env, *looper, quitSafely);
    }

    jni::JavaVM& vm { *mbgl::android::theJVM };
    jni::UniqueEnv env { jni::AttachCurrentThread(vm) };

//    struct LooperTag { static constexpr auto path = "android/os/Looper"; };
//    struct HandlerTag { static constexpr auto path = "android/os/Handler"; };

    jni::jclass* looperClass
        { &jni::FindClass(*env, "android/os/Looper") };
    jni::jmethodID& prepare
        { jni::GetStaticMethodID(*env, *looperClass, "prepare", "()V") };
    jni::jmethodID& loop
        { jni::GetStaticMethodID(*env, *looperClass, "loop", "()V") };
    jni::jmethodID& myLooper
        { jni::GetStaticMethodID(*env, *looperClass, "myLooper", "()Landroid/os/Looper;") };
    jni::jmethodID& quitSafely
        { jni::GetMethodID(*env, *looperClass, "quitSafely", "()V") };

    jni::jclass* handlerClass
        { &jni::FindClass(*env, "android/os/Handler") };
    jni::jmethodID& handlerNew
        { jni::GetMethodID(*env, *handlerClass, "<init>", "(Landroid/os/Looper;)V") };
    jni::jmethodID& post
        { jni::GetMethodID(*env, *handlerClass, "post", "(Ljava/lang/Runnable;)B") };
    jni::jmethodID& postDelayed
        { jni::GetMethodID(*env, *handlerClass, "postDelayed", "(Ljava/lang/Runnable;L)B") };
    jni::jmethodID& removeCallbacks
        { jni::GetMethodID(*env, *handlerClass, "removeCallbacks", "(Ljava/lang/Runnable;)V") };

    jni::UniqueGlobalRef<jni::jobject> looper
        { ( jni::CallStaticMethod<void>(*env, *looperClass, prepare),
            jni::NewGlobalRef(*env, *jni::CallStaticMethod<jni::jobject*>(*env, *looperClass, myLooper)) ) };

    jni::UniqueGlobalRef<jni::jobject> handler
        { jni::NewGlobalRef(*env, jni::NewObject(*env, *handlerClass, handlerNew, *looper)) };

    std::unique_ptr<AsyncTask> async;
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
