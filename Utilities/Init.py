
import os
import sys
from pathlib import *
import argparse

#
# Script Argument Parsing
#

Parser = argparse.ArgumentParser(description='Initial setup script.')
Parser.add_argument('reporoot', type=Path,
                    help='The path to the repository root.')
Parser.add_argument('--outbff', type=Path,
                    help='The path to the resulting bff file (not dir).')

Args = Parser.parse_args()


#
# Preparation
#

RepoRoot = Args.reporoot.resolve()
SystemBFF = Args.outbff
BuildPath = RepoRoot / "Build"

if not BuildPath.exists():
  BuildPath.mkdir(parents=True)

if not SystemBFF.parent.exists():
  SystemBFF.parent.mkdir(parents=True)


Defines = []
Vars = {}

Vars["RepoRoot"] = RepoRoot

#
# Visual Studio Paths
#

VS140COMNTOOLS_Fallback = Path("C:/", "Program Files (x86)", "Microsoft Visual Studio 14.0", "Common7", "Tools")
VS140COMNTOOLS = Path(os.environ.get("VS140COMNTOOLS", VS140COMNTOOLS_Fallback))

if VS140COMNTOOLS.exists():
  Vars["VS2015Path"] = VS140COMNTOOLS.parent.parent
  Defines.append("USE_VS2015")

#
# Windows SDK Paths
#
WindowsSdkDir_Fallback = Path("C:/", "Program Files (x86)", "Windows Kits", "10")
WindowsSdkDir = Path(os.environ.get("WindowsSdkDir", WindowsSdkDir_Fallback))

assert WindowsSdkDir.exists(), "Unable to find Windows 10 SDK."
Vars["Windows10SDKPath"] = Path(WindowsSdkDir)

# Determine Windows SDK Version
Windows10SDKVersions = [File.name for File in (WindowsSdkDir / "Include").iterdir() if File.is_dir()]
assert len(Windows10SDKVersions) > 0, "Unable to find Windows 10 SDK installation"
Windows10SDKVersions.sort()
Windows10SDKVersions.reverse()

Vars["Windows10SDKVersion"] = Windows10SDKVersions[0]

#
# Vulkan SDK Path
#

VULKAN_SDK = Path(os.environ.get("VULKAN_SDK", os.environ.get("VK_SDK_PATH", None)))
if VULKAN_SDK is not None:
  Vars["VulkanSDK"] = VULKAN_SDK

#
# Write The File
#

with SystemBFF.open("w") as OutFile:
  def Write(*Args, **KwArgs):
    print(*Args, **KwArgs, file=OutFile)
  Write("#once")

  Write("")
  for Key, Value in Vars.items():
    Write(".{} = '{}'".format(Key, Value))

  Write("")
  for Define in Defines:
    Write("#define {}".format(Define))
