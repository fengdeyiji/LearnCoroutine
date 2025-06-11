#import <print>
#import <coroutine>
#import <cstdlib>
#import <iostream>

static int print_times = 0;
template <typename ...Args>
void log(std::format_string<Args...> __fmt, Args &&...args) {
  std::print("{:>2d} :", ++print_times);
  std::print(__fmt, std::forward<Args>(args)...);
  std::cout << std::endl;
}

struct LinkedCoroutine {
  LinkedCoroutine() : prev_(this), next_(this), handle_() {};
  LinkedCoroutine(const LinkedCoroutine &) = delete;
  LinkedCoroutine &operator=(const LinkedCoroutine &) = delete;
  ~LinkedCoroutine() {
    prev_ = nullptr;
    next_ = nullptr;
  }
  bool empty() { return prev_ == this; }
  void link_next(LinkedCoroutine &new_coroutine) {
    LinkedCoroutine *tmp_next = next_;
    next_ = &new_coroutine;
    new_coroutine.next_ = tmp_next;
    new_coroutine.prev_ = this;
    tmp_next->prev_ = &new_coroutine;
  }
  void remove_self() {
    LinkedCoroutine *tmp_prev = prev_;
    LinkedCoroutine *tmp_next = next_;
    if (!empty()) {
      tmp_prev->next_ = tmp_next;
      tmp_next->prev_ = tmp_prev;
      next_ = nullptr;
      prev_ = nullptr;
    }
  }
  LinkedCoroutine *prev_;
  LinkedCoroutine *next_;
  std::coroutine_handle<> handle_;
};

template  <typename Ret>
class MyPromise;

template <typename MyFuture>
concept ValidFuture = requires {
  typename MyFuture::return_type;
  typename MyFuture::promise_type;
};

template <typename Ret>
struct MyFuture {
  using return_type = Ret;
  using promise_type = MyPromise<Ret>;
  MyFuture(std::coroutine_handle<promise_type> h) : promise_(&h.promise()) {}
  MyFuture(const MyFuture<Ret> &) = delete; // disallow copy
  MyFuture<Ret> &operator=(const MyFuture<Ret> &) = delete; // disallow copy
  MyFuture(MyFuture<Ret> &&rhs) : promise_(rhs.promise_) { rhs.promise_ = nullptr; } // allow move
  MyFuture<Ret> &operator=(MyFuture<Ret> &&rhs) { new (this) MyFuture(rhs); return *this; } // allow move
  ~MyFuture();
  void resume();
  bool done();
  Ret &get_result();
  promise_type *promise_;
};

template <typename Ret>
struct MyPromise {
  static void *operator new(std::size_t size) {
    void *ptr = std::malloc(size);
    if (!ptr) [[unlikely]] throw std::bad_alloc();
    log("alloc memory for coroutine frame:{:p}", ptr);
    return ptr;
  }
  static void operator delete(void *ptr) {
    std::free(ptr);
    log("free memory for coroutine frame:{:p}", ptr);
  }
  MyPromise(int idx) {
    coro_handle_.handle_ = std::coroutine_handle<MyPromise<Ret>>::from_promise(*this);
    log("promise constructed in frame {:p} for args {:d}", this->coro_handle_.handle_.address(), idx);
  }
  ~MyPromise() {
    log("promise destructed in frame {:p}", this->coro_handle_.handle_.address());
  }
  template <typename MiddleResult>
  struct MiddleAwaitable {
    MiddleAwaitable(MyFuture<MiddleResult> &&task) : task_(std::move(task)) {}
    constexpr bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<>) const noexcept {
      log("MiddleAwaitable suspend, thread control transfer to child frame {:p}", task_.promise_->coro_handle_.handle_.address());
      return task_.promise_->coro_handle_.handle_;
    }
    MiddleResult &await_resume() const noexcept {
      log("MiddleResult resume in frame {:p} get result {:d}", task_.promise_->coro_handle_.handle_.address(), task_.promise_->result_);
      return task_.promise_->result_;
    }
    MyFuture<MiddleResult> task_;
  };
  struct FinalAwaitable {
    constexpr bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> this_coro) const noexcept {
      std::coroutine_handle<> ret = std::noop_coroutine(); // noop协程的resume动作不会导致未定义行为，线程控制权将直接返回caller
      if (!handle_.empty()) { // 子协程在最后一次挂起时，将线程转移到母协程(如存在)的挂起点上
        ret =  handle_.prev_->handle_;
        handle_.remove_self();
        log("FinalAwaitable suspend in frame {:p}, thread control transfer to parent frame {:p}", this_coro.address(), ret.address());
      } else {
        log("FinalAwaitable suspend in frame {:p}, thread control transfer to main()", this_coro.address());
      }
      return ret;
    }
    void await_resume() const noexcept { std::abort(); }// final suspend挂起后，await_resume便不可再被调用
    LinkedCoroutine &handle_;
  };
  std::suspend_always initial_suspend() noexcept { return {}; }
  FinalAwaitable final_suspend() noexcept { return {coro_handle_}; }
  void unhandled_exception() { std::abort(); } // 异常处理
  template <typename MyFuture, std::enable_if_t<ValidFuture<MyFuture>, bool> = false>
  MiddleAwaitable<typename MyFuture::return_type> await_transform(MyFuture &&task) {
    coro_handle_.link_next(task.promise_->coro_handle_);
    return {std::move(task)};
  }
  template <typename Others, std::enable_if_t<!ValidFuture<Others>, bool> = false>
  auto await_transform(Others &&others) { return std::forward<Others>(others); }
  auto get_return_object() { return MyFuture{std::coroutine_handle<MyPromise<Ret>>::from_promise(*this)}; }
  void return_value(int value) { result_ = value; }
  Ret result_;
  LinkedCoroutine coro_handle_;
};

template <typename Ret>
MyFuture<Ret>::~MyFuture() {
  if (promise_) {
    promise_->coro_handle_.handle_.destroy();
  }
}

template <typename Ret>
void MyFuture<Ret>::resume() {
  LinkedCoroutine &linked_handle = promise_->coro_handle_;
  if (linked_handle.empty()) {
    log("resume frame {:p}", linked_handle.handle_.address());
    linked_handle.handle_.resume();
  } else {
    log("resume frame {:p}", linked_handle.prev_->handle_.address());
    linked_handle.prev_->handle_.resume();
  }
}

template <typename Ret>
bool MyFuture<Ret>::done() {
  return promise_->coro_handle_.handle_.done();
}

template <typename Ret>
Ret &MyFuture<Ret>::get_result() {
  return promise_->result_;
}

MyFuture<int> coro_fib(int idx) {
  co_await std::suspend_always{};
  int ret = 0;
  if (idx == 1) {
    ret = 1;
  } else if (idx == 2) {
    ret = 1;
  } else {
    ret = co_await coro_fib(idx - 1) + co_await coro_fib(idx - 2);
  }
  co_return ret;
}

int main() {
  MyFuture<int> task = coro_fib(3);
  while (!task.done()) {
    log("control return to main, keep resume task");
    task.resume();
  }
  log("Result: {}", task.get_result());
  return 0;
}