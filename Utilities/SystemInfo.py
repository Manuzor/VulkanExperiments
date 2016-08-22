"""
Provides general infos and utilities about the current system and the repo.
"""

from pathlib import Path
import json
import shutil


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

def VulkanSDKPathAndLatestVersion(Env):
  VulkanSDKPath_Fallback = Path("C:/", "VulkanSDK", "1.0.13.0")
  VulkanSDKPath = Path(Env.get("VULKAN_SDK", Env.get("VK_SDK_PATH", VulkanSDKPath_Fallback)))

  if not VulkanSDKPath.exists():
    return VulkanSDKPath, None

  VulkanSDKVersion = VulkanSDKPath.name
  return VulkanSDKPath, VulkanSDKVersion

def RepoRoot(Env):
  ThisFilePath = Path(__file__)
  UtilitiesPath = ThisFilePath.parent
  return UtilitiesPath.parent

def RepoBuildPath(Env):
  BuildPath = RepoRoot(Env) / "Build"
  return BuildPath

def RepoUtilitiesPath(Env):
  UtilitiesPath = RepoRoot(Env) / "Utilities"
  return UtilitiesPath

def RepoWorkspacePath(Env):
  WorkspacePath = RepoRoot(Env) / "Workspace"
  return WorkspacePath

def RepoManifestPath(Env):
  ManifestPath = RepoWorkspacePath(Env) / "RepoManifest.json"
  return ManifestPath

def LoadRepoManifest(FilePath):
  if FilePath.exists():
    with FilePath.open("r") as JsonFile:
      return json.load(JsonFile)

def FASTBuildPaths(Env):
  """
  Chooses a FASTBuild installation.

  First, the system's PATH is searched for the FBuild executable. If none was
  found, the one in the Utilities folder will be chose as fallback. It is
  assumed that the version in the Utilities folder always exists.
  """
  FallbackPath = RepoUtilitiesPath(Env) / "FASTBuild"
  FBuildFilePath = Path(shutil.which("FBuild"))
  SystemPath = Path()
  if FBuildFilePath.is_file():
    SystemPath = FBuildFilePath.parent.resolve()

  MainPath = SystemPath if SystemPath.is_dir() else FallbackPath
  return MainPath, SystemPath, FallbackPath

def SystemBFFPath(Env):
  BffPath = RepoWorkspacePath(Env) / "System.bff"
  return BffPath

def SublimeText3WorkingDir(Env):
  return RepoWorkspacePath(Env) / "SublimeText3"

def VisualStudioWorkingDir(Env):
  return RepoWorkspacePath(Env) / "VisualStudio"

def CommandLineWorkingDir(Env):
  return RepoWorkspacePath(Env) / "CommandLine"

def BackbonePath(Env):
  Result = Env.get("BACKBONE_REPO_ROOT", None)
  if Result:
    return Path()

  Result = Path.cwd()
  while Result.parent != Result:
    Candidate = Result / "Backbone"
    if Candidate.is_dir():
      return Candidate

    Result = Result.parent

  return Path()
