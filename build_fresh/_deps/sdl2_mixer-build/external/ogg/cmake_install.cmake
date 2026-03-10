# Install script for directory: C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/EngineDemo")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-build/external/ogg/ogg.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-build/external/ogg/ogg-0.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-build/external/ogg/ogg.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/SDL2_mixer/html" TYPE FILE FILES
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/framing.html"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/index.html"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/oggstream.html"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/ogg-multiplex.html"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/fish_xiph_org.png"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/multiplex1.png"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/packets.png"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/pages.png"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/stream.png"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/vorbisword2.png"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/white-ogg.png"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/white-xifish.png"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/rfc3533.txt"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/rfc5334.txt"
    "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/skeleton.html"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/SDL2_mixer/html" TYPE DIRECTORY FILES "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-src/external/ogg/doc/libogg")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/Users/UserName/Dev/HellEngine/build_fresh/_deps/sdl2_mixer-build/external/ogg/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
