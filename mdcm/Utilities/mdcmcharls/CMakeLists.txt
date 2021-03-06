if(NOT CHARLS_NAMESPACE)
  set(CHARLS_NAMESPACE "CHARLS")
  set(CHARLS_STANDALONE 1)
endif()

string(TOLOWER ${CHARLS_NAMESPACE} CHARLS_LIBRARY_NAME)

project(${CHARLS_NAMESPACE} CXX)

foreach(p
    CMP0042
    CMP0063)
  if(POLICY ${p})
    cmake_policy(SET ${p} NEW)
  endif()
endforeach()

# CHARLS version number
set(CHARLS_MAJOR_VERSION 2)
set(CHARLS_MINOR_VERSION 0)
set(CHARLS_BUILD_VERSION 0)
set(CHARLS_VERSION "${CHARLS_MAJOR_VERSION}.${CHARLS_MINOR_VERSION}.${CHARLS_BUILD_VERSION}")

add_definitions(-DCHARLS_STATIC)
if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_library(${CHARLS_LIBRARY_NAME}
  interface.cpp  jpegls.cpp  jpegmarkersegment.cpp  jpegstreamreader.cpp  jpegstreamwriter.cpp)

target_compile_features(${CHARLS_LIBRARY_NAME} PRIVATE cxx_nullptr)
set_target_properties(${CHARLS_LIBRARY_NAME} PROPERTIES
                    VERSION ${CHARLS_MAJOR_VERSION}.${CHARLS_MINOR_VERSION}
                    SOVERSION ${CHARLS_MAJOR_VERSION}
                    ${CHARLS_LIBRARY_PROPERTIES})


