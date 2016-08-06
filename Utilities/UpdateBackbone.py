import os
from pathlib import *
import importlib.util

import SystemInfo

def Main():
  #
  # Preparation
  #
  BackbonePath = SystemInfo.BackbonePath(os.environ)
  if not BackbonePath.exists():
    print("Failed to find backbone path.")
    return

  BackbonePath = BackbonePath.resolve()
  BackboneGeneratorModulePath = BackbonePath / "Utilities" / "GenerateSingleHeader.py"
  RepoRoot = SystemInfo.RepoRoot(os.environ).resolve()
  CodePath = RepoRoot / "Code"

  #
  # Load the generator module
  #

  ModuleSpec = importlib.util.spec_from_file_location("GenerateSingleHeader.py", str(BackboneGeneratorModulePath))
  Module = importlib.util.module_from_spec(ModuleSpec)
  ModuleSpec.loader.exec_module(Module)

  #
  # Generate
  #
  Module.Generate(OutBasePath=CodePath, BackboneRoot=BackbonePath)


if __name__ == '__main__':
  Main()
