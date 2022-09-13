list(APPEND IncludeDirs ${CMAKE_SYSROOT}/usr/include/EGL ${CMAKE_SYSROOT}/usr/include/GLES2)
list(APPEND PublicLibs GLESv2 EGL X11)
list(APPEND PrivateLibs pthread dl xcb GLdispatch Xau Xdmcp bsd md)

set(LIBRARIES_FROM_REFERENCES "")
add_executable(DoubleBuffer DoubleBuffer.cpp Input.cpp Input.h MyFiles.cpp MyFiles.h Surface.cpp Surface.h)
target_include_directories(DoubleBuffer PUBLIC ${IncludeDirs} .)
target_link_libraries(DoubleBuffer  PUBLIC "${LIBRARIES_FROM_REFERENCES}" ${PublicLibs} PRIVATE ${PrivateLibs} stb)