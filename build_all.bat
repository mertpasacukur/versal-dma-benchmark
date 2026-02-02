@echo off
REM ============================================================================
REM Versal DMA Benchmark - Complete Build Script
REM Target: VPK120 (Versal Premium VP1202)
REM ============================================================================

setlocal EnableDelayedExpansion

echo ============================================================
echo        VERSAL DMA BENCHMARK - COMPLETE BUILD
echo ============================================================
echo.

REM Get script directory
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%

REM Vivado and Vitis paths - adjust if needed
REM Default Xilinx installation paths
if not defined XILINX_VIVADO (
    if exist "C:\Xilinx\Vivado\2024.1\bin\vivado.bat" (
        set VIVADO_PATH=C:\Xilinx\Vivado\2024.1
    ) else if exist "C:\Xilinx\Vivado\2023.2\bin\vivado.bat" (
        set VIVADO_PATH=C:\Xilinx\Vivado\2023.2
    ) else if exist "C:\Xilinx\Vivado\2023.1\bin\vivado.bat" (
        set VIVADO_PATH=C:\Xilinx\Vivado\2023.1
    ) else (
        echo ERROR: Vivado installation not found!
        echo Please set XILINX_VIVADO environment variable
        exit /b 1
    )
) else (
    set VIVADO_PATH=%XILINX_VIVADO%
)

if not defined XILINX_VITIS (
    if exist "C:\Xilinx\Vitis\2024.1\bin\xsct.bat" (
        set VITIS_PATH=C:\Xilinx\Vitis\2024.1
    ) else if exist "C:\Xilinx\Vitis\2023.2\bin\xsct.bat" (
        set VITIS_PATH=C:\Xilinx\Vitis\2023.2
    ) else if exist "C:\Xilinx\Vitis\2023.1\bin\xsct.bat" (
        set VITIS_PATH=C:\Xilinx\Vitis\2023.1
    ) else (
        echo ERROR: Vitis installation not found!
        echo Please set XILINX_VITIS environment variable
        exit /b 1
    )
) else (
    set VITIS_PATH=%XILINX_VITIS%
)

set VIVADO_CMD=%VIVADO_PATH%\bin\vivado.bat
set XSCT_CMD=%VITIS_PATH%\bin\xsct.bat

echo Vivado: %VIVADO_PATH%
echo Vitis:  %VITIS_PATH%
echo Project: %PROJECT_ROOT%
echo.

REM ============================================================================
REM Step 1: Vivado Build (Synthesis, Implementation, Bitstream, XSA)
REM ============================================================================

echo ============================================================
echo   STEP 1: VIVADO BUILD
echo ============================================================
echo.

set VIVADO_SCRIPT=%PROJECT_ROOT%vivado\scripts\build_project.tcl

if not exist "%VIVADO_SCRIPT%" (
    echo ERROR: Vivado build script not found: %VIVADO_SCRIPT%
    exit /b 1
)

echo Running Vivado build script...
echo This may take 30-60 minutes depending on your system.
echo.

call "%VIVADO_CMD%" -mode batch -source "%VIVADO_SCRIPT%" -notrace

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Vivado build failed!
    exit /b 1
)

echo.
echo Vivado build completed successfully!
echo.

REM Verify XSA was generated
set XSA_FILE=%PROJECT_ROOT%vivado\output\xsa\dma_benchmark.xsa
if not exist "%XSA_FILE%" (
    echo ERROR: XSA file not found: %XSA_FILE%
    exit /b 1
)

echo XSA exported: %XSA_FILE%
echo.

REM ============================================================================
REM Step 2: Vitis Platform Creation
REM ============================================================================

echo ============================================================
echo   STEP 2: VITIS PLATFORM CREATION
echo ============================================================
echo.

set PLATFORM_SCRIPT=%PROJECT_ROOT%vitis\scripts\create_platform.tcl

if not exist "%PLATFORM_SCRIPT%" (
    echo ERROR: Platform script not found: %PLATFORM_SCRIPT%
    exit /b 1
)

echo Creating Vitis platform from XSA...
echo.

call "%XSCT_CMD%" "%PLATFORM_SCRIPT%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Platform creation failed!
    exit /b 1
)

echo.
echo Platform created successfully!
echo.

REM ============================================================================
REM Step 3: Vitis Application Creation and Build
REM ============================================================================

echo ============================================================
echo   STEP 3: VITIS APPLICATION BUILD
echo ============================================================
echo.

set APP_SCRIPT=%PROJECT_ROOT%vitis\scripts\create_application.tcl

if not exist "%APP_SCRIPT%" (
    echo ERROR: Application script not found: %APP_SCRIPT%
    exit /b 1
)

echo Creating and building Vitis application...
echo.

call "%XSCT_CMD%" "%APP_SCRIPT%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Application build failed!
    exit /b 1
)

echo.

REM ============================================================================
REM Build Complete
REM ============================================================================

echo ============================================================
echo        BUILD COMPLETE
echo ============================================================
echo.
echo Output Files:
echo   XSA:      %PROJECT_ROOT%vivado\output\xsa\dma_benchmark.xsa
echo   PDI:      %PROJECT_ROOT%vivado\output\xsa\dma_benchmark.pdi
echo   ELF:      %PROJECT_ROOT%vitis\workspace\dma_benchmark_app\Debug\dma_benchmark_app.elf
echo   Reports:  %PROJECT_ROOT%vivado\output\versal_dma_benchmark\reports\
echo.
echo To run the benchmark:
echo   1. Connect VPK120 to your PC via JTAG
echo   2. Open Vitis IDE
echo   3. Import workspace: %PROJECT_ROOT%vitis\workspace
echo   4. Right-click application -^> Run As -^> Launch on Hardware
echo.
echo Or use command line:
echo   xsct
echo   connect
echo   target -set -filter {name =~ "*A72*0"}
echo   rst -system
echo   device program %PROJECT_ROOT%vivado\output\xsa\dma_benchmark.pdi
echo   dow %PROJECT_ROOT%vitis\workspace\dma_benchmark_app\Debug\dma_benchmark_app.elf
echo   con
echo.
echo ============================================================

endlocal
exit /b 0
