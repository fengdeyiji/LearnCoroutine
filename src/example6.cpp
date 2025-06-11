#import <cstddef>
#import <cstdlib>
#import <iostream>
#import <print>
#import <vector>
#import <coroutine>
#import <exception>
#import <stdint.h>
#import <chrono>

class MyPromise;

struct MyFuture {
  using promise_type = MyPromise;
  // 构造函数接收协程句柄
  MyFuture(std::coroutine_handle<promise_type> h) : handle_(h) {}
  std::coroutine_handle<promise_type> handle_;
};

struct MyPromise {
  static int64_t total_alloc_size;
  static void *operator new(std::size_t size) {
    void *ptr = std::malloc(size);
    if (ptr == nullptr) {
      throw std::bad_alloc();
    } else {
      total_alloc_size += size;
    }
    return ptr;
  }
  std::suspend_always initial_suspend() noexcept {  return {}; } // 协程立即开始
  std::suspend_always final_suspend() noexcept { return {}; } // 协程结束时不挂起
  auto get_return_object() { return MyFuture{std::coroutine_handle<MyPromise>::from_promise(*this)}; }
  void return_value(int value) { result_ = value; }
  void unhandled_exception() { std::terminate(); } // 异常处理
  int64_t result_ = 0x11111111;
};
int64_t MyPromise::total_alloc_size = 0;

auto empty_suspend_operation() {
  struct {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) const {}
    void await_resume() const noexcept {}
  } ret;
  return ret;
}

MyFuture coro_add(int a, int b) {
  co_await empty_suspend_operation();
  co_await empty_suspend_operation();
  co_await empty_suspend_operation();
  co_return a + b;
}

int main() {
  while (true) {
    MyPromise::total_alloc_size = 0;
    int64_t coro_num = 0;
    std::cout << "输入协程数量:" << std::endl;
    std::cin >> coro_num;
    std::vector<MyFuture> vec_futures;
    vec_futures.reserve(coro_num);
    void *vec_ptr = &vec_futures[0];
    auto start = std::chrono::high_resolution_clock::now();
    for (int64_t i = 0; i < coro_num; ++i) {
      vec_futures.push_back(coro_add(1, 2));
    }
    if (&vec_futures[0] != vec_ptr) {
      std::terminate();
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    while (!vec_futures[0].handle_.done()) {
      for (auto &future : vec_futures) {
        future.handle_.resume();
      }
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - start).count();
    std::print("创建{:d}协程耗时:{:d}ns, 平均{:.2f}ns\n", coro_num, duration, duration * 1.0 / coro_num);
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    std::print("挂起/恢复{:d}协程(x3)耗时:{:d}ns, 平均{:.2f}ns\n", coro_num, duration, duration * 1.0 / coro_num / 3);
    std::print("used bytes:{:.2f}MB\n", MyPromise::total_alloc_size * 1.0 / 1024 / 1024);
  }
  return 0;
}