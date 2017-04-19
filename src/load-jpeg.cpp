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
      std::ios::iostate streamExceptionState;
      std::vector<JOCTET> buffer;
      std::exception_ptr currentError;

      JPEGDecompressionAdapter(std::istream& src)
        : base(), stream(src), streamExceptionState(src.exceptions()), buffer(1024)
      {
        // Note 1: we *must not* call jpeg_create_decompress here. See loadJPEG for why

        // Note 2: this might (???) be able to throw an exception. This is safe because
        //       although JPEGDecompressionAdapter does directly own a resource (jpeg_decompress_struct)
        //       this is not initialised until later (see note 1) so no specific cleanup is
        //       needed in the exceptional case.
        src.exceptions(std::istream::failbit | std::istream::badbit | std::istream::eofbit);
      }

      ~JPEGDecompressionAdapter() {
        jpeg_destroy_decompress(&base);

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

    // Install a custom error hander that returns to our main control code so that
    // correct clean-up and propagation of the error can occur.
    //
    void InstallErrorHandlers(JPEGDecompressionAdapter& jpeg) {
      jpeg.base.err = jpeg_std_error(&jpeg.err);
      
      jpeg.err.error_exit = [](j_common_ptr cptr) {
        JPEGDecompressionAdapter* jpeg = reinterpret_cast<JPEGDecompressionAdapter*>(cptr);
        longjmp(jpeg->errHandler, 1);
      };
    }

    // Configures a custom data source based on a std::istream to feed the JPEG
    // decompressor.
    //
    void InstallIOAdapter(JPEGDecompressionAdapter& jpeg) {
      jpeg.base.src = &jpeg.src;

      // Setting bytes_in_buffer = 0 means that an immediate call to fill_input_buffer
      // is generated when decompression is attempted. 
      jpeg.src.next_input_byte = nullptr;
      jpeg.src.bytes_in_buffer = 0;

      // We do init & clean-up elsewhere in the system so these functions
      // are no-op.
      jpeg.src.init_source = [](j_decompress_ptr) {};
      jpeg.src.term_source = [](j_decompress_ptr) {};

      // According to my reading of the libjpeg spec, the standard function should
      // do what we want so I don't fiddle with this.
      jpeg.src.resync_to_restart = jpeg_resync_to_restart;

      // fill_input_buffer & skip_input_data are where the action is...
      //
      // They both go to great lengths to correctly adapt exceptions. Basic idea is:
      // (1) Perform *all* actions that could throw within a try/catch
      // (2) Store the current exception within our JPEGDecompressionAdapter struct.
      // (3) Call libjpeg error handling using the ERREXIT macro. (This function 
      //     never returns) We then trust our custom error_exit handler to do the
      //     right thing.

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

    }
  }

  Image LoadJPEG(std::istream& src) {

    // Sequence of actions is important here:
    // (1) Call setjmp; must be first because error handler setup
    //     depends on having a valid jmpbuf
    // (2) Install error handling code; must be next because *ANY* calls
    //     into libjpeg can potentially cause errors, including jpeg_create_decompress
    // (3) Create the decompressor (jpeg_create_decompress)
    // (4) Install the std::istream adapter code (requires a valid decompressor)
    // (5) Read the JPEG header etc. (requires a valid data stream & decompressor)
    // (6) Create the james::Image (can only happen after decompression has started
    //     so that we know the dimensions)
    // (7) Finally we be do the decompression & clean up
    //
    // Throwing exceptions within the body of LoadJPEG is fine because this will
    // just trigger the destructor of JPEGDecompressionAdapter which will call
    // jpeg_destroy_decompress and reset the stream.

    JPEGDecompressionAdapter jpeg(src);
    Image img;

    if (setjmp(jpeg.errHandler)) {
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

    // Remember: DON'T call jpeg_destroy_decompress; the destructor of
    // JPEGDecompressionAdapter will do that for us.

    return img;
  }

}