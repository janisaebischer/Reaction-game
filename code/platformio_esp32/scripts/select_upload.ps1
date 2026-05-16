# Interactive upload selection script
Write-Host ""
Write-Host "======================================" -ForegroundColor Cyan
Write-Host "  ReactionGame - Upload Method" -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "1) Serial (USB) - Upload Firmware + Files" -ForegroundColor Yellow
Write-Host "2) OTA (WiFi)   - Upload Firmware only" -ForegroundColor Yellow
Write-Host ""
$choice = Read-Host "Select option (1 or 2)"

Write-Host ""
switch ($choice) {
    "1" {
        Write-Host "Uploading via Serial..." -ForegroundColor Green
        Write-Host "Step 1: Uploading Files (SPIFFS)..." -ForegroundColor Cyan
        & pio run -e reaction_game -t uploadfs
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Step 2: Uploading Firmware..." -ForegroundColor Cyan
            & pio run -e reaction_game -t upload
        } else {
            Write-Host "Files upload failed!" -ForegroundColor Red
            exit 1
        }
    }
    "2" {
        Write-Host "Uploading via OTA (WiFi)..." -ForegroundColor Green
        Write-Host "Make sure ESP32 is running and connected to network" -ForegroundColor Yellow
        & pio run -e ota -t upload --upload-port=ReactionGame.local
    }
    default {
        Write-Host "Invalid choice!" -ForegroundColor Red
        exit 1
    }
}

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "Upload completed successfully!" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "Upload failed!" -ForegroundColor Red
    exit 1
}
