"""
Generates the .bff file required for the build system to work.
"""

import os
import sys
from pathlib import *
from datetime import datetime

import SystemInfo

def Main():
  ManifestPath = SystemInfo.RepoManifestPath(os.environ)
  Manifest = SystemInfo.LoadRepoManifest(ManifestPath)
  if Manifest is None:
    print("Unable to load repo manifest: ", str(ManifestPath), file=sys.stderr)
    print("Maybe re-run the init script?", file=sys.stderr)


  #
  # Preparation
  #
  OutFilePath = SystemInfo.SystemBFFPath(os.environ)
  if not OutFilePath.parent.exists():
    OutFilePath.parent.mkdir(parents=True)

  RepoRoot = Path(Manifest["Repo"]["Path"])
  Windows10SDKPath = Path(Manifest["Windows10SDK"]["Path"])
  Windows10SDKVersion = Manifest["Windows10SDK"]["Version"]
  VS2015Path = Path(Manifest["VS2015"]["Path"])
  VulkanSDKPath = Path(Manifest["VulkanSDK"]["Path"])
  VulkanSDKVersion = Manifest["VulkanSDK"]["Version"]

  assert Windows10SDKPath and Windows10SDKPath.exists()
  assert Windows10SDKVersion


  #
  # Write The System.bff File
  #
  with OutFilePath.open("w") as OutFile:
    def Write(*Args, **KwArgs):
      print(*Args, **KwArgs, file=OutFile)
    Write("// Generated at {}".format(datetime.now()))
    Write("")
    Write("#once")
    Write("")

    Write(".RepoRoot = '{}'".format(RepoRoot))
    Write(".Windows10SDKPath = '{}'".format(Windows10SDKPath))
    Write(".Windows10SDKVersion = '{}'".format(Windows10SDKVersion))

    if VS2015Path.exists():
      Write(".VS2015Path = '{}'".format(VS2015Path))

    if VulkanSDKPath.exists():
      Write(".VulkanSDKPath = '{}'".format(VulkanSDKPath))
      Write(".VulkanSDKVersion = '{}'".format(VulkanSDKVersion))
      VersionParts = VulkanSDKVersion.split(".")
      Version = tuple(map(int, VersionParts))
      IsOld = Version[0] == 1 and Version[1] == 0 and Version[2] <= 13
      Write(".VulkanDebugLibSuffix = '{}'".format("" if IsOld else "d"))


if __name__ == '__main__':
  Main()
