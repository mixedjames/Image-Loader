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

#include <utility>
#include <stdexcept>
#include <cassert>
#include <vector>
#include <png.h>

#include <iostream>

namespace james {

  namespace {

    void ReadBytes(png_structp png, png_bytep buffer, png_size_t length)
    {
      std::istream* src = (std::istream*) png_get_io_ptr(png);
      if (length != src->rdbuf()->sgetn((char*)buffer, length)) {
        png_error(png, "Unexpected end of file.");
      }

      assert(src->good());
    }
  }

  Image LoadPNG(std::istream& src) {
    assert(src.exceptions() == std::istream::goodbit);

    char* errorMessage;
    std::exception_ptr currentError;

    Image img;
    png_structp png = nullptr;
    png_infop info = nullptr;
    png_bytepp rowPtrs = nullptr;
    unsigned char* dstPtr = nullptr;

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
      errorMessage = "Failed to initialise libpng (png_create_read_struct failed)";
      goto onError;
    }

    info = png_create_info_struct(png);
    if (!info) {
      errorMessage = "Failed to initialise libpng (png_create_info_struct failed)";
      goto onError;
    }

    if (setjmp(png_jmpbuf(png))) {
      errorMessage = "libPNG encountered an internal error (longjmp called)";
      goto onError;
    }

    png_set_read_fn(png, &src, ReadBytes);
    png_read_png(png, info, 0, nullptr);
    rowPtrs = png_get_rows(png, info);

    try {
      img = Image(png_get_image_width(png, info), png_get_image_height(png, info));
    }
    catch (...) {
      currentError = std::current_exception();
      goto onError;
    }

    std::size_t height = img.Height();
    dstPtr = img.Pixels();

    while (height --) {
      memcpy(dstPtr, *rowPtrs, img.Width()*4);

      dstPtr += img.Width()*4;
      rowPtrs++;
    }

    png_destroy_read_struct(&png, &info, nullptr);

    return img;

  onError:
    if (png) { png_destroy_read_struct(&png, &info, nullptr); }

    if (currentError) {
      std::rethrow_exception(currentError);
    }
    else {
      throw std::runtime_error(errorMessage);
    }
  }

}