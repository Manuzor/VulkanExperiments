
import os
import sys
import subprocess
import argparse
from pathlib import *

import SystemInfo


def Build(*, ProxyBff, ExtraFBuildArgs=[]):
  ManifestPath = SystemInfo.RepoManifestPath(os.environ)
  Manifest = SystemInfo.LoadRepoManifest(ManifestPath)
  if Manifest is None:
    print("Unable to load repo manifest: ", str(ManifestPath), file=sys.stderr)
    print("Maybe re-run the init script?", file=sys.stderr)

  MainBFF = Path(Manifest["Repo"]["Path"], "fbuild.bff")

  #
  # Ensure existance of proxy bff
  #

  if ProxyBff is None:
    ProxyBff = MainBFF

  if not ProxyBff.is_file():
    if not ProxyBff.parent.is_dir():
      ProxyBff.parent.mkdir(parents=True)
    ProxyBff.write_text('#include "{}"'.format(MainBFF))

  #
  # Find and prepare the FBuild executable and its arguments
  #
  FBuildExecutable = Manifest["FASTBuild"]["ExecutablePath"]
  FBuildArgs = [ "-config", str(ProxyBff) ]

  #
  # Assemble and execute the command
  #
  Command = [ FBuildExecutable, *FBuildArgs, *ExtraFBuildArgs ]
  Result = subprocess.run(Command, stdout=sys.stdout, stderr=sys.stderr)
  return Result.returncode

def Main():
  Parser = argparse.ArgumentParser(description="Wrapper around FBuild.")
  Parser.add_argument('--proxy-bff',
                      dest="ProxyBff",
                      type=Path,
                      help="The proxy .bff file to trick FBuild into creating different .fdb files.")
  Parser.add_argument('ExtraFBuildArgs',
                      nargs=argparse.REMAINDER,
                      help="Arguments that will be passed to FBuild.")
  Args = Parser.parse_args()
  ExtraFBuildArgs = [Arg for Arg in Args.ExtraFBuildArgs if Arg != "--"]
  Build(ProxyBff=Args.ProxyBff, ExtraFBuildArgs=ExtraFBuildArgs)

if __name__ == '__main__':
  Main()
