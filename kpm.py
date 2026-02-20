#!/usr/bin/env python3
"""
Kinetrix Package Manager (kpm)
Manages dependencies and packages for Kinetrix robotics projects.
Usage:
  kpm init              - Create a kinetrix.json file
  kpm install <pkg>     - Install a package
  kpm publish           - Publish current package
"""

import sys
import os
import json
import urllib.request
import urllib.error
import shutil
import tarfile
import io

REGISTRY_URL = "http://localhost:5050"

def cmd_init():
    if os.path.exists("kinetrix.json"):
        print("Error: kinetrix.json already exists in this directory.")
        return
        
    name = input("Package name (default: my_robot): ").strip() or "my_robot"
    version = input("Version (default: 1.0.0): ").strip() or "1.0.0"
    author = input("Author: ").strip()
    
    config = {
        "name": name,
        "version": version,
        "author": author,
        "dependencies": {}
    }
    
    with open("kinetrix.json", "w") as f:
        json.dump(config, f, indent=4)
    print(f"\nInitialized empty Kinetrix project in {os.getcwd()}/kinetrix.json")

def cmd_install(pkg_name):
    # If no package name provided, read from kinetrix.json
    if not pkg_name:
        if not os.path.exists("kinetrix.json"):
            print("Error: no package specified and no kinetrix.json found.")
            return
        with open("kinetrix.json", "r") as f:
            config = json.load(f)
            
        print(f"Installing dependencies for {config.get('name')}...")
        for dep in config.get("dependencies", {}):
            install_single(dep)
        return
        
    install_single(pkg_name)
    
    # Add to kinetrix.json if it exists
    if os.path.exists("kinetrix.json"):
        with open("kinetrix.json", "r") as f:
            config = json.load(f)
        if "dependencies" not in config:
            config["dependencies"] = {}
        config["dependencies"][pkg_name] = "latest"
        with open("kinetrix.json", "w") as f:
            json.dump(config, f, indent=4)

def install_single(pkg_name):
    print(f"Resolving {pkg_name}...")
    modules_dir = "kinetrix_modules"
    if not os.path.exists(modules_dir):
        os.makedirs(modules_dir)
        
    url = f"{REGISTRY_URL}/api/packages/{pkg_name}/download"
    try:
        req = urllib.request.Request(url)
        with urllib.request.urlopen(req) as response:
            tar_data = response.read()
            
        # Extract tarball
        with tarfile.open(fileobj=io.BytesIO(tar_data), mode="r:gz") as tar:
            target_dir = os.path.join(modules_dir, pkg_name)
            if os.path.exists(target_dir):
                shutil.rmtree(target_dir)
            os.makedirs(target_dir)
            tar.extractall(path=target_dir)
            
        print(f"✓ Added {pkg_name} to kinetrix_modules/")
    except urllib.error.URLError as e:
        print(f"Error: Could not install '{pkg_name}'. Package not found or registry offline.")
        print(f"Details: {e}")

def cmd_publish():
    if not os.path.exists("kinetrix.json"):
        print("Error: No kinetrix.json found. Run 'kpm init' first.")
        return
        
    with open("kinetrix.json", "r") as f:
        config = json.load(f)
        
    name = config["name"]
    version = config["version"]
    
    print(f"Packing {name} v{version}...")
    
    # Create tarball in memory
    tar_stream = io.BytesIO()
    with tarfile.open(fileobj=tar_stream, mode="w:gz") as tar:
        for item in os.listdir("."):
            if item not in ["kinetrix_modules", ".git", "__pycache__"]:
                tar.add(item)
                
    tar_data = tar_stream.getvalue()
    
    print(f"Publishing to {REGISTRY_URL}...")
    url = f"{REGISTRY_URL}/api/packages/publish"
    
    try:
        req = urllib.request.Request(url, data=tar_data, method="POST")
        req.add_header('Content-Type', 'application/gzip')
        req.add_header('X-Package-Name', name)
        req.add_header('X-Package-Version', version)
        
        with urllib.request.urlopen(req) as response:
            print(response.read().decode('utf-8'))
            print(f"✓ Published {name} successfully!")
    except urllib.error.URLError as e:
        print(f"Error: Could not publish package.")
        print(f"Details: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
        
    cmd = sys.argv[1]
    if cmd == "init":
        cmd_init()
    elif cmd == "install":
        pkg = sys.argv[2] if len(sys.argv) > 2 else None
        cmd_install(pkg)
    elif cmd == "publish":
        cmd_publish()
    else:
        print(f"Unknown command: {cmd}")
        print(__doc__)
