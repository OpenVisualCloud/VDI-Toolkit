@echo off
%1(start /min cmd.exe /c %0 :&exit)

:start
echo ===============================
echo Windows Workload Emulate Test
echo ===============================

timeout 5
start /w /min python edge.py

timeout 3
if exist C:\Users\Administrator\Downloads\Doc1.docx del C:\Users\Administrator\Downloads\Doc1.docx
start /w /min python word.py

timeout 3
if exist C:\Users\Administrator\Downloads\hello.c del C:\Users\Administrator\Downloads\hello.c
start /w /min python vscode.py

timeout 3
start /w /min python vlc.py

timeout 3
if exist C:\Users\Administrator\Documents\notepad.txt del C:\Users\Administrator\Documents\notepad.txt
start /w /min python nodepad.py

goto start

:end