rem @echo off
rem Call this script to copy the appropriate DLLs into the krepel build
rem directory. Especially useful after a `build.bat clean`.
rem Note: You might want to change the variable `From` below.

setlocal
  set From=D:\Vulkan-LoaderAndValidationLayers
  set Config=Debug

  set BuildDir=%~dp0..\Build

  if not exist "%BuildDir%" (
    echo The target directory does not exist:
    echo %BuildDir%
    exit /b 1
  )

  echo Copying %Config% versions from %From% to %BuildDir%

  pushd "%BuildDir%"
    copy "%From%\build\loader\%Config%\vulkan-1.dll"
    copy "%From%\build\loader\%Config%\vulkan-1.pdb"
    copy "%From%\build\layers\%Config%\VkLayer_core_validation.dll"
    copy "%From%\build\layers\%Config%\VkLayer_core_validation.pdb"
    copy "%From%\build\layers\%Config%\VkLayer_core_validation.json"
    copy "%From%\build\layers\%Config%\VkLayer_device_limits.dll"
    copy "%From%\build\layers\%Config%\VkLayer_device_limits.pdb"
    copy "%From%\build\layers\%Config%\VkLayer_device_limits.json"
    copy "%From%\build\layers\%Config%\VkLayer_image.dll"
    copy "%From%\build\layers\%Config%\VkLayer_image.pdb"
    copy "%From%\build\layers\%Config%\VkLayer_image.json"
    copy "%From%\build\layers\%Config%\VkLayer_object_tracker.dll"
    copy "%From%\build\layers\%Config%\VkLayer_object_tracker.pdb"
    copy "%From%\build\layers\%Config%\VkLayer_object_tracker.json"
    copy "%From%\build\layers\%Config%\VkLayer_parameter_validation.dll"
    copy "%From%\build\layers\%Config%\VkLayer_parameter_validation.pdb"
    copy "%From%\build\layers\%Config%\VkLayer_parameter_validation.json"
    copy "%From%\build\layers\%Config%\VkLayer_swapchain.dll"
    copy "%From%\build\layers\%Config%\VkLayer_swapchain.pdb"
    copy "%From%\build\layers\%Config%\VkLayer_swapchain.json"
    copy "%From%\build\layers\%Config%\VkLayer_threading.dll"
    copy "%From%\build\layers\%Config%\VkLayer_threading.pdb"
    copy "%From%\build\layers\%Config%\VkLayer_threading.json"
    copy "%From%\build\layers\%Config%\VkLayer_unique_objects.dll"
    copy "%From%\build\layers\%Config%\VkLayer_unique_objects.pdb"
    copy "%From%\build\layers\%Config%\VkLayer_unique_objects.json"
  popd

endlocal
