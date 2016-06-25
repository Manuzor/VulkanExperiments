"""
Initialize this repository by creating a manifest file in the Build dir that
can be used by other scripts to obtain infos about this repo.
"""

import os
import sys
from pathlib import *
from datetime import datetime
import json
import argparse

import SystemInfo

#
# Argument parsing
#

Parser = argparse.ArgumentParser(description="Generates the repo manifest, if it doesn't exist already.")
Parser.add_argument("-f", "--force",
                    action="store_true",
                    help="Whether to generate the manifest, regardless of whether it already exists or not.")
Args = Parser.parse_args()
Force = Args.force

#
# Preparation
#

RepoRoot = SystemInfo.RepoRoot(os.environ).resolve()
ManifestFilePath = SystemInfo.RepoManifestPath(os.environ)

if ManifestFilePath.exists() and not Force:
  print("Manifest already exists. Delete it or try some old-fashioned --force if you need to regenerate it.")
  sys.exit()

if not ManifestFilePath.parent.exists():
  ManifestFilePath.parent.mkdir(parents=True)

Manifest = {}

#
# About the file itself.
#
Manifest["Meta"] = {}
Manifest["Meta"]["LastInitTime"] = str(datetime.now())

#
# Repo Stuff
#
Manifest["Repo"] = {}
Manifest["Repo"]["Path"] = RepoRoot.as_posix()


#
# Visual Studio Paths
#
Manifest["VS2015"] = {}

VS2015Path = SystemInfo.VisualStudio2015Path(os.environ)
Manifest["VS2015"]["Path"] = VS2015Path.as_posix()


#
# Windows SDK Paths
#
Manifest["Windows10SDK"] = {}

Windows10SDKPath, Windows10SDKVersion = SystemInfo.Windows10SDKPathAndLatestVersion(os.environ)

assert Windows10SDKPath.exists(), "Unable to find Windows 10 SDK."
Manifest["Windows10SDK"]["Path"] = Windows10SDKPath.as_posix()

assert Windows10SDKVersion, "Unable to find any installed Windows 10 SDK version. Something must be seriously wrong."
Manifest["Windows10SDK"]["Version"] = Windows10SDKVersion


#
# Vulkan SDK Path
#
Manifest["VulkanSDK"] = {}

VulkanSDKPath = SystemInfo.VulkanSDKPath(os.environ)
Manifest["VulkanSDK"]["Path"] = VulkanSDKPath.as_posix()

#
# Write The File
#
with ManifestFilePath.open("w") as ManifestFile:
  json.dump(Manifest, ManifestFile, indent=2)
