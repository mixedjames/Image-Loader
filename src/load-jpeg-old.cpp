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

extern "C" {
#include <jpeglib.h>
#include <jerror.h>
}

namespace james {

  namespace {

    struct SourceAdapter
      : private jpeg_source_mgr
    {

      SourceAdapter(jpeg_decompress_struct& jpeg, std::istream& src)
        : jpeg_(jpeg), src_(src), buffer_(1024)
      {
        this->next_input_byte = &buffer_[0];
        this->bytes_in_buffer = 0;

        this->init_source = [](j_decompress_ptr jpeg) {
          static_cast<SourceAdapter*>(jpeg->src)->InitSource();
        };
        this->fill_input_buffer = [](j_decompress_ptr jpeg) -> boolean {
          return static_cast<SourceAdapter*>(jpeg->src)->FillInputBuffer();
        };
        this->skip_input_data = [](j_decompress_ptr jpeg, long length) {
          static_cast<SourceAdapter*>(jpeg->src)->SkipInputData(length);
        };
        this->resync_to_restart = [](j_decompress_ptr jpeg, int desired) -> boolean {
          return static_cast<SourceAdapter*>(jpeg->src)->ResyncToRestart(desired);
        };
        this->term_source = [](j_decompress_ptr jpeg) {
          static_cast<SourceAdapter*>(jpeg->src)->TermSource();
        };
      }

      jpeg_source_mgr JPEGSourceManager() { return *this; }

    private:
      jpeg_decompress_struct& jpeg_;
      std::istream& src_;
      std::vector<JOCTET> buffer_;
      std::exception_ptr currentError_;

      void InitSource() {
        // Does nothing
      }

      boolean FillInputBuffer() {
        try {
          bytes_in_buffer = (std::size_t) src_.rdbuf()->sgetn((char*)&buffer_[0], buffer_.size());

          if (bytes_in_buffer == 0) {
            throw std::runtime_error("Unexpected end of file");
          }
          return TRUE;
        }
        catch (...) {
          currentError_ = std::current_exception();
          ERREXIT(&jpeg_, JWRN_JPEG_EOF);
          return TRUE;
        }
      }

      void SkipInputData(long length) {
        try {
          if (length > 0) {
            src_.seekg(length, std::ios::cur);
          }
        }
        catch (...) {
          currentError_ = std::current_exception();
          ERREXIT(&jpeg_, JWRN_JPEG_EOF);
        }
      }

      boolean ResyncToRestart(int desired) {
        return jpeg_resync_to_restart(&jpeg_, desired);
      }

      void TermSource() {
        // Do nothing
      }
    };

    void bomb(j_common_ptr info) {
      assert(false);
    }
  }

  Image LoadJPEG(std::istream& src) {

    jpeg_error_mgr errMgr;
    jpeg_decompress_struct jpeg;
    SourceAdapter srcAdapter(jpeg, src);
    Image img;

    jpeg.err = jpeg_std_error(&errMgr);
    errMgr.error_exit = bomb;

    jpeg_create_decompress(&jpeg);

    jpeg.src = &srcAdapter.JPEGSourceManager();

    jpeg_read_header(&jpeg, true);

    return img;
  }

}