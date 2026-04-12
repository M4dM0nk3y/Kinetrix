#!/usr/bin/env python3
"""
Kinetrix Robot Agent
Runs on the robot hardware (e.g., Raspberry Pi).
Polls the Cloud Registry for OTA updates and applies them automatically.

Security:
  - Safe tar extraction (prevents Zip Slip / path traversal)
  - Semantic version comparison (rejects downgrades)
"""

import urllib.request
import urllib.error
import urllib.parse
import json
import time
import os
import subprocess
import tarfile
import io
import sys
import shutil
import re

ROBOT_ID = os.environ.get("ROBOT_ID", "robot-001")
REGISTRY_URL = os.environ.get("KPM_REGISTRY_URL", "http://localhost:5050")
DEPLOY_DIR = "./robot_deployment"
POLL_INTERVAL = 5  # seconds

current_version = None
current_pkg = None
process = None


def parse_semver(version_str):
    """Parse a version string into a comparable tuple, e.g. '3.1.0' -> (3, 1, 0).
    Returns (0, 0, 0) for unparseable strings like 'latest'."""
    match = re.match(r'^v?(\d+)(?:\.(\d+))?(?:\.(\d+))?', str(version_str))
    if not match:
        return (0, 0, 0)
    return (
        int(match.group(1)),
        int(match.group(2) or 0),
        int(match.group(3) or 0),
    )


def is_version_upgrade(current_ver, new_ver):
    """Return True if new_ver is strictly greater than current_ver.
    Versions that can't be parsed (like 'latest') always trigger an update."""
    current_tuple = parse_semver(current_ver)
    new_tuple = parse_semver(new_ver)
    # If both are unparseable (0,0,0), allow the update
    if current_tuple == (0, 0, 0) or new_tuple == (0, 0, 0):
        return True
    return new_tuple > current_tuple


def safe_extract_tar(tar, dest_dir):
    """Extract tar members safely, rejecting any path traversal attempts."""
    abs_dest = os.path.abspath(dest_dir)
    for member in tar.getmembers():
        member_path = os.path.abspath(os.path.join(dest_dir, member.name))
        if not member_path.startswith(abs_dest + os.sep) and member_path != abs_dest:
            raise Exception(f"Path traversal attempt blocked: {member.name}")
    tar.extractall(path=dest_dir)


def get_latest_deployment():
    url = f"{REGISTRY_URL}/api/ota/{ROBOT_ID}/poll"
    try:
        req = urllib.request.Request(url)
        with urllib.request.urlopen(req) as response:
            data = json.loads(response.read().decode('utf-8'))
            if data.get("update_available"):
                return data["deployment"]
            return None
    except urllib.error.URLError:
        return None

def download_and_extract(pkg_name):
    url = f"{REGISTRY_URL}/api/packages/{pkg_name}/download"
    print(f"[{ROBOT_ID}] Downloading {pkg_name}...")
    try:
        req = urllib.request.Request(url)
        with urllib.request.urlopen(req) as response:
            tar_data = response.read()
            
        if os.path.exists(DEPLOY_DIR):
            shutil.rmtree(DEPLOY_DIR)
        os.makedirs(DEPLOY_DIR)
            
        with tarfile.open(fileobj=io.BytesIO(tar_data), mode="r:gz") as tar:
            safe_extract_tar(tar, DEPLOY_DIR)
            
        print(f"[{ROBOT_ID}] ✓ Extracted to {DEPLOY_DIR}")
        return True
    except Exception as e:
        print(f"[{ROBOT_ID}] Failed to download {pkg_name}: {e}")
        return False

def compile_and_run():
    global process
    
    main_file = os.path.join(DEPLOY_DIR, "main.kx")
    if not os.path.exists(main_file):
        print(f"[{ROBOT_ID}] Error: No 'main.kx' found in deployment package.")
        return
        
    print(f"[{ROBOT_ID}] Compiling {main_file} for Raspberry Pi...")
    out_file = os.path.join(DEPLOY_DIR, "robot_app.py")
    
    # Assume kcc is in parent or path
    kcc_path = "./kcc"
    if not os.path.exists(kcc_path):
        kcc_path = "../kcc"
        
    compile_cmd = [kcc_path, main_file, "--target", "rpi", "-o", out_file]
    
    res = subprocess.run(compile_cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"[{ROBOT_ID}] Compilation failed:\n{res.stderr}")
        return
        
    print(f"[{ROBOT_ID}] Compilation successful. Starting new process.")
    
    if process and process.poll() is None:
        process.terminate()
        process.wait()
        
    # Start the python process
    process = subprocess.Popen(["python3", out_file])
    print(f"[{ROBOT_ID}] -> App running (PID: {process.pid})")

def main():
    global current_version, current_pkg
    print(f"Starting Kinetrix Robot Agent [ID: {ROBOT_ID}]")
    print(f"Polling {REGISTRY_URL} every {POLL_INTERVAL}s...")
    
    while True:
        deployment = get_latest_deployment()
        
        if deployment:
            pkg = deployment["package"]
            ver = deployment["version"]
            
            if pkg != current_pkg or ver != current_version:
                # Reject version downgrades (only when both are parseable semver)
                if current_version and not is_version_upgrade(current_version, ver):
                    print(f"[{ROBOT_ID}] Rejecting downgrade: {current_version} -> {ver}")
                else:
                    print(f"\n[{ROBOT_ID}] OTA Update Detected: {pkg} v{ver}")
                    if download_and_extract(pkg):
                        compile_and_run()
                        current_pkg = pkg
                        current_version = ver
                
        time.sleep(POLL_INTERVAL)

if __name__ == "__main__":
    main()
