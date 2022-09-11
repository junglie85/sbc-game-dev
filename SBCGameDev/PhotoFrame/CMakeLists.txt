list(APPEND IncludeDirs ${CMAKE_SYSROOT}/usr/include/EGL ${CMAKE_SYSROOT}/usr/include/GLES2)
list(APPEND PublicLibs GLESv2 EGL X11)
list(APPEND PrivateLibs pthread dl xcb GLdispatch Xau Xdmcp bsd md)

set(LIBRARIES_FROM_REFERENCES "")
add_executable(PhotoFrame PhotoFrame.cpp MyFiles.cpp MyFiles.h)
target_include_directories(PhotoFrame PUBLIC ${IncludeDirs} .)
target_link_libraries(PhotoFrame  PUBLIC "${LIBRARIES_FROM_REFERENCES}" ${PublicLibs} PRIVATE ${PrivateLibs} stb)