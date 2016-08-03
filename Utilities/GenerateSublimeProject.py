"""
Generates a sublime text project file from the repo manifest and prints it
to stdout in the sublime-project format.
"""

import os
from pathlib import Path

import SystemInfo

ManifestPath = SystemInfo.RepoManifestPath(os.environ)
Manifest = SystemInfo.LoadRepoManifestPath(ManifestPath)
if Manifest is None:
  print("Unable to load repo manifest: ", str(ManifestPath), file=sys.stderr)
  print("Maybe re-run the init script?", file=sys.stderr)

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

FileTemplate = """{{
  "build_systems":
  [
    {{
      "cmd":
      [
        "${{folder}}/Utilities/FASTBuild/FBuild.exe",
        "-ide"
      ],
      "file_regex": "([A-z]:.*?)\\\\(([0-9]+)(?:,\\\\s*[0-9]+)?\\\\)",
      "name": "VulkanExperiments",
      "variants":
      [{BuildRules}
      ],
      "working_dir": "${{folder}}"
    }}
  ],
  "folders":
  [
    {{
      "path": "."
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
      "name": "Vulkan SDK",
      "file_include_patterns": [ "*.h", "*.hpp" ]
    }}
  ]
}}"""

#
# Print The Result
#
FormattedBuildRules = ""
for Rule in BuildRules:
  Template = """
        {{
          "cmd":
          [
            "${{folder}}/Utilities/FASTBuild/FBuild.exe",
            "-ide",
            "{Arg}"
          ],
          "name": "{Name}"
        }},"""
  FormattedBuildRules += Template.format(**Rule)

Formatted = FileTemplate.format(BuildRules=FormattedBuildRules, **Manifest)
print(Formatted, end='\n')
