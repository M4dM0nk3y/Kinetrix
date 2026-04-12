#!/usr/bin/env python3
"""
Kinetrix Cloud Registry Server
Provides endpoints for:
1. Publishing packages (`kpm publish`)
2. Downloading packages (`kpm install`)
3. OTA deployments (`kpm deploy`)
4. Robot Agents polling for updates

Security:
  Set KPM_API_KEY env var to require authentication on publish/deploy.
  If unset, all endpoints are open (local development mode).
"""

from http.server import HTTPServer, BaseHTTPRequestHandler
import json
import os
import shutil
import re

HOST = '0.0.0.0'
PORT = int(os.environ.get('KPM_PORT', '5050'))

# Optional API key for publish/deploy authentication
API_KEY = os.environ.get('KPM_API_KEY', None)

# Strict package name validation: alphanumeric, hyphens, underscores only
VALID_PKG_RE = re.compile(r'^[a-zA-Z0-9_-]+$')

# Mock database tracking published packages and latest deployments
db = {
    "packages": {},       # Map of package_name -> list of versions
    "deployments": {}     # Map of robot_id -> {"package": pkg, "version": ver}
}

DATA_DIR = "./cloud_data"


def sanitize_pkg_name(name):
    """Sanitize and validate a package name to prevent path traversal."""
    if not name:
        return None
    # Strip any directory components
    name = os.path.basename(name)
    if not name or not VALID_PKG_RE.match(name):
        return None
    return name


class KinetrixRegistryHandler(BaseHTTPRequestHandler):

    def send_json(self, status, data):
        self.send_response(status)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode('utf-8'))

    def check_auth(self):
        """Check API key if KPM_API_KEY is configured."""
        if API_KEY is None:
            return True  # No auth configured (local dev mode)
        provided = self.headers.get('X-API-Key', '')
        return provided == API_KEY

    def do_GET(self):
        # /api/packages/<pkg>/download
        match = re.match(r'/api/packages/([^/]+)/download', self.path)
        if match:
            pkg_name = sanitize_pkg_name(match.group(1))
            if not pkg_name:
                self.send_json(400, {"error": "Invalid package name"})
                return
            pkg_path = os.path.join(DATA_DIR, f"{pkg_name}.tar.gz")
            if os.path.exists(pkg_path):
                self.send_response(200)
                self.send_header('Content-type', 'application/gzip')
                self.end_headers()
                with open(pkg_path, 'rb') as f:
                    self.wfile.write(f.read())
            else:
                self.send_json(404, {"error": f"Package {pkg_name} not found"})
            return
            
        # /api/ota/<robot_id>/poll
        match = re.match(r'/api/ota/([^/]+)/poll', self.path)
        if match:
            robot_id = match.group(1)
            deployment = db["deployments"].get(robot_id)
            if deployment:
                self.send_json(200, {"update_available": True, "deployment": deployment})
            else:
                self.send_json(200, {"update_available": False})
            return
            
        self.send_json(404, {"error": "Endpoint not found"})

    def do_POST(self):
        # /api/packages/publish
        if self.path == '/api/packages/publish':
            if not self.check_auth():
                self.send_json(403, {"error": "Invalid or missing API key"})
                return

            pkg_name = sanitize_pkg_name(self.headers.get('X-Package-Name'))
            pkg_version = self.headers.get('X-Package-Version')
            
            if not pkg_name:
                self.send_json(400, {"error": "Invalid or missing package name (alphanumeric, hyphens, underscores only)"})
                return
            if not pkg_version:
                self.send_json(400, {"error": "Missing package version"})
                return
                
            length = int(self.headers.get('Content-Length', 0))
            data = self.rfile.read(length)
            
            pkg_path = os.path.join(DATA_DIR, f"{pkg_name}.tar.gz")
            with open(pkg_path, 'wb') as f:
                f.write(data)
                
            if pkg_name not in db["packages"]:
                db["packages"][pkg_name] = []
            db["packages"][pkg_name].append(pkg_version)
            
            self.send_json(200, {"status": "success", "message": f"Published {pkg_name} v{pkg_version}"})
            return
            
        # /api/ota/deploy
        if self.path == '/api/ota/deploy':
            if not self.check_auth():
                self.send_json(403, {"error": "Invalid or missing API key"})
                return

            length = int(self.headers.get('Content-Length', 0))
            body = json.loads(self.rfile.read(length).decode('utf-8'))
            
            robot_id = body.get("robot_id")
            pkg_name = body.get("package")
            version = body.get("version", "latest")
            
            if not robot_id or not pkg_name:
                self.send_json(400, {"error": "Missing parameters"})
                return
                
            db["deployments"][robot_id] = {
                "package": pkg_name,
                "version": version
            }
            
            self.send_json(200, {"status": "success", "message": f"Deployed {pkg_name} to {robot_id}"})
            return
            
        self.send_json(404, {"error": "Endpoint not found"})

if __name__ == '__main__':
    if not os.path.exists(DATA_DIR):
        os.makedirs(DATA_DIR)
    
    if API_KEY:
        print(f"Authentication enabled (KPM_API_KEY is set)")
    else:
        print(f"WARNING: No KPM_API_KEY set — running in open mode (local dev only)")

    server = HTTPServer((HOST, PORT), KinetrixRegistryHandler)
    print(f"Kinetrix Cloud Registry started on http://{HOST}:{PORT}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    print("\nShutting down registry...")
    server.server_close()
