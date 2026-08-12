# tests/unit/portal/portal_integration_test.py
# M5.1 smoke: starts mxh_portal, hits the endpoints, asserts expected responses.
# Run via: python tests/unit/portal/portal_integration_test.py
# Or via ctest: see portal_CMakeLists.txt gtest_add_tests.

import subprocess
import sys
import time
import urllib.request
import json
import os
import tempfile

# Force UTF-8 output so ctest captures Unicode characters
if sys.platform == "win32":
    import codecs
    sys.stdout = codecs.getwriter("utf-8")(sys.stdout.buffer, "replace")
    sys.stderr = codecs.getwriter("utf-8")(sys.stderr.buffer, "replace")

EXE = os.path.join(os.path.dirname(__file__), "../../../build/tools/MoxianPortal/Debug/mxh_portal.exe")
if not os.path.exists(EXE):
    # Try from build root
    alt = os.path.join(os.path.dirname(__file__), "../../../../modern/build/tools/MoxianPortal/Debug/mxh_portal.exe")
    if os.path.exists(alt):
        EXE = alt

STATIC_ROOT = os.path.join(os.path.dirname(__file__), "../../../../deploy/portal/static")
PORT = 28999  # unique per run

def wait_for_port(port, timeout=8):
    t0 = time.time()
    while time.time() - t0 < timeout:
        try:
            urllib.request.urlopen(f"http://127.0.0.1:{port}/api/healthz", timeout=1)
            return True
        except Exception:
            time.sleep(0.25)
    return False

def get_json(path):
    with urllib.request.urlopen(f"http://127.0.0.1:{PORT}{path}", timeout=5) as r:
        return json.loads(r.read())

def main():
    if not os.path.exists(EXE):
        print(f"SKIP: {EXE} not found — build MoxianPortal first")
        sys.exit(0)

    env = os.environ.copy()
    env["PORTAL_PORT"]        = str(PORT)
    env["PORTAL_STATIC_ROOT"] = STATIC_ROOT

    # Write a minimal index.html for the smoke
    idx = os.path.join(STATIC_ROOT, "index.html")
    os.makedirs(os.path.dirname(idx), exist_ok=True)
    with open(idx, "w", encoding="utf-8") as f:
        f.write("<!DOCTYPE html><html><body>Moxian Portal</body></html>\n")

    proc = subprocess.Popen(
        [EXE],
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    try:
        ok = wait_for_port(PORT)
        if not ok:
            print("FAIL: portal did not start on port", PORT)
            sys.exit(1)

        # --- /api/healthz ---
        body = get_json("/api/healthz")
        assert body.get("status") == "ok",         f"healthz status={body}"
        assert body.get("version") == "1.0.0",    f"healthz version={body}"
        print("PASS: /api/healthz →", body)

        # --- /api/unknown → 404 ---
        try:
            get_json("/api/nonexistent")
            print("FAIL: /api/nonexistent should return 404")
            sys.exit(1)
        except urllib.error.HTTPError as e:
            assert e.code == 404, f"expected 404, got {e.code}"
            print(f"PASS: /api/nonexistent → 404")

        # --- / → 200 index.html ---
        with urllib.request.urlopen(f"http://127.0.0.1:{PORT}/", timeout=5) as r:
            assert r.status == 200, f"root status={r.status}"
            data = r.read()
            assert b"Moxian Portal" in data, "index.html content mismatch"
            print(f"PASS: / → 200 ({len(data)} bytes)")

        # --- /static/..* (missing file) → 404 JSON ---
        try:
            get_json("/static/dist/main.js")
            print("FAIL: /static/dist/main.js should return 404")
            sys.exit(1)
        except urllib.error.HTTPError as e:
            assert e.code == 404, f"expected 404, got {e.code}"
            body2 = json.loads(e.read())
            assert "error" in body2, f"404 body={body2}"
            print(f"PASS: /static/dist/main.js → 404 JSON")

        print("\nALL TESTS PASSED")
        sys.exit(0)

    finally:
        proc.terminate()
        proc.wait(timeout=5)

if __name__ == "__main__":
    main()
