# Java JNI binding for libnfs.
#
# On Android the NDK sysroot ships jni.h directly, so no extra include
# directories are needed. Everywhere else we locate a JDK through CMake's
# FindJNI module.

if(NOT ANDROID)
  find_package(JNI REQUIRED)
endif()

add_library(nfsjni libnfs_jni.c)

target_include_directories(nfsjni PRIVATE ${JNI_INCLUDE_DIRS})
target_link_libraries(nfsjni PRIVATE nfs ${JNI_LIBRARIES})

# Produce a plain "libnfsjni.so" on every platform so that
# System.loadLibrary("nfsjni") always finds it.
set_target_properties(nfsjni PROPERTIES OUTPUT_NAME "nfsjni")

install(TARGETS nfsjni
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Java sources are installed as-is so that an Android/Java project can
# compile them into its own application or an AAR.
install(DIRECTORY java/org
        DESTINATION share/libnfs-jni/java)
