#import <coroutine>
#import <exception>
#import <iostream>

class MyPromise;

struct FibGenerator {
  using promise_type = MyPromise;
  // 构造函数接收协程句柄
  FibGenerator(std::coroutine_handle<promise_type> h) : handle_(h) {}
  ~FibGenerator() {
    if (handle_) {
      handle_.destroy(); // 释放资源
    }
  }
  int generate_value();
  std::coroutine_handle<promise_type> handle_;
};

struct MyPromise {
  std::suspend_never initial_suspend() noexcept { return {}; } // 协程立即开始
  std::suspend_always final_suspend() noexcept { return {}; } // 协程结束时不挂起
  auto get_return_object() { return FibGenerator{std::coroutine_handle<MyPromise>::from_promise(*this)}; }
  void return_void() {}
  void unhandled_exception() { std::terminate(); } // 异常处理
  std::suspend_always yield_value(int value) { current_value_ = value; return {}; }
  int current_value_;
};

int FibGenerator::generate_value() {
  handle_.resume();
  return handle_.promise().current_value_;
}

FibGenerator coro_generate_fibo() {
  int a = 1;
  int b = 1;
  int new_value = 0;
  co_yield a;
  co_yield b;
  while (true) {
    co_yield (new_value = a + b);
    a = b;
    b = new_value;
  }
  co_return;
}

int main() {
  auto gen = coro_generate_fibo();
  std::cout << 1 << std::endl;
  for (int i = 0; i < 10; ++i) {
    std::cout << gen.generate_value() << std::endl; 
  }
  return 0;
}