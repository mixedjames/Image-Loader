#include <james/image-loader.hpp>
#include <iostream>


int main() {
  james::Image a;
  james::Image b(100, 200);

  a = std::move(b);

  std::cout << "Hellooo!";
  std::cin.get();

  return 0;
}