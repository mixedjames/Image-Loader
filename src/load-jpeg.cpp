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

#include <stdio.h>
#include <utility>
#include <vector>
#include <assert.h>
#include <setjmp.h>

extern "C" {
#include <jpeglib.h>
#include <jerror.h>
}

namespace james {

  namespace {

    struct JPEGDecompressionAdapter {
      jpeg_decompress_struct base;
      jpeg_error_mgr err;
      jpeg_source_mgr src;

      jmp_buf errHandler;
      std::istream& stream;
      std::vector<JOCTET> buffer;
      std::exception_ptr currentError;

      JPEGDecompressionAdapter(std::istream& src) : stream(src), buffer(1024) {}
    };

    void InstallErrorHandlers(JPEGDecompressionAdapter& jpeg) {
      jpeg.base.err = jpeg_std_error(&jpeg.err);
      
      jpeg.err.error_exit = [](j_common_ptr cptr) {
        JPEGDecompressionAdapter* jpeg = reinterpret_cast<JPEGDecompressionAdapter*>(cptr);

        longjmp(jpeg->errHandler, 1);
      };
    }

    void InstallIOAdapter(JPEGDecompressionAdapter& jpeg) {
      jpeg.base.src = &jpeg.src;

      jpeg.src.next_input_byte = &jpeg.buffer[0];
      jpeg.src.bytes_in_buffer = 0;

      jpeg.src.init_source = [](j_decompress_ptr) {};
      jpeg.src.term_source = [](j_decompress_ptr) {};

      jpeg.src.fill_input_buffer = [](j_decompress_ptr dptr) -> boolean {
        JPEGDecompressionAdapter* jpeg = (JPEGDecompressionAdapter*) dptr;

        try {
          jpeg->src.bytes_in_buffer = (std::size_t) jpeg->stream.rdbuf()->sgetn(
            (char*)&jpeg->buffer[0], jpeg->buffer.size());
          jpeg->src.next_input_byte = &jpeg->buffer[0];
        }
        catch (...) {
          jpeg->currentError = std::current_exception();
          ERREXIT(dptr, JERR_FILE_READ);
        }
        return TRUE;
      };

      jpeg.src.skip_input_data = [](j_decompress_ptr dptr, long l) {
        JPEGDecompressionAdapter* jpeg = (JPEGDecompressionAdapter*)dptr;
        try {
          jpeg->stream.seekg(l, std::ios::cur);
        }
        catch (...) {
          jpeg->currentError = std::current_exception();
          ERREXIT(dptr, JERR_FILE_READ);
        }
      };

      jpeg.src.resync_to_restart = [](j_decompress_ptr dptr, int desired) -> boolean {
        return jpeg_resync_to_restart(dptr, desired);
      };
    }
  }

  Image LoadJPEG(std::istream& src) {

    JPEGDecompressionAdapter jpeg(src);
    Image img;

    if (setjmp(jpeg.errHandler)) {
      jpeg_destroy_decompress(&jpeg.base);

      if (jpeg.currentError) {
        std::rethrow_exception(jpeg.currentError);
      }
      else {
        throw std::runtime_error("Error decompressing JPEG stream.");
      }
    }

    InstallErrorHandlers(jpeg);

    jpeg_create_decompress(&jpeg.base);

    InstallIOAdapter(jpeg);

    jpeg_read_header(&jpeg.base, true);
    jpeg_start_decompress(&jpeg.base);

    img = Image(jpeg.base.image_width, jpeg.base.image_height, jpeg.base.num_components << 3);

    JSAMPROW rowPtr[1];
    rowPtr[0] = img.Pixels();

    while (jpeg.base.output_scanline < jpeg.base.output_height) {
      jpeg_read_scanlines(&jpeg.base, rowPtr, 1);
      rowPtr[0] += jpeg.base.image_width*jpeg.base.num_components;
    }

    jpeg_finish_decompress(&jpeg.base);
    jpeg_destroy_decompress(&jpeg.base);

    return img;
  }

}