#include "stdafx.h"
#include "ImageWrapper.h"
#include "External/stb_image.h"
#include "External/stb_image_write.h"

Image ReadImage(const char* filename)
{
    Image result;
    int w, h, ch;
    result.data = stbi_load(filename, &w, &h, &ch, 0);
    MyAssert(result.data);
    result.w = w;
    result.h = h;
    result.ch = ch;
    return result;
}

void FreeImage(Image& image)
{
    stbi_image_free(image.data);
}

void WriteImage(const char* filename, Image& image)
{
    CHECKED_CALL(stbi_write_png(filename, image.w, image.h, image.ch, image.data, 0));
}
