#include <james/image-loader.hpp>
#include <iostream>
#include <fstream>

int main() {
  std::ifstream src("Tux.png", std::ios::in | std::ios::binary);
  if (!src.good()) {
    std::cout << "Failed to open file.";
    return 0;
  }

  {
    james::Image a(james::LoadPNG(src));
    std::cout << "Loaded image. (" << a.Width() << "x" << a.Height() << ")\n";
  }

  std::cin.get();

  return 0;
}