# BeeSense AWS Deployment Script (PowerShell)
# Deploy dashboard with SQLite database to AWS EC2

$ErrorActionPreference = "Stop"

Write-Host "🐝 BeeSense AWS Deployment" -ForegroundColor Cyan
Write-Host "==========================" -ForegroundColor Cyan
Write-Host ""

# Configuration
$REMOTE_USER = "ubuntu"
$REMOTE_HOST = "3.75.94.127"
$REMOTE_PATH = "/home/ubuntu/beesense"

# Check if we have SSH configured
Write-Host "📡 Testing connection to AWS..." -ForegroundColor Yellow
try {
    $result = ssh -o ConnectTimeout=5 "${REMOTE_USER}@${REMOTE_HOST}" "echo 'Connected'" 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "Connection failed"
    }
    Write-Host "✅ Connection OK" -ForegroundColor Green
} catch {
    Write-Host "❌ Cannot connect to ${REMOTE_HOST}" -ForegroundColor Red
    Write-Host "   Make sure you have SSH access configured" -ForegroundColor Red
    exit 1
}
Write-Host ""

# Stop the running server
Write-Host "🛑 Stopping server..." -ForegroundColor Yellow
ssh "${REMOTE_USER}@${REMOTE_HOST}" "pm2 stop beesense || true" 2>&1 | Out-Null
Write-Host ""

# Create backup of database if it exists
Write-Host "💾 Backing up database..." -ForegroundColor Yellow
ssh "${REMOTE_USER}@${REMOTE_HOST}" "cd ${REMOTE_PATH} && [ -f beesense.db ] && cp beesense.db beesense.db.backup || true" 2>&1 | Out-Null
Write-Host ""

# Upload files
Write-Host "📤 Uploading files..." -ForegroundColor Yellow
scp server.js "${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PATH}/"
scp database.js "${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PATH}/"
scp package.json "${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PATH}/"
scp index.html "${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PATH}/"
scp style.css "${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PATH}/"
Write-Host "✅ Files uploaded" -ForegroundColor Green
Write-Host ""

# Install dependencies
Write-Host "📦 Installing dependencies..." -ForegroundColor Yellow
ssh "${REMOTE_USER}@${REMOTE_HOST}" "cd ${REMOTE_PATH} && npm install"
Write-Host "✅ Dependencies installed" -ForegroundColor Green
Write-Host ""

# Start server with PM2
Write-Host "🚀 Starting server..." -ForegroundColor Yellow
ssh "${REMOTE_USER}@${REMOTE_HOST}" "cd ${REMOTE_PATH} && pm2 start server.js --name beesense || pm2 restart beesense"
ssh "${REMOTE_USER}@${REMOTE_HOST}" "pm2 save"
Write-Host "✅ Server started" -ForegroundColor Green
Write-Host ""

# Check status
Write-Host "📊 Server status:" -ForegroundColor Cyan
ssh "${REMOTE_USER}@${REMOTE_HOST}" "pm2 status beesense"
Write-Host ""

Write-Host "✅ Deployment complete!" -ForegroundColor Green
Write-Host ""
Write-Host "🌐 Dashboard: http://${REMOTE_HOST}:8080" -ForegroundColor Cyan
Write-Host "📊 API: http://${REMOTE_HOST}:8080/api/tracking" -ForegroundColor Cyan
Write-Host "📝 Logs: ssh ${REMOTE_USER}@${REMOTE_HOST} 'pm2 logs beesense'" -ForegroundColor Cyan
Write-Host ""
