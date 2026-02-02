@echo off
REM ============================================================================
REM Versal DMA Benchmark - Vivado Only Build
REM Generates Bitstream and exports XSA
REM ============================================================================

setlocal EnableDelayedExpansion

echo ============================================================
echo        VIVADO BUILD - BITSTREAM AND XSA EXPORT
echo ============================================================
echo.

set SCRIPT_DIR=%~dp0

REM Find Vivado
if not defined XILINX_VIVADO (
    for %%V in (2024.1 2023.2 2023.1) do (
        if exist "C:\Xilinx\Vivado\%%V\bin\vivado.bat" (
            set VIVADO_PATH=C:\Xilinx\Vivado\%%V
            goto :found_vivado
        )
    )
    echo ERROR: Vivado not found! Set XILINX_VIVADO environment variable.
    exit /b 1
) else (
    set VIVADO_PATH=%XILINX_VIVADO%
)
:found_vivado

echo Using Vivado: %VIVADO_PATH%
echo.

set VIVADO_CMD=%VIVADO_PATH%\bin\vivado.bat
set BUILD_SCRIPT=%SCRIPT_DIR%vivado\scripts\build_project.tcl

echo Running: %BUILD_SCRIPT%
echo.

call "%VIVADO_CMD%" -mode batch -source "%BUILD_SCRIPT%" -notrace

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo BUILD FAILED!
    exit /b 1
)

echo.
echo ============================================================
echo VIVADO BUILD COMPLETED SUCCESSFULLY
echo ============================================================
echo XSA: %SCRIPT_DIR%vivado\output\xsa\dma_benchmark.xsa
echo PDI: %SCRIPT_DIR%vivado\output\xsa\dma_benchmark.pdi
echo ============================================================

endlocal
