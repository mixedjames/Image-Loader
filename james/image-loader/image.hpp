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
#pragma once

namespace james {

  /**
   * An Image represents a 2d raster of colour data.
   *
   * ### Properties
   * An Image is specified by:
   * - A width & height (immutable; specified in pixels)
   * - A colour depth (immutable; specified as bits per pixels)
   * - An array of pixels (mutable if you have a non-const object)
   *
   * ### Supported values
   * Dimensions are limited only by the maximum allowable value of unsigned int &
   * by available memory.
   *
   * Bit depth may only be 8, 24 or 32. (8 being a monochrome image, 32 featuring an
   * 8bit alpha channel) This is enforced in the debug version by assertions & by
   * throwing a std::invalid_argument exception in the release version. (See Exception
   * safety section for rationale)
   *
   * It is assumed but not enforced that the pixel format is RGBA.
   *
   * ### Copying & inheritance behaviour
   * Image is designed as a **value type**:
   * - It implements both copy and move semantics
   * - It **should not** be used as a base class (& it **does not** provide a virtual
   *   destructor, nor virtual methods, nor protected data)
   *
   * ### Packing
   * Image assumes its pixel data is tightly packed.
   * - No padding between pixels (i.e. for 24bpp, pixels are *not* dword aligned)
   * - No padding between rows (i.e. for 24bpp, rows *might not* be dword aligned either)
   *
   * I mention this mainly because it makes RGB instances of james::Image fundamentally
   * incompatible with Windows bitmap functions (i.e. `StretchDIBits`) as these require
   * rows (but not pixels) be *word* aligned.
   *
   * ### Exception safety
   * Two guarentees are provided by james::Image:
   * 1. No-throw guarantee (marked by noexcept)
   * 2. Strong guarantee (everywhere else)
   *
   * In general, james::Image throws very few exceptions however, the out-of-memory
   * condition is reported in this way.
   *
   * A distinction is made between logic errors (those caused by an incorrect use of the
   * API) and runtime errors, (those caused by a malfunctioning system or by an unexpected
   * system state) and these are handled slightly differently:
   * - **Logic errors:** in debug mode this will trigger assertion failures. This prevents
   *   sloppy clients from catching & silently ignoring these errors. In release mode they
   *   will be reported by throwing a std::logic_error (or subclass thereof).
   * - **Runtime errors:** reported by throwing exceptions. In general these are either
   *   std::exception or subclasses thereof. (We cannot say std::runtime_error because
   *   std::bad_alloc is not a subclass of this)
   *
   * ### Thread safety
   * `james::Image` is not thread safe.
   */
  struct Image {
    Image();
    Image(const Image&);
    Image(Image&&) noexcept;

    Image(unsigned int w, unsigned int h, unsigned int bpp);

    ~Image() noexcept;

    Image& operator= (const Image&);
    Image& operator= (Image&&) noexcept;

    void Swap(Image&) noexcept;

    unsigned int Width() const noexcept { return w_; }
    unsigned int Height() const noexcept { return h_; }
    unsigned int BitsPerPixel() const noexcept { return bpp_; }

    const unsigned char* Pixels() const noexcept { return pixels_; }
    unsigned char* Pixels() noexcept { return pixels_; }

  private:
    unsigned int w_, h_, bpp_;
    unsigned char* pixels_;
  };

  inline std::size_t ByteSize(const Image& img) {
    return img.Width()*img.Height()*(img.BitsPerPixel() >> 3);
  }
}