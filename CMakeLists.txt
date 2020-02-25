#
# Copyright (c) 2019 Arnaud Loonstra.
#
# License MPLv2
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

cmake_minimum_required(VERSION 3.1)
project(gazebosc)

set(CMAKE_CXX_STANDARD 14)
set(DEP_IMGUI_VERSION "1.72b" CACHE STRING "ImGui version to use")
set(DEP_IMNODES_DIR ".")

add_subdirectory("ext/czmq" EXCLUDE_FROM_ALL)
# overrides to prevent the Findczmq.cmake script to fail
set(CZMQ_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/ext/czmq/)
set(CZMQ_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext/czmq/include/)
add_subdirectory("ext/libsphactor" EXCLUDE_FROM_ALL)
add_subdirectory("ext/SDL-mirror" EXCLUDE_FROM_ALL)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
pkg_check_modules(LO REQUIRED liblo)

find_package (Python3 REQUIRED COMPONENTS Development)
if(Python3_FOUND)
    message ("python3 found")
    message (${Python3_LIBRARIES})
    add_definitions(-DPYTHON3_FOUND)
else()
    message (FATAL_ERROR "Cannot find Python3")
endif()

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    # Build samples and pull in engine dependency only when this library is built
    # independently. These are not necessary when user is consuming library in their
    # own project.
    if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}.zip")
        file(DOWNLOAD "https://github.com/ocornut/imgui/archive/v${DEP_IMGUI_VERSION}.zip" ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}.zip)
    endif ()
    if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}.zip)
    endif ()
endif ()

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Actors ACTOR_SOURCES)

if (UNIX)
    link_directories(
        /usr/local/lib/
    )
    add_executable(gazebosc 
       	main.cpp

        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/imgui_impl_opengl3.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/imgui_impl_opengl3.h
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/imgui_impl_sdl.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/imgui_impl_sdl.h
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imconfig.h
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imgui.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imgui.h
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imgui_draw.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imgui_widgets.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imgui_internal.h
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imstb_rectpack.h
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imstb_textedit.h
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imstb_truetype.h
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/libs/gl3w/GL/gl3w.c
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/libs/gl3w/GL/gl3w.h
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/libs/gl3w/GL/glcorearb.h
        ${DEP_IMNODES_DIR}/ImNodes.h
        ${DEP_IMNODES_DIR}/ImNodes.cpp
        ${DEP_IMNODES_DIR}/ImNodesEz.h
        ${DEP_IMNODES_DIR}/ImNodesEz.cpp

	RtMidi.h
        RtMidi.cpp
	rtmidi_c.h
        rtmidi_c.cpp
	
        stage.cpp

	GActor.h
	GActor.cpp
        ${ACTOR_SOURCES}
    )
    target_include_directories(gazebosc PUBLIC
        ${Python3_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples
        ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/libs/gl3w
        ${DEP_IMNODES_DIR}
    )
    if (APPLE)
        target_link_libraries(gazebosc PUBLIC 
            SDL2-static 
            czmq-static 
            sphactor-static 
            ${ZMQ_LIBRARIES} 
            ${LO_LIBRARIES} 
            ${Python3_LIBRARIES} 
            -ldl "-framework OpenGL" "-framework CoreFoundation" "-framework CoreMidi" "-framework CoreAudio")
	add_definitions(-D__MACOSX_CORE__)
    else()
        target_link_libraries(gazebosc PUBLIC 
            SDL2-static
            czmq-static
            sphactor-static
            ${ZMQ_LIBRARIES}
            ${LO_LIBRARIES}
            ${Python3_LIBRARIES}
            -lasound -ldl -lGL -lGLEW)
        target_compile_options(gazebosc PUBLIC -D__LINUX_ALSA__ )
    endif()
    target_compile_options(gazebosc PUBLIC ${SDL2_CFLAGS_OTHER} )
    target_compile_definitions(gazebosc PUBLIC -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1)
endif ()
add_custom_command(
    TARGET gazebosc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/tester.py
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tester.py)

