#import <print>

int add(int a, int b) {
  return a + b;
}

int main() {
  int result = add(1,2);
  std::print("result:{}\n", result);
  return 0;
}