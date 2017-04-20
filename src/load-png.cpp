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
#include <algorithm>
#include <png.h>

#include <iostream>

namespace james {

  namespace {

    // PNGDataMgr is a RAII type for managing the two key structures used in libPNG:
    // --> png_structp & png_infop
    //
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

    // PNGLoaderState bundles up all the data needed for a LoadPNG call so that it can
    // be accessible to callback functions that get a single data pointer for context.
    //
    struct PNGLoaderState {
      PNGDataMgr libPNG;
      
      Image img;

      std::istream& src;
      std::ios::iostate streamExceptionState;

      jmp_buf jmpBuf;
      std::exception_ptr currentError;

      PNGLoaderState(std::istream& src)
        : src(src), streamExceptionState(src.exceptions())
      {
        // Note: this might (???) be able to throw an exception. This is safe because
        //       PNGLoaderState does not directly own any raw resources - they are all
        //       wrapped up in manager objects.
        src.exceptions(std::istream::failbit | std::istream::badbit | std::istream::eofbit);
      }

      ~PNGLoaderState() {
        // Note: this could potentially throw an exceptions. I'm pretty sure it wont, because
        //       we are only ever making the exception behaviour *less* likely to throw,
        //       but none-the-less, having potentially throwing code in a destructor
        //       makes me uneasy.
        //
        //       At the moment we just handle this by having it as the last action in the
        //       destructor (so any clean-up will definitely happen) & then propagate
        //       the exception.
        //
        src.exceptions(streamExceptionState);
      }
    };

    // Setup our std::istream adapter callback
    //
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
    // LoadPNG has a specific order of operation...
    // (1) Allocate PNGLoaderState - we now have the PNG data structures needed
    // (2) Call setjmp to setup error handling (pretty much any libPNG call can
    //     fail with a longjmp but NOT create_read/info_struct - unlike libJPEG)
    // (3) Install the std::istream IO adapter
    // (4) Read the whole image into memory & decompress
    // (5) Allocate the iImage (using the newly known image dimensions) and 
    //     copy the decompressed data into it

    PNGLoaderState state(src);

    png_uint_32 w;
    png_uint_32 h;
    int channelWidth;
    int nChannels;

    png_bytepp rowPtrs = nullptr;
    unsigned char* dstPtr = nullptr;

    if (setjmp(state.jmpBuf)) {
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

    // I *think* that the combination of transform flags passed to png_read_png means that we should
    // only ever get 8 bit channels with 3 or 4 components per pixel, however, I'm not quite sure so
    // in the spirit of "belt and braces" we check and throw anyway... (since if I'm wrong we would
    // have a buffer overrun which is a mjor security cock up)

    if (channelWidth != 8) {
      throw std::runtime_error("PNG channel width was not 8. Only 8 is supported.");
    }

    if (nChannels != 3 && nChannels != 4) {
      throw std::runtime_error("Number of PNG colour channels was neither 3 nor 4.");
    }
      
    state.img = Image(w, h, nChannels << 3);
    
    rowPtrs = png_get_rows(state.libPNG.png, state.libPNG.info);
    dstPtr = state.img.Pixels();

    while (h --) {
      memcpy(dstPtr, *rowPtrs, std::min(w*nChannels, ByteSize(state.img)));

      dstPtr += w*nChannels;
      rowPtrs++;
    }

    return state.img;
  }
}