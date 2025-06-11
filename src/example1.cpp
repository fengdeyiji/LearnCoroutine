#import <print>
#import <coroutine>
#import <exception>

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
  std::suspend_never initial_suspend() noexcept { return {}; } // 协程立即开始
  std::suspend_always final_suspend() noexcept { return {}; } // 协程结束时不挂起
  auto get_return_object() { return MyFuture{std::coroutine_handle<MyPromise>::from_promise(*this)}; }
  void return_value(int value) { result_ = value; }
  void unhandled_exception() { std::terminate(); } // 异常处理
  int result_;
};

MyFuture coro_add(int a, int b) {
  co_return a + b;
}

int main() {
  MyFuture coro_future = coro_add(1, 2);
  std::print("Result: {}\n", coro_future.handle_.promise().result_);
  return 0;
}