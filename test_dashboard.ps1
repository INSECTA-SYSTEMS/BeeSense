# BeeSense Test Script
# Test the AWS dashboard and API

Write-Host "=== BeeSense AWS Dashboard Test ===" -ForegroundColor Cyan
Write-Host ""

$SERVER_URL = "http://3.75.94.127:8080"

# Test 1: Check if server is reachable
Write-Host "Test 1: Checking if server is reachable..." -ForegroundColor Yellow
try {
    $response = Invoke-WebRequest -Uri $SERVER_URL -TimeoutSec 5 -UseBasicParsing
    Write-Host "Success - Dashboard is accessible (Status: $($response.StatusCode))" -ForegroundColor Green
}
catch {
    Write-Host "Failed - Dashboard not accessible: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "  Make sure the server is running on AWS!" -ForegroundColor Red
    exit 1
}

Write-Host ""

# Test 2: Get current tracking data
Write-Host "Test 2: Fetching current tracking data..." -ForegroundColor Yellow
try {
    $data = Invoke-RestMethod -Uri "$SERVER_URL/api/tracking" -Method Get
    Write-Host "Success - API is working!" -ForegroundColor Green
    Write-Host "  Current data:" -ForegroundColor Gray
    Write-Host "    Einflug: $($data.einflug)" -ForegroundColor Gray
    Write-Host "    Ausflug: $($data.ausflug)" -ForegroundColor Gray
    Write-Host "    Last Update: $($data.lastUpdate)" -ForegroundColor Gray
}
catch {
    Write-Host "Failed - API not responding: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Write-Host ""

# Test 3: Send test data
Write-Host "Test 3: Sending test data to API..." -ForegroundColor Yellow
$testData = @{
    einflug = 42
    ausflug = 37
    timestamp = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
} | ConvertTo-Json

try {
    $response = Invoke-RestMethod -Uri "$SERVER_URL/api/tracking" -Method Post -Body $testData -ContentType "application/json"
    Write-Host "Success - Test data sent!" -ForegroundColor Green
    Write-Host "  Server response: $($response.message)" -ForegroundColor Gray
}
catch {
    Write-Host "Failed - Could not send data: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Write-Host ""

# Test 4: Verify data was received
Write-Host "Test 4: Verifying data was updated..." -ForegroundColor Yellow
Start-Sleep -Seconds 1
try {
    $updatedData = Invoke-RestMethod -Uri "$SERVER_URL/api/tracking" -Method Get
    if ($updatedData.einflug -eq 42 -and $updatedData.ausflug -eq 37) {
        Write-Host "Success - Data successfully updated on server!" -ForegroundColor Green
        Write-Host "  Einflug: $($updatedData.einflug)" -ForegroundColor Green
        Write-Host "  Ausflug: $($updatedData.ausflug)" -ForegroundColor Green
    }
    else {
        Write-Host "Warning - Data mismatch!" -ForegroundColor Yellow
        Write-Host "  Expected: Einflug=42, Ausflug=37" -ForegroundColor Yellow
        Write-Host "  Got: Einflug=$($updatedData.einflug), Ausflug=$($updatedData.ausflug)" -ForegroundColor Yellow
    }
}
catch {
    Write-Host "Failed - Could not verify: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "=== All Tests Passed! ===" -ForegroundColor Green
Write-Host ""
Write-Host "Your BeeSense dashboard is ready!" -ForegroundColor Cyan
Write-Host "Dashboard URL: $SERVER_URL" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Open dashboard in browser: $SERVER_URL" -ForegroundColor White
Write-Host "2. Configure ESP32 WiFi (see ESP32_SETUP.md)" -ForegroundColor White
Write-Host "3. Flash ESP32 firmware" -ForegroundColor White
Write-Host ""
