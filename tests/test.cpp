#include <james/image-loader.hpp>
#include <iostream>
#include <fstream>
#include <cassert>
#include <Windows.h>

james::Image png;
james::Image jpeg;

void FlipByteOrder(james::Image& i) {
  unsigned char* pixel = i.Pixels();
  unsigned char* end = &i.Pixels()[i.Width()*i.Height()*(i.BitsPerPixel() >> 3)];

  while (pixel != end) {
    std::swap(pixel[0], pixel[2]);
    pixel += i.BitsPerPixel() >> 3;
  }
}

void DrawImage(HDC dc, const james::Image& img, int x, int y) {
  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = img.Width();
  bmi.bmiHeader.biHeight = -(int)img.Height();
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = img.BitsPerPixel();
  bmi.bmiHeader.biCompression = BI_RGB;

  int result = StretchDIBits(
    dc,
    x, y, img.Width(), img.Height(), // dst rect
    0, 0, img.Width(), img.Height(), // src rect
    img.Pixels(), &bmi, 0, SRCCOPY
  );

  assert(result > 0);

}

void Paint(HDC dc) {
  DrawImage(dc, jpeg, 0, 0);
  DrawImage(dc, png, 300, 0);
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
  std::ifstream src2("cube.jpg", std::ios::in | std::ios::binary);

  assert(src.good() && src2.good());

  png = james::LoadPNG(src);
  jpeg = james::LoadJPEG(src2);

  FlipByteOrder(png);
  FlipByteOrder(jpeg);

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