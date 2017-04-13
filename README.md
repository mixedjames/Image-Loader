Image Loader
============

James' Image Loader is a small C++ library that provides a simple wrapper for using
libpng and libjpeg.

It handles a lot of boilerplate that can otherwise be irritating to get right each time
you want to load an image:
  - Provides a single function to load each image type
  - Throws exceptions on failure
  - Provides a minimal Image RAII type to manage the image data

Legal stuff
-----------
JWT is licensed under the LGPL 2.1.
Copyright &copy; 2017 James Heggie

Building JWT
------------
A few important notes:
1. I am currently working using Visual Studio 2015 so instructions are all relative
   to this version of VS.
2. Image-Loader depends on libpng, and as a consequence also depends on zlib. Fortunately
   both a easy to build on Visual Studio 2015.

### Step 1: download the latest versions
zlib is here: [http://www.zlib.net/]http://www.zlib.net/

libpng is here: [http://www.libpng.org/pub/png/]http://www.libpng.org/pub/png/

To match my build process, decompress both archives to the lib directory within the
Image-Loader project space.

### Step 2: build zlib
Navigate to `*image-loader-root*/lib/zlib/contrib/vstudio/vc14` and open `zlibvc.sln`.

There are many projects within this solution but you only need to build two:
1. ZlibStat debug (I build for x86 but choose whatever architecture you're using)
2. ZlibStat release (or ZlibstatReleaseWithoutASM - it's easier to configure!)

This will create various things but what you need are:
1. `x86/ZlibStatDebug/zlibstat.lib`
2. `x86/ZlibRelease/zlibstat.lib`

I copied them to my `*image-loader-root*/lib/bin` folder, renaming them as follows:
1. `zlib-x86d.lib` (for the debug library)
2. `zlib-x86.lib` (for the release library)

### Step 3: build libpng
Navigate to `*image-loader-root*/lib/libpng/projects/vstudio.`

Open the file `zlib.props` and find the line `<ZLibSrcDir>...</ZLibSrcDir>`.
This path must point to your zlib installation. If you're copying my system that would be
`<ZLibSrcDir>..\..\..\..\zlib</ZLibSrcDir>`.

Next, open `vstudio.sln`, upgrading any projects when prompted. You'll need to build the
following projects (most easily done using the Batch Build feature):
1. libpng debug library
2. libpng release library

This will create two files which need to be copied and renamed as before:
1. `Debug Library/libpng16.lib` --> `/lib/bin/libpng16-x86d.lib`
2. `Release Library/libpng16.lib` --> `/lib/bin/libpng16-x86.lib`

### Step 4: that's it!
If you've copied my file structure then the files in `/vs2015/` should just work. If
you've done your own thing then you'll need to adjust your include and library paths
accordingly.

Basic Usage
-----------
Pending