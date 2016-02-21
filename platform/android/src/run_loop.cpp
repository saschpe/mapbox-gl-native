#include <mbgl/util/run_loop.hpp>
#include <mbgl/util/async_task.hpp>
#include <mbgl/util/thread_local.hpp>

#include <jni/jni.hpp>

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
//        handler = env.NewObject(handlerClass, handlerNew, looper);
    }

    void run() {
//        env.CallStaticVoidMethod(looperClass, loop);
    }

    void stop() {
//        env.CallVoidMethod(looper, quitSafely);
    }

    jni::JavaVM& vm { *mbgl::android::theJVM };
    jni::UniqueEnv env { jni::AttachCurrentThread(vm) };

    jni::UniqueGlobalRef<jni::jclass> looperClass
        { jni::NewGlobalRef(*env, jni::FindClass(*env, "android/os/Looper")) };
    const jni::jmethodID& prepare
        { jni::GetStaticMethodID(*env, *looperClass, "prepare", "()V") };
    const jni::jmethodID& loop
        { jni::GetStaticMethodID(*env, *looperClass, "loop", "()V") };
    const jni::jmethodID& myLooper
        { jni::GetStaticMethodID(*env, *looperClass, "myLooper", "()Landroid/os/Looper;") };
    const jni::jmethodID& quitSafely
        { jni::GetMethodID(*env, *looperClass, "quitSafely", "()V") };

    jni::UniqueGlobalRef<jni::jclass> handlerClass
        { jni::NewGlobalRef(*env, jni::FindClass(*env, "android/os/Handler")) };
    const jni::jmethodID& handlerNew
        { jni::GetMethodID(*env, *handlerClass, "<init>", "(Landroid/os/Looper;)V") };
    const jni::jmethodID& post
        { jni::GetMethodID(*env, *handlerClass, "post", "(Ljava/lang/Runnable;)B") };
    const jni::jmethodID& postDelayed
        { jni::GetMethodID(*env, *handlerClass, "postDelayed", "(Ljava/lang/Runnable;L)B") };
    const jni::jmethodID& removeCallbacks
        { jni::GetMethodID(*env, *handlerClass, "removeCallbacks", "(Ljava/lang/Runnable;)V") };

//    const jni::jobject& looper
//        { ( jni::CallStaticVoidMethod(*env, *looperClass, prepare),
//            jni::CallStaticObjectMethod(*env, *looperClass, myLooper) ) };

    jni::jobject* looper
        { jni::CallStaticObjectMethod(*env, *looperClass, myLooper) };

    const jni::jobject* handler;

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
