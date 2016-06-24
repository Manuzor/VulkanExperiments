"""
Provides general infos and utilities about the current system and the repo.
"""

from pathlib import Path
import json


def VisualStudio2015Path(Env):
  VS140COMNTOOLS_Fallback = Path("C:/", "Program Files (x86)", "Microsoft Visual Studio 14.0", "Common7", "Tools")
  VS140COMNTOOLS = Path(Env.get("VS140COMNTOOLS", VS140COMNTOOLS_Fallback))

  if VS140COMNTOOLS.exists():
    return VS140COMNTOOLS.parent.parent

def Windows10SDKPathAndLatestVersion(Env):
  WindowsSdkDir_Fallback = Path("C:/", "Program Files (x86)", "Windows Kits", "10")
  WindowsSdkDir = Path(Env.get("WindowsSdkDir", WindowsSdkDir_Fallback))

  if not WindowsSdkDir.exists():
    return WindowsSdkDir, None

  # Determine Windows SDK Version
  Windows10SDKVersions = [File.name for File in (WindowsSdkDir / "Include").iterdir() if File.is_dir()]
  if len(Windows10SDKVersions) == 0:
    return WindowsSdkDir, None

  Windows10SDKVersions.sort()

  return WindowsSdkDir, Windows10SDKVersions[-1]

def VulkanSDKPath(Env):
  VulkanSDKPath_Fallback = Path("C:/", "VulkanSDK", "1.0.13.0")
  VulkanSDKPath = Path(Env.get("VULKAN_SDK", Env.get("VK_SDK_PATH", VulkanSDKPath_Fallback)))
  return VulkanSDKPath

def RepoRoot(Env):
  ThisFilePath = Path(__file__)
  UtilitiesPath = ThisFilePath.parent
  return UtilitiesPath.parent

def RepoManifestPath(Env):
  BuildPath = RepoRoot(Env) / "Build"
  ManifestPath = BuildPath / "RepoManifest.json"
  return ManifestPath

def LoadRepoManifestPath(FilePath):
  if FilePath.exists():
    with FilePath.open("r") as JsonFile:
      return json.load(JsonFile)

def SystemBFFPath(Env):
  return RepoRoot(Env) / "Build" / "System.bff"
