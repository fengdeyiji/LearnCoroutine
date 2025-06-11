#import <print>
#import <coroutine>
#import <exception>
#import <cstdlib>
#import <sstream>
#import <iostream>
#import <iomanip>
#import <source_location>

#pragma pack(4)

void *CORO_FRAME_PTR = nullptr;
size_t CORO_FRAME_SIZE = 0;

inline void print_memory_content(const std::source_location &loc = std::source_location::current()) {
  std::stringstream ss;
  using word = int32_t;
  word *p_address = (word *)(CORO_FRAME_PTR);
  int word_each_line = 4;
  int lines = CORO_FRAME_SIZE / (word_each_line * sizeof(word));
  for (int line_idx = 0; line_idx < lines; ++line_idx) {
    ss << "\n" << std::setfill('0') << std::setw(8) << std::hex << p_address << ":";
    for (int word_idx = 0; word_idx < word_each_line && ((char *)p_address < (char *)CORO_FRAME_PTR + CORO_FRAME_SIZE); ++word_idx) {
      ss << "\t0x" << std::setfill('0') << std::setw(8) << std::hex << *p_address;
      ++p_address;
    }
  }
  std::print("print at: {}:line {:d}, addr:{:p}, size:{:d}:{}\n", loc.function_name(), loc.line(), CORO_FRAME_PTR, CORO_FRAME_SIZE, ss.str());
}

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
  static void *operator new(std::size_t size) {
    CORO_FRAME_PTR = std::malloc(size);
    if (CORO_FRAME_PTR == nullptr) {
      throw std::bad_alloc();
    }
    CORO_FRAME_SIZE = size;
    std::print("call promise_type::operator new to alloc memory, addr:{:p}, size:{:d}\n", CORO_FRAME_PTR, CORO_FRAME_SIZE);
    return CORO_FRAME_PTR;
  }
  std::suspend_always initial_suspend() noexcept { print_memory_content(); return {}; } // 协程立即开始
  std::suspend_always final_suspend() noexcept { print_memory_content(); return {}; } // 协程结束时不挂起
  auto get_return_object() { return MyFuture{std::coroutine_handle<MyPromise>::from_promise(*this)}; }
  void return_value(int value) { result_ = value; }
  void unhandled_exception() { std::terminate(); } // 异常处理
  int32_t result_ = 0x11111111;
};

auto empty_suspend_operation() {
  struct {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) const { print_memory_content(); }
    void await_resume() const noexcept {}
    int32_t flag_ = 0x22222222;
  } ret;
  return ret;
}

MyFuture coro_add(int a, int b) {
  int32_t var_in_coroutine = 0x33333333;
  co_await empty_suspend_operation();
  co_await empty_suspend_operation();
  co_await empty_suspend_operation();
  co_return a + b;
}

int main() {
  MyFuture coro_future = coro_add(1, 2);
  while (!coro_future.handle_.done()) coro_future.handle_.resume();
  print_memory_content();
  std::print("Result: {}\n", coro_future.handle_.promise().result_);
  return 0;
}