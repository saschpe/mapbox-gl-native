#include <mbgl/util/run_loop.hpp>
#include <mbgl/util/async_task.hpp>
#include <mbgl/util/thread_local.hpp>

#include <mbgl/jni/thread_attachment.hpp>
#include <mbgl/jni/global_ref.hpp>

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
        looperClass = env.FindClass("android/os/Looper");
        prepare  = env.GetStaticMethodID(looperClass, "prepare",  "()V");
        loop     = env.GetStaticMethodID(looperClass, "loop",     "()V");
        myLooper = env.GetStaticMethodID(looperClass, "myLooper", "()Landroid/os/Looper;");
        quitSafely = env.GetMethodID(looperClass, "quitSafely", "()V");

        handlerClass    = env.FindClass("android/os/Handler");
        handlerNew      = env.GetMethodID(handlerClass, "<init>", "(Landroid/os/Looper;)V");
        post            = env.GetMethodID(handlerClass, "post", "(Ljava/lang/Runnable;)B");
        postDelayed     = env.GetMethodID(handlerClass, "postDelayed", "(Ljava/lang/Runnable;L)B");
        removeCallbacks = env.GetMethodID(handlerClass, "removeCallbacks", "(Ljava/lang/Runnable;)V");

        env.CallStaticVoidMethod(looperClass, prepare);
        looper = env.CallStaticObjectMethod(looperClass, myLooper);
        handler = env.NewObject(handlerClass, handlerNew, looper);
    }

    void run() {
        env.CallStaticVoidMethod(looperClass, loop);
    }

    void stop() {
        env.CallVoidMethod(looper, quitSafely);
    }

    jni::ThreadAttachment env { mbgl::android::theJVM };

    jni::GlobalRef<jclass> looperClass;
    jmethodID prepare;
    jmethodID loop;
    jmethodID myLooper;
    jmethodID quitSafely;

    jni::GlobalRef<jclass> handlerClass;
    jmethodID handlerNew;
    jmethodID post;
    jmethodID postDelayed;
    jmethodID removeCallbacks;

    jobject looper;
    jobject handler;

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
