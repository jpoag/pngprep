/* pngprep
 * Copyright 2009 Jetro Lauha
 *
 * Performs pre-processing for alpha image files. See tool usage instructions
 * for more info. Some other input formats are supported as well in addition
 * to png, supplied by stb_image. However, note that only certain kind of
 * png files are supported, e.g. trying to load 64 bpp (16 bits per color
 * component) png files will lead to a crash with the current version of
 * stb_image. Output files are always normalized to 32 bpp (8 bits per
 * color component) and saved in png format.
 *
 * My home page / blog URL:
 *   http://jet.ro
 *
 * URL for stb_image:
 *   http://nothings.org/stb_image.c
 *
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * $Id: pngprep.cpp 85 2009-10-17 19:21:34Z tonic $
 * $Revision: 85 $
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "png.h"

#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"


#ifdef _WIN32
#define strcasecmp _stricmp
#endif


void png_error_fn(png_structp png_ptr, png_const_charp msg)
{
    fprintf(stderr, "libpng ERROR: %s\n", msg);
    exit(EXIT_FAILURE);
}

void png_warn_fn(png_structp png_ptr, png_const_charp msg)
{
    fprintf(stderr, "libpng warning: %s\n", msg);
}


bool write_png(const char *resultFile, int width, int height, unsigned int *imagedata)
{
    FILE *fp = fopen(resultFile, "wb");
    if (fp == 0)
        return false;

    png_structp png_ptr = 0;
    png_infop info_ptr = 0;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, png_error_fn, png_warn_fn);
    if (png_ptr == 0)
    {
        fclose(fp);
        return false;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == 0)
    {
        fclose(fp);
        return false;
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    //png_set_bgr(png_ptr); // little-endian

    for (int y = 0; y < height; ++y)
    {
        png_write_row(png_ptr, (png_bytep)imagedata);
        imagedata += width;
    }

    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(fp);

    return true;
}


void dilate(unsigned int *pixels, int w, int h)
{
    // 1st pass: blank color of all pixels with alpha 0
    int offs = 0;
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x, ++offs)
        {
            int a = (pixels[offs] & 0xff000000) >> 24;
            int r = (pixels[offs] & 0x00ff0000) >> 16;
            int g = (pixels[offs] & 0x0000ff00) >> 8;
            int b = (pixels[offs] & 0x000000ff);
            if (a == 0)
                pixels[offs] = 0;
        }
    }

    // 2nd pass: dilate color to pixels with alpha=0 but having neighbors with alpha>0
    offs = 0;
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            if ((pixels[offs] & 0xff000000) == 0)
            {
                // iterate through 3x3 window around pixel
                int left = x - 1, right = x + 1, top = y - 1, bottom = y + 1;
                if (left < 0) left = 0;
                if (right >= w) right = w - 1;
                if (top < 0) top = 0;
                if (bottom >= h) bottom = h - 1;
                int x2, y2, colors = 0, offs2 = top * w + left;
                int red = 0, green = 0, blue = 0;
                for (y2 = top; y2 <= bottom; ++y2)
                {
                    for (x2 = left; x2 <= right; ++x2)
                    {
                        unsigned int px = pixels[offs2++];
                        if (px & 0xff000000)
                        {
                            red += (px & 0x00ff0000) >> 16;
                            green += (px & 0x0000ff00) >> 8;
                            blue += (px & 0x000000ff);
                            ++colors;
                        }
                    }
                    offs2 += w - (right - left + 1);
                }
                if (colors > 0)
                {
                    // replace color bytes with average from neighbors
                    pixels[offs] &= 0xff000000;
                    pixels[offs] |= (red / colors) << 16;
                    pixels[offs] |= (green / colors) << 8;
                    pixels[offs] |= (blue / colors);
                }
            }
            ++offs;
        }
    }

}


void premul(unsigned int *pixels, int w, int h)
{
    int offs = 0;
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x, ++offs)
        {
            int a = (pixels[offs] & 0xff000000) >> 24;
            int r = (pixels[offs] & 0x00ff0000) >> 16;
            int g = (pixels[offs] & 0x0000ff00) >> 8;
            int b = (pixels[offs] & 0x000000ff);
            r = r * a / 255;
            g = g * a / 255;
            b = b * a / 255;
            pixels[offs] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
}


enum OP
{
    OP_DILATE = 0,
    OP_PREMUL = 1,
    OP_COUNT //
};

const char *opNames[OP_COUNT] =
{
    "dilate",
    "premul"
};

int main(int argc, char *argv[])
{
    int op = OP_DILATE;
    const char *infile, *outfile;

    printf("pngprep v1.0.1\n");

    while (argc >= 2 && argv[1][0] == '-')
    {
        if (strcmp(argv[1], "-premul") == 0)
        {
            op = OP_PREMUL;
            --argc;
            ++argv;
        }
    }

    if (argc < 3)
    {
        printf("\nUsage: pngprep [-premul] input.png output.png\n\n");
        printf("Processes each png file and saves the processed version.\n");
        printf("The default operation is to dilate color from alpha>0 pixels\n");
        printf("to nearby pixels with alpha=0. Using -premul option changes\n");
        printf("the operation to pre-multiply colors by alpha instead.\n\n");
        printf("Warning: Destination files are overwritten!\n\n");
        printf("    -premul    pre-multiply colors by alpha instead of dilating\n\n");
        return EXIT_SUCCESS;
    }

    infile = argv[1];
    outfile = argv[2];

    bool filenamesMatch = false;
    #ifdef _WIN32
    if (strcasecmp(infile, outfile) == 0)
        filenamesMatch = true;
    #else
    if (strcmp(infile, outfile) == 0)
        filenamesMatch = true;
    #endif
    if (filenamesMatch)
    {
        fprintf(stderr, "Error: In-place conversion is not supported.\n");
        return EXIT_FAILURE;
    }

    int w, h, components;
    unsigned char *imgData = stbi_load(infile, &w, &h, &components, 0);
    if (components == 1 || components == 3)
        printf("Warning: image has no alpha channel.\n");
    if (components == 2)
        printf("Warning: 16 bpp grayscale with alpha images are converted to 32 bpp.\n");


    if (w <= 0 || h <= 0)
    {
        fprintf(stderr, "Error: invalid image size (%d,%d)\n", w, h);
        stbi_image_free(imgData);
        return EXIT_FAILURE;
    }

    /*
    int offs = 0;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            char tmp, *pb = (char *)&imagedata[offs++];
            // swap RB
            tmp = pb[0];
            pb[0] = pb[2];
            pb[2] = tmp;
        }
    }
    */
    switch (op)
    {
    case OP_DILATE:
        dilate((unsigned int *)imgData, w, h);
        break;
    case OP_PREMUL:
        premul((unsigned int *)imgData, w, h);
        break;
    default:
        fprintf(stderr, "Error: invalid operation (%d)\n", op);
    }

    bool success = write_png(outfile, w, h, (unsigned int *)imgData);

    if (!success)
        fprintf(stderr, "Error saving image to file %s\n", outfile);

    stbi_image_free(imgData);

    printf("%s processed (%s) and saved as %s\n", infile, opNames[op], outfile);

    return EXIT_SUCCESS;
}
