#import <initializer_list>
#import <print>
#import <thread>
#import <coroutine>
#import <exception>
#import <iostream>
#import <list>
#import <condition_variable>
#import <mutex>
#import <functional>
class MyPromise;

struct MyFuture {
  using promise_type = MyPromise;
  // 构造函数接收协程句柄
  MyFuture(std::coroutine_handle<promise_type> h) : handle_(h) {}
  ~MyFuture() {
    if (handle_) {
      handle_.destroy(); // 释放资源
    }
  }
  std::coroutine_handle<promise_type> handle_;
};

struct MyPromise {
  std::suspend_always initial_suspend() noexcept { return {}; } // 协程立即开始
  std::suspend_always final_suspend() noexcept { return {}; } // 协程结束时不挂起
  auto get_return_object() { return MyFuture{std::coroutine_handle<MyPromise>::from_promise(*this)}; }
  void return_value(int64_t value) { result_ = value; }
  void unhandled_exception() { std::terminate(); } // 异常处理
  int64_t result_;
};

struct AsyncWorker {
  AsyncWorker() : stop_flag_(false) {
    new (&thread_) std::jthread([this]() {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock lk(lock_);
          cv_.wait(lk, [this]{ return stop_flag_ || !task_queue_.empty(); });
          if (stop_flag_) {
            break;
          } else {
            task = std::move(*task_queue_.begin());
            task_queue_.pop_front();
          }
        }
        task();
      }
    });
  }
  ~AsyncWorker() { thread_.join(); }
  static AsyncWorker &get_instance() {
    static AsyncWorker async_worker;
    return async_worker;
  }
  template <typename Function>
  void append_task(Function &&function) {
    std::lock_guard<std::mutex> lg(lock_);
    task_queue_.push_back(std::forward<Function>(function));
    cv_.notify_all();
  }
  void stop() {
    std::lock_guard lk(lock_);
    stop_flag_ = true;
    cv_.notify_all();
  }
private:
  bool stop_flag_;
  std::jthread thread_;
  std::mutex lock_;
  std::condition_variable cv_;
  std::list<std::function<void()>> task_queue_;
};

struct MyCoroScheduler {
  using CoroHandle = std::coroutine_handle<MyPromise>;
  static MyCoroScheduler &get_instance() {
    static MyCoroScheduler scheduler;
    return scheduler;
  }
  void init(const std::initializer_list<MyFuture *> &initializer_list) {
    for (auto &my_future : initializer_list) {
      await_ready_queue_.push_back(my_future->handle_);
    }
    total_running_task_size_ = initializer_list.size();
  }
  void append_await_ready_task(CoroHandle handle) {
    std::lock_guard<std::mutex> lg(lock_);
    await_ready_queue_.push_back(handle);
    cv_.notify_all();
  }
  void event_loop() {
    while (total_running_task_size_ > 0) {
      CoroHandle fetched_ready_coro;
      {
        std::unique_lock lk(lock_);
        cv_.wait(lk, [this]{ return !await_ready_queue_.empty(); });
        fetched_ready_coro = *await_ready_queue_.begin();
        await_ready_queue_.pop_front();
      }
      fetched_ready_coro.resume();
      if (fetched_ready_coro.done()) {
        total_running_task_size_--;
      }
    }
  }
private:
  mutable std::mutex lock_;
  std::condition_variable cv_;
  std::list<CoroHandle> await_ready_queue_;
  std::list<CoroHandle> done_queue_;
  int64_t total_running_task_size_;
};

auto async_get_input() {
  struct AwaitGetStdInput {
    constexpr bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<MyPromise> handle) {
      AsyncWorker::get_instance().append_task([this, handle]() {
        std::cin >> this->input_value_;
        MyCoroScheduler::get_instance().append_await_ready_task(handle);
      });
    }
    int64_t await_resume() const { return input_value_; }
    int64_t input_value_;
  } awaiter;
  return awaiter;
}

MyFuture coro_add() {
  int64_t a = co_await async_get_input();
  int64_t b = co_await async_get_input();
  co_return a + b;
}

int main() {
  MyFuture future1 = coro_add();
  MyFuture future2 = coro_add();
  MyCoroScheduler::get_instance().init({&future1, &future2});
  MyCoroScheduler::get_instance().event_loop();
  std::print("Result1: {}\n", future1.handle_.promise().result_);
  std::print("Result2: {}\n", future2.handle_.promise().result_);
  AsyncWorker::get_instance().stop();
  return 0;
}