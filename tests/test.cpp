#include <james/image-loader.hpp>
#include <iostream>
#include <fstream>
#include <cassert>
#include <Windows.h>

james::Image png;
james::Image jpeg;

void FlipByteOrder(james::Image& i) {
  unsigned char* pixel = i.Pixels();
  unsigned char* end = &i.Pixels()[i.Width()*i.Height()*4];

  while (pixel != end) {
    std::swap(pixel[0], pixel[2]);
    pixel += i.BitsPerPixel() >> 3;
  }
}

void Paint(HDC dc) {
  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = png.Width();
  bmi.bmiHeader.biHeight = -(int)png.Height();
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = png.BitsPerPixel();
  bmi.bmiHeader.biCompression = BI_RGB;

  int result = StretchDIBits(
    dc,
    0, 0, png.Width(), png.Height(), // dst rect
    0, 0, png.Width(), png.Height(), // src rect
    png.Pixels(), &bmi, 0, SRCCOPY
    );

  assert(result > 0);
}

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  switch (m) {
  case WM_CLOSE:
    PostQuitMessage(0);
    break;

  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(h, &ps);
    Paint(dc);
    EndPaint(h, &ps);
    return 0;
  }
  }

  return DefWindowProc(h, m, w, l);
}

int main() {
  std::ifstream src("Tux.png", std::ios::in | std::ios::binary);
  assert(src.good());
  png = james::LoadPNG(src);

  FlipByteOrder(png);

  WNDCLASS wc = {};
  wc.lpfnWndProc = WndProc;
  wc.lpszClassName = "TestClass";
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  RegisterClass(&wc);

  HWND wnd = CreateWindow("TestClass", "",
    WS_POPUP | WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
    nullptr, nullptr, (HINSTANCE) GetModuleHandle(nullptr), nullptr);
  assert(wnd);

  MSG m;
  while (GetMessage(&m, nullptr, 0, 0)) {
    TranslateMessage(&m);
    DispatchMessage(&m);
  }

  return m.wParam;
}