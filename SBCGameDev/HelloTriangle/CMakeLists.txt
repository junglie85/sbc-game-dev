list(APPEND IncludeDirs ${CMAKE_SYSROOT}/usr/include/EGL ${CMAKE_SYSROOT}/usr/include/GLES2)
list(APPEND PublicLibs GLESv2 EGL X11)
list(APPEND PrivateLibs pthread dl xcb GLdispatch Xau Xdmcp bsd md)

set(LIBRARIES_FROM_REFERENCES "")
add_executable(HelloTriangle HelloTriangle.cpp)
target_include_directories(HelloTriangle PUBLIC ${IncludeDirs} .)
target_link_libraries(HelloTriangle  PUBLIC "${LIBRARIES_FROM_REFERENCES}" ${PublicLibs} PRIVATE ${PrivateLibs})
