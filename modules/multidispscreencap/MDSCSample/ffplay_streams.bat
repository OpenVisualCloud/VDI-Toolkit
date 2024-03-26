@echo off
set clients_num=%1
set clients_st_id=%2
set /a clients_ed_id=%clients_st_id% + %clients_num% - 1
set ip=%3
set port=%4
set /a client_id=0
set ffplayPath=%~dp0
setlocal enabledelayedexpansion
for /l %%i in (%clients_st_id%, 1, %clients_ed_id%) do (
	set url=rtsp://%ip%:%port%/screencap%%i
	start cmd /k "cd %ffplayPath% && ffplay.exe -x 1280 -y 720 -rtsp_transport tcp !url!"
)
