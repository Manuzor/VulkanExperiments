
from pathlib import Path


def FindVisualStudio2015Path(Env):
  VS140COMNTOOLS_Fallback = Path("C:/", "Program Files (x86)", "Microsoft Visual Studio 14.0", "Common7", "Tools")
  VS140COMNTOOLS = Path(Env.get("VS140COMNTOOLS", VS140COMNTOOLS_Fallback))

  if VS140COMNTOOLS.exists():
    return VS140COMNTOOLS.parent.parent

def FindWindows10SDKPathAndVersion(Env):
  WindowsSdkDir_Fallback = Path("C:/", "Program Files (x86)", "Windows Kits", "10")
  WindowsSdkDir = Path(Env.get("WindowsSdkDir", WindowsSdkDir_Fallback))

  if WindowsSdkDir.exists():
    # Determine Windows SDK Version
    Windows10SDKVersions = [File.name for File in (WindowsSdkDir / "Include").iterdir() if File.is_dir()]
    if len(Windows10SDKVersions) == 0:
      return

    Windows10SDKVersions.sort()
    Windows10SDKVersions.reverse()

    return WindowsSdkDir, Windows10SDKVersions[0]

def FindVulkanSDK(Env):
  VulkanSDKPath = Path(Env.get("VULKAN_SDK", Env.get("VK_SDK_PATH", None)))
  if VulkanSDKPath.exists():
    return VulkanSDKPath
