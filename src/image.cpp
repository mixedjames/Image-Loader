/*
  Image Loader
  Copyright (C) 2017 James Heggie

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <james/image-loader.hpp>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace james {

  Image::Image()
    : w_(0), h_(0), bpp_(32), pixels_(nullptr)
  {
  }

  Image::Image(const Image& src)
    : w_(src.w_), h_(src.h_), bpp_(src.bpp_), pixels_(new unsigned char[w_*h_*(src.bpp_>>3)])
  {
    std::memcpy(pixels_, src.pixels_, w_*h_*4);
  }

  Image::Image(Image&& src) noexcept
    : w_(src.w_), h_(src.h_), bpp_(src.bpp_), pixels_(src.pixels_)
  {
    src.w_ = 0;
    src.h_ = 0;
    src.pixels_ = nullptr;
  }

  Image::Image(unsigned int w, unsigned int h, unsigned int bpp)
    : w_(w), h_(h), bpp_(bpp), pixels_(new unsigned char[w*h*(bpp>>3)])
  {
#ifndef NDEBUG
    assert(bpp == 8 || bpp == 24 || bpp == 32);
#endif

    if (bpp != 8 && bpp != 24 && bpp != 32) {
      throw std::invalid_argument("james::Image only supports 8, 24 & 32bpp images.");
    }
  }

  Image::~Image() noexcept {
    delete pixels_;
  }

  Image& Image::operator= (const Image& src) {
    Image newImg(src);
    Swap(newImg);
    return *this;
  }

  Image& Image::operator= (Image&& src) noexcept {
    Image newImg(std::move(src));
    Swap(newImg);
    return *this;
  }

  void Image::Swap(Image& src) noexcept {
    std::swap(w_, src.w_);
    std::swap(h_, src.h_);
    std::swap(bpp_, src.bpp_);
    std::swap(pixels_, src.pixels_);
  }

}