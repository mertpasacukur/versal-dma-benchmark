@echo off
REM ============================================================================
REM Versal DMA Benchmark - Run on Hardware
REM Downloads PDI and ELF to VPK120 and starts execution
REM ============================================================================

setlocal EnableDelayedExpansion

echo ============================================================
echo        RUN DMA BENCHMARK ON VPK120
echo ============================================================
echo.

set SCRIPT_DIR=%~dp0

REM Check required files exist
set PDI_FILE=%SCRIPT_DIR%vivado\output\xsa\dma_benchmark.pdi
set ELF_FILE=%SCRIPT_DIR%vitis\workspace\dma_benchmark_app\Debug\dma_benchmark_app.elf

if not exist "%PDI_FILE%" (
    echo ERROR: PDI not found: %PDI_FILE%
    echo Please run build first.
    exit /b 1
)

if not exist "%ELF_FILE%" (
    echo ERROR: ELF not found: %ELF_FILE%
    echo Please run build first.
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
    echo ERROR: Vitis not found!
    exit /b 1
) else (
    set VITIS_PATH=%XILINX_VITIS%
)
:found_vitis

set XSCT_CMD=%VITIS_PATH%\bin\xsct.bat

echo PDI: %PDI_FILE%
echo ELF: %ELF_FILE%
echo.
echo Make sure VPK120 is connected via JTAG and powered on.
echo.
pause

REM Create XSCT script
set XSCT_SCRIPT=%SCRIPT_DIR%run_hw.tcl

echo # Auto-generated XSCT script > "%XSCT_SCRIPT%"
echo puts "Connecting to hardware..." >> "%XSCT_SCRIPT%"
echo connect >> "%XSCT_SCRIPT%"
echo puts "Targeting A72 core 0..." >> "%XSCT_SCRIPT%"
echo target -set -filter {name =~ "*A72*0"} >> "%XSCT_SCRIPT%"
echo puts "Resetting system..." >> "%XSCT_SCRIPT%"
echo rst -system >> "%XSCT_SCRIPT%"
echo after 1000 >> "%XSCT_SCRIPT%"
echo puts "Programming device with PDI..." >> "%XSCT_SCRIPT%"
echo device program {%PDI_FILE:\=/%} >> "%XSCT_SCRIPT%"
echo after 2000 >> "%XSCT_SCRIPT%"
echo puts "Downloading ELF..." >> "%XSCT_SCRIPT%"
echo dow {%ELF_FILE:\=/%} >> "%XSCT_SCRIPT%"
echo puts "Starting execution..." >> "%XSCT_SCRIPT%"
echo con >> "%XSCT_SCRIPT%"
echo puts "" >> "%XSCT_SCRIPT%"
echo puts "============================================================" >> "%XSCT_SCRIPT%"
echo puts "Application running. Connect terminal to UART for output." >> "%XSCT_SCRIPT%"
echo puts "UART Settings: 115200 baud, 8N1" >> "%XSCT_SCRIPT%"
echo puts "============================================================" >> "%XSCT_SCRIPT%"

echo Running XSCT...
call "%XSCT_CMD%" "%XSCT_SCRIPT%"

del "%XSCT_SCRIPT%"

echo.
echo ============================================================
echo Benchmark is now running on VPK120
echo Open a terminal to UART (115200, 8N1) to see output
echo ============================================================

endlocal
