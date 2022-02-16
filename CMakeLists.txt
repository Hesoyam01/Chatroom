cmake_minimum_required(VERSION 3.4)
project(chatroom)
include(ExternalProject)

ExternalProject_Add(
  pahomqttc
  GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.c.git
  GIT_TAG v1.3.0 
  CMAKE_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  UPDATE_COMMAND ""
  )

ExternalProject_Get_Property(pahomqttc install_dir)

INCLUDE_DIRECTORIES(
  .
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_INSTALL_PREFIX}/include
  ${install_dir}/include
)

  # There are multiple libmqtt versions
  # libmqttv3a.so - asynchronous
  # libmqttv3as.so - asynchronous with SSL
  # libmqttv3c.so - "classic" / synchronous
  # libmqttv3cs.so - "classic" / synchronous with SSL

if (WIN32)
  ADD_DEFINITIONS(/DCMAKE_BUILD /D_CRT_SECURE_NO_DEPRECATE)
  configure_file(${CMAKE_INSTALL_PREFIX}/lib/paho-mqtt3a.dll ${CMAKE_CURRENT_BINARY_DIR}/paho-mqtt3a.dll COPYONLY)
  if (CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND libs ${CMAKE_INSTALL_PREFIX}/lib/paho-mqtt3a.lib)
  else()
    list(APPEND libs ${CMAKE_INSTALL_PREFIX}/lib/paho-mqtt3a.lib)
  endif()
endif()

if (UNIX)
  list(APPEND libs
    # ${CMAKE_INSTALL_PREFIX}/lib/libpaho-mqttv3c.so
    ${install_dir}/lib/libpaho-mqtt3a.so
    #-lz
  )
endif()

add_executable(chatroom src/main.c)
install(TARGETS DESTINATION bin)
add_dependencies(chatroom pahomqttc)
TARGET_LINK_LIBRARIES(chatroom ${libs})