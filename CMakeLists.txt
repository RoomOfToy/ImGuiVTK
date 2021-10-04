cmake_minimum_required (VERSION 3.6)
project(ImGuiVTK)

set(CMAKE_CXX_STANDARD 17)

# supposes to use VTK >= 9.0
find_package(VTK COMPONENTS 
  vtkCommonColor
  vtkCommonCore
  vtkCommonDataModel
  vtkCommonSystem
  vtkFiltersExtraction
  vtkFiltersSources
  vtkIOGeometry
  vtkIOLegacy
  vtkIOPLY
  vtkIOXML
  vtkIOImage
  vtkImagingStencil
  vtkInteractionStyle
  vtkRenderingContextOpenGL2
  vtkRenderingCore
  vtkRenderingFreeType
  vtkRenderingGL2PSOpenGL2
  vtkRenderingOpenGL2
  vtkRenderingVolumeOpenGL2
  vtkRenderingAnnotation
  vtkInteractionWidgets
  QUIET
)
message(STATUS "VTK_VERSION: ${VTK_VERSION}")

# glfw
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake_modules")
find_package(GLFW3 REQUIRED)
include_directories(${GLFW3_INCLUDE_DIR})
link_libraries(${GLFW3_LIBRARY})

# opengl
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

# glad
include_directories(
  ${PROJECT_SOURCE_DIR}/include/glad
  ${PROJECT_SOURCE_DIR}/include/KHR
)

# imgui
include_directories(
  ${PROJECT_SOURCE_DIR}/include/imgui
  ${PROJECT_SOURCE_DIR}/include/imgui/backends
)

# imgui-filebrowser
include_directories(
  ${PROJECT_SOURCE_DIR}/include/imgui-filebrowser
)

# custom header
include_directories(
  ${PROJECT_SOURCE_DIR}/include
)

add_executable(ImGuiVTK_test 
  ${PROJECT_SOURCE_DIR}/src/ImGuiVTK_test.cpp
  ${PROJECT_SOURCE_DIR}/src/ImGuiVTK.cpp
  ${PROJECT_SOURCE_DIR}/include/glad/glad.c
  ${PROJECT_SOURCE_DIR}/include/imgui/imgui.cpp
  ${PROJECT_SOURCE_DIR}/include/imgui/imgui_draw.cpp
  ${PROJECT_SOURCE_DIR}/include/imgui/imgui_tables.cpp
  ${PROJECT_SOURCE_DIR}/include/imgui/imgui_widgets.cpp
  ${PROJECT_SOURCE_DIR}/include/imgui/backends/imgui_impl_glfw.cpp
  ${PROJECT_SOURCE_DIR}/include/imgui/backends/imgui_impl_opengl3.cpp)
target_link_libraries (
  ImGuiVTK_test
  ${GLFW3}
  OpenGL::GL
  ${VTK_LIBRARIES}
)
# vtk_module_autoinit is needed
vtk_module_autoinit(
  TARGETS ImGuiVTK_test
  MODULES ${VTK_LIBRARIES}
)