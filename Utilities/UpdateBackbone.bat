@echo off

set BackboneGeneratorScript=%~dp0../../Backbone/GenerateSingleHeader.bat

if exist "%BackboneGeneratorScript%" goto :ScriptExists
echo Unable to find Backbone Generator Script: "%BackboneGeneratorScript%"
goto :eof

:ScriptExists

pushd "%~dp0../Code"
  call "%BackboneGeneratorScript%"
popd
