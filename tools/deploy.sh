#!/bin/bash

# ARGUMENT 1: Project Root Path (Defaults to current dir if empty)
PROJECT_ROOT="${1:-.}"

# CONFIG
DRIVE_LABEL="EVERDRIVE"
MOUNT_POINT="/media/$USER/$DRIVE_LABEL"
BUILD_FILE="${PROJECT_ROOT}/build/us_n64/sm64.z64"
TARGET_NAME="Microtransactions64.z64"

echo "🔥 STARTING DEPLOYMENT..."

# 1. BUILD
echo "🔨 Compiling..."
make VERSION=us -j$(nproc)
if [ $? -ne 0 ]; then
    echo "❌ Build Failed!"
    exit 1
fi

# 2. CHECK DRIVE
if [ ! -d "$MOUNT_POINT" ]; then
    echo "⚠️ Drive not found at $MOUNT_POINT"
    echo "   Attempting to mount by label..."
    # Only needed if your OS doesn't auto-mount
    # sudo mount -L $DRIVE_LABEL /mnt/everdrive 
    # MOUNT_POINT="/mnt/everdrive"
    exit 1
fi

# 3. COPY
echo "📦 Copying to Cartridge..."
cp "$BUILD_FILE" "$MOUNT_POINT/$TARGET_NAME"

# 4. SYNC (Critical for SD Cards!)
echo "💾 Syncing buffers..."
sync

echo "✅ DEPLOY COMPLETE. Safe to eject."