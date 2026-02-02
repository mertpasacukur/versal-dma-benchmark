@echo off
REM ============================================================================
REM Versal DMA Benchmark - Vitis Only Build
REM Creates Platform and Application from existing XSA
REM ============================================================================

setlocal EnableDelayedExpansion

echo ============================================================
echo        VITIS BUILD - PLATFORM AND APPLICATION
echo ============================================================
echo.

set SCRIPT_DIR=%~dp0

REM Check XSA exists
set XSA_FILE=%SCRIPT_DIR%vivado\output\xsa\dma_benchmark.xsa
if not exist "%XSA_FILE%" (
    echo ERROR: XSA not found: %XSA_FILE%
    echo Please run build_vivado.bat first.
    exit /b 1
)

REM Find Vitis
if not defined XILINX_VITIS (
    for %%V in (2024.1 2023.2 2023.1) do (
        if exist "C:\Xilinx\Vitis\%%V\bin\xsct.bat" (
            set VITIS_PATH=C:\Xilinx\Vitis\%%V
            goto :found_vitis
        )
    )
    echo ERROR: Vitis not found! Set XILINX_VITIS environment variable.
    exit /b 1
) else (
    set VITIS_PATH=%XILINX_VITIS%
)
:found_vitis

echo Using Vitis: %VITIS_PATH%
echo XSA: %XSA_FILE%
echo.

set XSCT_CMD=%VITIS_PATH%\bin\xsct.bat

REM Step 1: Create Platform
echo ============================================================
echo   Creating Platform...
echo ============================================================
call "%XSCT_CMD%" "%SCRIPT_DIR%vitis\scripts\create_platform.tcl"

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Platform creation failed!
    exit /b 1
)

REM Step 2: Create and Build Application
echo.
echo ============================================================
echo   Building Application...
echo ============================================================
call "%XSCT_CMD%" "%SCRIPT_DIR%vitis\scripts\create_application.tcl"

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Application build failed!
    exit /b 1
)

echo.
echo ============================================================
echo VITIS BUILD COMPLETED SUCCESSFULLY
echo ============================================================
echo ELF: %SCRIPT_DIR%vitis\workspace\dma_benchmark_app\Debug\dma_benchmark_app.elf
echo ============================================================

endlocal
