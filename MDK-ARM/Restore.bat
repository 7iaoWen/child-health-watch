@echo off
setlocal

REM 删除 STM32F103C8Tx_Template 文件夹及其内容
if exist "STM32F103C8Tx_Template" (
    rd /s /q "STM32F103C8Tx_Template"
) else (
    echo STM32F103C8Tx_Template folder does not exist.
)

REM 删除 RTE 文件夹及其内容
if exist "RTE" (
    rd /s /q "RTE"
) else (
    echo RTE folder does not exist.
)

REM 删除文件名中包含 uvguix 的文件
for /f "delims=" %%i in ('dir /b /a-d *uvguix*') do (
    echo Deleting file %%i ...
    del "%%i"
)

del *.lst /s

endlocal
echo Done.

exit
