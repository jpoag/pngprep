# pngprep
PNG alpha preparation tool to fix blending artifacts (outlines) when rendering

## Problem
See http://www.adriancourreges.com/blog/2017/05/09/beware-of-transparent-pixels/

## Solution
https://jet.ro/2009/10/17/tool-pngprep/

When programming visuals, I often need to fix color values of pixels in image data where alpha is 0. Here’s a little tool called pngprep which can do some pre-processing of image data and save the result as a 32 bpp png file.

Supported operations include dilation, where color of alpha>0 pixels are dilated to nearby pixels with alpha=0. This is the default operation. An alternative operation is pre-multiplying colors by alpha, i.e. value of any color component won’t be larger than the alpha value after this operation.

Using either of these operations will “magically” fix many graphical glitches related to drawing images with linear interpolation. Generally using pre-multiplied alpha would be better choice, but using it requires modifications to the source code (different blending mode). That is, it should be known in the source code if image to be drawn has pre-multiplied alpha or not. Just doing the dilation operation won’t need any changes to the code, so it is helpful in many situations.

I’m using stb_image to load the images, so some other input formats should be supported as well in addition to png. However, note that only certain kind of input files are supported, e.g. trying to load 64 bpp (16 bits per color component) png files will lead to a crash. Output files are always normalized to 32 bpp (8 bits per color component) and saved in png format.

Download version 1.0, including source code and a pre-built executable for Windows:
pngprep.zip

Disclaimer: Use it for anything, but do so at your own risk.

#Licences 
This program uses the following libraries:

## STB_Image
https://github.com/nothings/stb/blob/master/stb_image.h

## LibPNG
http://www.libpng.org/pub/png/libpng.html

## zlib
https://www.zlib.net/