@py -3 "%~dp0Utilities\Build.py" --proxy-bff "%~dp0Workspace\CommandLine.bff" -- %*

@rem Only pause if run from explorer via double-click.
@if not "%0" == "%~0" @pause
