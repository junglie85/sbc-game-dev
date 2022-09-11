#pragma once

class MyFiles {
public:
    MyFiles();
    ~MyFiles();
    int height;
    int width;
    int comp;
    char* load(const char* filename, int*, int*);
};
