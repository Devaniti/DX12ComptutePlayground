#pragma once

struct Image
{
    unsigned char* data;
    size_t w, h, ch;
};

Image ReadImage(const char* filename);
void FreeImage(Image& image);

void WriteImage(const char* filename, Image& image);
