"""
Generates a sublime text project file from the repo manifest and prints it
to stdout in the sublime-project format.
"""

import os
from pathlib import Path

import SystemInfo

BuildRules = [
  {
    "Name": "Core",
    "Arg": "Core",
  },
  {
    "Name": "Cfg",
    "Arg": "Cfg",
  },
  {
    "Name": "ShaderCompiler",
    "Arg": "ShaderCompiler",
  },
  {
    "Name": "Application",
    "Arg": "Application",
  },
  {
    "Name": "Tests: Build Only",
    "Arg": "Tests",
  },
  {
    "Name": "Tests: Build and Run",
    "Arg": "Tests-Run",
  },
  {
    "Name": "Generate: Sublime Text Project",
    "Arg": "subl",
  },
  {
    "Name": "Generate: Visual Studio Solution",
    "Arg": "vs",
  },
  {
    "Name": "Rebuild All",
    "Arg": "-clean",
  },
]

BuildCommandTemplate = """\"py", "-3", "{Repo[Path]}/Utilities/Build.py", "--proxy-bff", "{Repo[WorkspacePath]}/SublimeText3.bff", "--", "-ide\""""

FileTemplate = """{{
  "build_systems":
  [
    {{
      "cmd":
      [
        {BuildCommand}
      ],
      "working_dir": "{SublWorkingDir}",
      "file_regex": "([A-z]:.*?)\\\\(([0-9]+)(?:,\\\\s*[0-9]+)?\\\\)",
      "name": "VulkanExperiments",
      "variants":
      [{BuildRules}
      ]
    }}
  ],
  "folders":
  [
    {{
      "path": "{Repo[Path]}"
    }},
    {{
      "path": "{Windows10SDK[Path]}/Include/{Windows10SDK[Version]}",
      "name": "Windows Kit {Windows10SDK[Version]}",
      "file_include_patterns": [ "*.h" ],
      "folder_exclude_patterns": [ "__pycache__" ],
    }},
    {{
      "path": "{VS2015[Path]}/VC",
      "name": "Visual Studio 2015 / VC",
      "file_include_patterns": [ "*.h" ]
    }},
    {{
      "path": "{VulkanSDK[Path]}",
      "name": "Vulkan SDK {VulkanSDK[Version]}",
      "file_include_patterns": [ "*.h", "*.hpp" ]
    }}
  ]
}}"""

BuildRuleTemplate = """
          {{
            "cmd":
            [
              {BuildCommand},
              "{Arg}"
            ],
            "name": "{Name}"
          }},"""


def Main():
  ManifestPath = SystemInfo.RepoManifestPath(os.environ)
  Manifest = SystemInfo.LoadRepoManifest(ManifestPath)
  if Manifest is None:
    print("Unable to load repo manifest: ", str(ManifestPath), file=sys.stderr)
    print("Maybe re-run the init script?", file=sys.stderr)

  BuildCommand = BuildCommandTemplate.format(**Manifest)
  SublWorkingDir = SystemInfo.SublimeText3WorkingDir(os.environ)

  #
  # Print The Result
  #
  FormattedBuildRules = ""
  for Rule in BuildRules:
    FormattedBuildRules += BuildRuleTemplate.format(BuildCommand=BuildCommand, **Rule)

  Formatted = FileTemplate.format(BuildCommand=BuildCommand, SublWorkingDir=SublWorkingDir.as_posix(), BuildRules=FormattedBuildRules, **Manifest)
  print(Formatted, end='\n')

  #
  # Make sure the working directory exists
  #
  if not SublWorkingDir.exists():
    SublWorkingDir.mkdir(parents=True)


if __name__ == '__main__':
  Main()
