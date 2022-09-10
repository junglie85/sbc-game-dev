#include "MyFiles.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

MyFiles::MyFiles() {}

MyFiles::~MyFiles() {}

char* MyFiles::load(const char* filename, int* width, int* height)
{
    unsigned char* data = stbi_load(filename, width, height, &comp, 4); // 4 components RGBA
    return (char*)data;
}
