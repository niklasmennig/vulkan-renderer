# Educational Realtime Vulkan Renderer
This is the renderer created for my master's thesis "Empowering Realtime Rendering Education: Building an Interactive Tool for Exploring Modern Rendering Algorithms".

## Acknowledgements
Scene assets (.gltf and .hdr files) are taken from Khronos' GLTF sample models repository (https://github.com/KhronosGroup/glTF-Sample-Models) and PolyHaven (https://polyhaven.com/).

## Dependencies
This project uses GLFW3 (https://www.glfw.org/) to create a window surface and to interface with Vulkan.
Intel's OpenImageDenoise (https://www.openimagedenoise.org/) can be optionally linked to provide realtime denoise filters to the renderer.

## Building
The build system used for this project is CMake (https://cmake.org/).
To build this project, the CMake variables `GLFW3_INCLUDE_DIR` and `GLFW3_LIBRARY` to the directory containing the GLFW headers and to the file *glfw3.lib* respectively.

To link with OIDN, activate the CMake option `LINK_OIDN` and provide `OpenImageDenoise_INCLUDE_DIR` and `OpenImageDenoise_LIBRARY` accordingly.

## Running
To start the renderer, execute the *renderer* executable and provide a scene file to be rendererd.
For example:
> renderer.exe scenes/sponza_sun.toml

When creating a screenshot, it is saved as *screenshot.exr* and can be viewed in an exr-compatible viewer such as *tev* (https://github.com/Tom94/tev)
