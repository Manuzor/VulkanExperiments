@pushd "%~dp0Workspace"
@"%~dp0Utilities\FASTBuild\FBuild.exe" -config "%~dp0fbuild.bff" %*
@popd

@rem Only pause if run from explorer via double-click.
@if not "%0" == "%~0" @pause
