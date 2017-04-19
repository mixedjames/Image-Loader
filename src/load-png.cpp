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

    // PNGLoaderState
    //
    // Gateway between LoadPNG & ReadBytes
    //
    struct PNGLoaderState {
      std::istream& src;
      char* errorMessage;
      std::exception_ptr currentError;

      PNGLoaderState(std::istream& src) : src(src), errorMessage(nullptr) {}
    };

    void ReadBytes(png_structp png, png_bytep buffer, png_size_t length)
    {
      PNGLoaderState* state = (PNGLoaderState*) png_get_io_ptr(png);

      try {
        if (length != state->src.rdbuf()->sgetn((char*)buffer, length)) {
          png_error(png, "Unexpected end of file.");
        }
      }
      catch (...) {
        state->currentError = std::current_exception();
        png_error(png, "IO error.");
      }

      if (!state->src.good()) {
        state->errorMessage = "IO error occured";
        png_error(png, "IO error.");
      }
    }
  }

  Image LoadPNG(std::istream& src) {
    PNGLoaderState state(src);

    Image img;
    png_structp png = nullptr;
    png_infop info = nullptr;
    png_bytepp rowPtrs = nullptr;
    unsigned char* dstPtr = nullptr;
    size_t rowWidth;

    png_uint_32 w;
    png_uint_32 h;
    int channelWidth, nChannels;

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
      state.errorMessage = "Failed to initialise libpng (png_create_read_struct failed)";
      goto onError;
    }

    info = png_create_info_struct(png);
    if (!info) {
      state.errorMessage = "Failed to initialise libpng (png_create_info_struct failed)";
      goto onError;
    }

    if (setjmp(png_jmpbuf(png))) {
      state.errorMessage = "libPNG encountered an internal error (longjmp called)";
      goto onError;
    }

    png_set_read_fn(png, &state, ReadBytes);
    png_read_png(
      png, info,
      PNG_TRANSFORM_SCALE_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_GRAY_TO_RGB,
      nullptr);
    
    rowPtrs = png_get_rows(png, info);
    w = png_get_image_width(png, info);
    h = png_get_image_height(png, info);
    channelWidth = png_get_bit_depth(png, info);
    nChannels = png_get_channels(png, info);
    rowWidth = w*nChannels;

    try {
      if (channelWidth != 8) {
        throw std::runtime_error("LoadPNG internal error: channel width was not 8 bits");
      }
      if (nChannels != 3 && nChannels != 4) {
        throw std::runtime_error("LoadPNG internal error: number of channels was not 3 or 4");
      }

      img = Image(w, h, nChannels<<3);
    }
    catch (...) {
      state.currentError = std::current_exception();
      goto onError;
    }

    std::size_t height = img.Height();
    dstPtr = img.Pixels();

    while (height --) {
      memcpy(dstPtr, *rowPtrs, rowWidth);

      dstPtr += rowWidth;
      rowPtrs++;
    }

    png_destroy_read_struct(&png, &info, nullptr);

    return img;

  onError:
    if (png) { png_destroy_read_struct(&png, &info, nullptr); }

    if (state.currentError) {
      std::rethrow_exception(state.currentError);
    }
    else {
      throw std::runtime_error(state.errorMessage);
    }
  }

}