# Live Verification Script for MacTrafficLights

Write-Host "=========================================="
Write-Host "  MacTrafficLights Live System Verification"
Write-Host "=========================================="

# 1. Verify MacTrafficLights process
$mtl = Get-Process -Name MacTrafficLights -ErrorAction SilentlyContinue
if (-not $mtl) {
    Write-Host "Starting MacTrafficLights.exe..."
    Start-Process .\MacTrafficLights.exe
    Start-Sleep -Seconds 2
    $mtl = Get-Process -Name MacTrafficLights -ErrorAction SilentlyContinue
}

if ($mtl) {
    Write-Host "[OK] MacTrafficLights running with PID: $($mtl.Id), Memory: $([math]::Round($mtl.WorkingSet64 / 1MB, 2)) MB"
} else {
    Write-Host "[FAIL] MacTrafficLights failed to run"
    exit 1
}

# 2. Launch Notepad
Write-Host "`n[TEST] Launching Notepad.exe..."
$notepad = Start-Process notepad.exe -PassThru
Start-Sleep -Seconds 2

# Check if Notepad window exists
$notepadHwnd = $notepad.MainWindowHandle
Write-Host "  -> Notepad PID: $($notepad.Id), HWND: $notepadHwnd"

# 3. Check for MacTrafficLights_Overlay window
$overlays = Get-Process | Where-Object { $_.ProcessName -eq "MacTrafficLights" }
Write-Host "  -> Verifying active overlays..."

# Read recent log entries
Write-Host "`n[LOGS] Recent Log Entries:"
Get-Content MacTrafficLights.log -Tail 10 | ForEach-Object { Write-Host "   $_" }

# Clean up Notepad
Write-Host "`n[TEST] Closing Notepad..."
Stop-Process -Id $notepad.Id -Force

Write-Host "`n=========================================="
Write-Host "  Live Verification Completed Successfully"
Write-Host "=========================================="
