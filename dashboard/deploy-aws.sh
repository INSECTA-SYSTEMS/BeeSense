#!/bin/bash

# BeeSense AWS Deployment Script
# Deploy dashboard with SQLite database to AWS EC2

set -e

echo "🐝 BeeSense AWS Deployment"
echo "=========================="
echo ""

# Configuration
REMOTE_USER="ubuntu"
REMOTE_HOST="3.75.94.127"
REMOTE_PATH="/home/ubuntu/beesense"

# Check if we're connected
echo "📡 Testing connection to AWS..."
if ! ssh -o ConnectTimeout=5 ${REMOTE_USER}@${REMOTE_HOST} "echo 'Connected'" > /dev/null 2>&1; then
    echo "❌ Cannot connect to ${REMOTE_HOST}"
    echo "   Make sure you have SSH access configured"
    exit 1
fi
echo "✅ Connection OK"
echo ""

# Stop the running server
echo "🛑 Stopping server..."
ssh ${REMOTE_USER}@${REMOTE_HOST} "pm2 stop beesense || true" > /dev/null 2>&1 || true
echo ""

# Create backup of database if it exists
echo "💾 Backing up database..."
ssh ${REMOTE_USER}@${REMOTE_HOST} "cd ${REMOTE_PATH} && [ -f beesense.db ] && cp beesense.db beesense.db.backup || true"
echo ""

# Upload files
echo "📤 Uploading files..."
scp server.js ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PATH}/
scp database.js ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PATH}/
scp package.json ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PATH}/
scp index.html ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PATH}/
scp style.css ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PATH}/
echo "✅ Files uploaded"
echo ""

# Install dependencies
echo "📦 Installing dependencies..."
ssh ${REMOTE_USER}@${REMOTE_HOST} "cd ${REMOTE_PATH} && npm install"
echo "✅ Dependencies installed"
echo ""

# Start server with PM2
echo "🚀 Starting server..."
ssh ${REMOTE_USER}@${REMOTE_HOST} "cd ${REMOTE_PATH} && pm2 start server.js --name beesense || pm2 restart beesense"
ssh ${REMOTE_USER}@${REMOTE_HOST} "pm2 save"
echo "✅ Server started"
echo ""

# Check status
echo "📊 Server status:"
ssh ${REMOTE_USER}@${REMOTE_HOST} "pm2 status beesense"
echo ""

echo "✅ Deployment complete!"
echo ""
echo "🌐 Dashboard: http://${REMOTE_HOST}:8080"
echo "📊 API: http://${REMOTE_HOST}:8080/api/tracking"
echo "📝 Logs: ssh ${REMOTE_USER}@${REMOTE_HOST} 'pm2 logs beesense'"
echo ""
