
import os
import sys
from pathlib import *
import argparse

import SystemInfo

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
VS2015Path = SystemInfo.FindVisualStudio2015Path(os.environ)
if VS2015Path:
  Vars["VS2015Path"] = VS2015Path
  Defines.append("USE_VS2015")


#
# Windows SDK Paths
#
Windows10SDKPath, Windows10SDKVersion = SystemInfo.FindWindows10SDKPathAndVersion(os.environ)
Vars["Windows10SDKPath"] = Windows10SDKPath
Vars["Windows10SDKVersion"] = Windows10SDKVersion


#
# Vulkan SDK Path
#
VulkanSDKPath = SystemInfo.FindVulkanSDK(os.environ)
if VulkanSDKPath:
  Vars["VulkanSDKPath"] = VulkanSDKPath


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
