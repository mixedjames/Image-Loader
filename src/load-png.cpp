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

    struct PNGDataMgr {
      png_structp png;
      png_infop info;

      PNGDataMgr()
        : png(nullptr), info(nullptr)
      {
        png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) {
          throw std::runtime_error("libPNG internal error (png_create_read_struct failed)");
        }

        info = png_create_info_struct(png);
        if (!info) {
          png_destroy_read_struct(&png, nullptr, nullptr);
          throw std::runtime_error("libPNG internal error (png_create_info_struct failed)");
        }
      }

      ~PNGDataMgr() {
        png_destroy_read_struct(&png, &info, nullptr);
      }
    };

    struct PNGLoaderState {
      PNGDataMgr libPNG;
      
      std::istream& src;
      std::ios::iostate streamExceptionState;

      std::exception_ptr currentError;

      PNGLoaderState(std::istream& src)
        : src(src), streamExceptionState(src.exceptions())
      {
        src.exceptions(std::istream::failbit | std::istream::badbit | std::istream::eofbit);
      }

      ~PNGLoaderState() {
        src.exceptions(streamExceptionState);
      }
    };

    void InstallIOAdapter(PNGLoaderState& state) {
      png_set_read_fn(state.libPNG.png, &state, [](png_structp png, png_bytep buffer, png_size_t length) {

        PNGLoaderState* state = (PNGLoaderState*) png_get_io_ptr(png);

        try {
          if (length != state->src.rdbuf()->sgetn((char*)buffer, length)) {
            throw std::runtime_error("Unexpected end of file.");
          }
        }
        catch (...) {
          state->currentError = std::current_exception();
          png_error(state->libPNG.png, "Exception adapter");
        }

      });
    }
  }

  Image LoadPNG(std::istream& src) {
    Image img;
    PNGLoaderState state(src);
    jmp_buf jmpBuf;

    png_uint_32 w;
    png_uint_32 h;
    int channelWidth;
    int nChannels;

    png_bytepp rowPtrs = nullptr;
    unsigned char* dstPtr = nullptr;

    if (setjmp(jmpBuf)) {
      if (state.currentError) {
        std::rethrow_exception(state.currentError);
      }
      else {
        throw std::runtime_error("An unspecified error occured.");
      }
    }

    InstallIOAdapter(state);

    png_read_png(
      state.libPNG.png, state.libPNG.info,
      PNG_TRANSFORM_SCALE_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_GRAY_TO_RGB,
      nullptr);

    w = png_get_image_width(state.libPNG.png, state.libPNG.info);
    h = png_get_image_height(state.libPNG.png, state.libPNG.info);
    channelWidth = png_get_bit_depth(state.libPNG.png, state.libPNG.info);
    nChannels = png_get_channels(state.libPNG.png, state.libPNG.info);

    if (channelWidth != 8) {
      throw std::runtime_error("PNG channel width was not 8. Only 8 is supported.");
    }

    if (nChannels != 3 && nChannels != 4) {
      throw std::runtime_error("Number of PNG colour channels was neither 3 nor 4.");
    }
      
    img = Image(w, h, nChannels << 3);
    
    rowPtrs = png_get_rows(state.libPNG.png, state.libPNG.info);
    dstPtr = img.Pixels();

    while (h --) {
      memcpy(dstPtr, *rowPtrs, w*nChannels);

      dstPtr += w*nChannels;
      rowPtrs++;
    }

    return img;
  }
}