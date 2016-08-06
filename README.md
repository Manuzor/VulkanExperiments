Vulkan Experiments
==================

Vulkan experiments for my master's thesis about graphics resource management possibilities using the Vulkan API.


Requirements
============

1. Python 3.5 or higher
2. Visual Studio 2015 or higher
3. Windows 10 SDK version `10.0.10586.0` or higher. Older versions might work but are not officially supported.


Build
=====

Before you can start building, run the `Init.bat` script. It will detect important paths on this system and saves them to a file (Workspace/RepoManifest.json).

Whenever an environment variable changes that is relevant to building this project, you have to re-run `Init.bat` (possibly with the `--force` argument).


Setup External: Backbone
========================

_Note: You only need this setup if you're using the script `Utilities/UpdateBackbone.py`._

There are two ways to set up the path to backbone.

1. Set the environment variable `BACKBONE_REPO_ROOT`.
2. Check out the Backbone repo somewhere close to this repo. The init routine tries to find a folder named `Backbone` and uses it as a fallback if no environment variable is found.
