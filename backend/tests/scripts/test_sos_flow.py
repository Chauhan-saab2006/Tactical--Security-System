"""
End-to-end test script for the Life Link SOS backend.
Simulates the full hardware -> backend -> frontend data flow.

Usage:
  py tests/scripts/test_sos_flow.py [--host HOST] [--port PORT]
"""
import argparse
import json
import time
import threading
import sys

try:
    import requests
except ImportError:
    print("ERROR: 'requests' not installed. Run: py -m pip install requests")
    sys.exit(1)

try:
    import websocket as ws_lib  # websocket-client
except ImportError:
    ws_lib = None
    print("WARNING: 'websocket-client' not installed. WebSocket tests will be skipped.")
    print("         Install with: py -m pip install websocket-client")


DEVICE_API_KEY = "antigravity_secret_123"

# ---------- helpers ----------

def post_telemetry(base, device_id, lat, lon, pulse, battery, sos):
    url = f"{base}/api/v1/telemetry"
    headers = {"X-Device-Key": DEVICE_API_KEY, "Content-Type": "application/json"}
    body = {
        "device_id": device_id,
        "latitude": lat,
        "longitude": lon,
        "pulse": pulse,
        "battery": battery,
        "sos": sos,
    }
    r = requests.post(url, headers=headers, json=body, timeout=15)
    return r.status_code, r.json()


def get_active_sos(base):
    r = requests.get(f"{base}/api/v1/sos/active", timeout=15)
    return r.status_code, r.json()


def get_devices_status(base):
    r = requests.get(f"{base}/api/v1/devices/status", timeout=15)
    return r.status_code, r.json()


def resolve_sos(base, event_id):
    r = requests.post(f"{base}/api/v1/sos/resolve/{event_id}", timeout=15)
    return r.status_code, r.json()


# ---------- tests ----------

class TestResults:
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.errors = []

    def ok(self, name):
        self.passed += 1
        print(f"  [PASS] {name}")

    def fail(self, name, detail=""):
        self.failed += 1
        self.errors.append(f"{name}: {detail}")
        print(f"  [FAIL] {name} -- {detail}")

    def summary(self):
        total = self.passed + self.failed
        print(f"\n{'='*50}")
        print(f"Results: {self.passed}/{total} passed, {self.failed} failed")
        if self.errors:
            print("\nFailures:")
            for e in self.errors:
                print(f"  • {e}")
        print(f"{'='*50}")
        return self.failed == 0


def run_tests(base):
    t = TestResults()

    # ── 1. Health check ──
    print("\n[1] Health Check")
    try:
        r = requests.get(f"{base}/", timeout=15)
        if r.status_code == 200 and "Life Link" in r.json().get("message", ""):
            t.ok("Root endpoint returns 200")
        else:
            t.fail("Root endpoint", f"status={r.status_code}")
    except Exception as e:
        t.fail("Root endpoint", str(e))

    # ── 2. Auth - reject bad device key ──
    print("\n[2] Auth / Device Key Validation")
    try:
        r = requests.post(
            f"{base}/api/v1/telemetry",
            headers={"X-Device-Key": "WRONG_KEY", "Content-Type": "application/json"},
            json={"device_id": "Test", "latitude": 29.0, "longitude": 77.0, "sos": False},
            timeout=15,
        )
        if r.status_code == 401:
            t.ok("Bad device key rejected (401)")
        else:
            t.fail("Bad device key", f"expected 401, got {r.status_code}")
    except Exception as e:
        t.fail("Bad device key", str(e))

    # ── 3. Post Wristband SOS ──
    print("\n[3] Wristband SOS Trigger")
    try:
        code, data = post_telemetry(base, "Wristband", 29.027916, 77.059315, 82, 95, True)
        if code == 200 and data.get("success"):
            t.ok(f"Wristband SOS posted (event_id={data.get('event_id')})")
            wristband_event_id = data["event_id"]
        else:
            t.fail("Wristband SOS", f"status={code}")
            wristband_event_id = None
    except Exception as e:
        t.fail("Wristband SOS", str(e))
        wristband_event_id = None

    # ── 4. Post Node-A SOS ──
    print("\n[4] Node-A SOS Trigger")
    try:
        code, data = post_telemetry(base, "Node-A", 29.211372, 77.017304, 95, 85, True)
        if code == 200 and data.get("success"):
            t.ok(f"Node-A SOS posted (event_id={data.get('event_id')})")
            node_a_event_id = data["event_id"]
        else:
            t.fail("Node-A SOS", f"status={code}")
            node_a_event_id = None
    except Exception as e:
        t.fail("Node-A SOS", str(e))
        node_a_event_id = None

    # ── 5. Verify active SOS list ──
    print("\n[5] Active SOS List")
    try:
        code, events = get_active_sos(base)
        active_ids = [e["id"] for e in events]
        if wristband_event_id in active_ids:
            t.ok("Wristband SOS is active")
        else:
            t.fail("Wristband SOS active check", f"not found in {active_ids}")
        if node_a_event_id in active_ids:
            t.ok("Node-A SOS is active")
        else:
            t.fail("Node-A SOS active check", f"not found in {active_ids}")
    except Exception as e:
        t.fail("Active SOS list", str(e))

    # ── 6. Devices status ──
    print("\n[6] Devices Status")
    try:
        code, devices = get_devices_status(base)
        device_ids = [d["device_id"] for d in devices]
        if "Wristband" in device_ids:
            t.ok("Wristband visible in device status")
        else:
            t.fail("Wristband device status", f"not in {device_ids}")
        if "Node-A" in device_ids:
            t.ok("Node-A visible in device status")
        else:
            t.fail("Node-A device status", f"not in {device_ids}")
    except Exception as e:
        t.fail("Devices status", str(e))

    # ── 7. Telemetry update (position change while SOS active) ──
    print("\n[7] Telemetry Update During Active SOS")
    try:
        code, data = post_telemetry(base, "Wristband", 29.028000, 77.059400, 88, 93, True)
        if code == 200 and data.get("event_id") == wristband_event_id:
            t.ok("SOS event updated (same event_id)")
        else:
            t.fail("SOS update", f"event_id mismatch: {data}")
    except Exception as e:
        t.fail("SOS update", str(e))

    # ── 8. Resolve SOS via API ──
    print("\n[8] Resolve SOS Event via API")
    if wristband_event_id:
        try:
            code, data = resolve_sos(base, wristband_event_id)
            if code == 200 and data.get("status") == "resolved":
                t.ok(f"Wristband SOS resolved (event {wristband_event_id})")
            else:
                t.fail("Resolve SOS", f"status={code}, data={data}")
        except Exception as e:
            t.fail("Resolve SOS", str(e))

    # ── 9. Auto-resolve via sos=false telemetry ──
    print("\n[9] Auto-Resolve via sos=false Telemetry")
    if node_a_event_id:
        try:
            code, data = post_telemetry(base, "Node-A", 29.211372, 77.017304, 72, 90, False)
            if code == 200:
                code2, events = get_active_sos(base)
                active_ids = [e["id"] for e in events]
                if node_a_event_id not in active_ids:
                    t.ok("Node-A SOS auto-resolved")
                else:
                    t.fail("Node-A auto-resolve", f"still active: {active_ids}")
            else:
                t.fail("Node-A auto-resolve telemetry", f"status={code}")
        except Exception as e:
            t.fail("Node-A auto-resolve", str(e))

    # ── 10. Pulse alert thresholds ──
    print("\n[10] Pulse Alert Edge Cases")
    try:
        # High pulse (tachycardia)
        code, _ = post_telemetry(base, "Wristband", 29.027916, 77.059315, 130, 90, False)
        t.ok("High pulse (130 BPM) accepted") if code == 200 else t.fail("High pulse", f"status={code}")

        # Low pulse (bradycardia)
        code, _ = post_telemetry(base, "Wristband", 29.027916, 77.059315, 45, 90, False)
        t.ok("Low pulse (45 BPM) accepted") if code == 200 else t.fail("Low pulse", f"status={code}")

        # Critical fluctuation (>20 BPM diff)
        code, _ = post_telemetry(base, "Wristband", 29.027916, 77.059315, 110, 90, False)
        t.ok("Critical fluctuation accepted") if code == 200 else t.fail("Critical fluct.", f"status={code}")
    except Exception as e:
        t.fail("Pulse alerts", str(e))

    # ── 11. WebSocket connectivity ──
    print("\n[11] WebSocket Connectivity")
    if ws_lib:
        ws_messages = []
        ws_connected = threading.Event()

        def on_open(wsapp):
            ws_connected.set()

        def on_message(wsapp, message):
            ws_messages.append(json.loads(message))

        ws_url = base.replace("http://", "ws://") + "/ws/sos"
        wsapp = ws_lib.WebSocketApp(ws_url, on_open=on_open, on_message=on_message)
        ws_thread = threading.Thread(target=wsapp.run_forever, daemon=True)
        ws_thread.start()

        if ws_connected.wait(timeout=3):
            t.ok("WebSocket /ws/sos connected")

            # Send an SOS and check WS receives it
            post_telemetry(base, "Node-B", 29.210347, 77.015948, 78, 80, True)
            time.sleep(1)

            if any(m.get("type") in ("sos_alert", "telemetry_update") for m in ws_messages):
                t.ok("WebSocket received SOS broadcast")
            else:
                t.fail("WebSocket broadcast", f"no SOS in {len(ws_messages)} messages")
        else:
            t.fail("WebSocket connection", "timeout after 3s")

        wsapp.close()
    else:
        t.fail("WebSocket test", "websocket-client not installed (skipped)")

    # ── 12. Login endpoint ──
    print("\n[12] Login / Auth")
    try:
        r = requests.post(
            f"{base}/api/v1/login",
            data={"username": "operator@lifelink.local", "password": "operator123"},
            timeout=5,
        )
        if r.status_code == 200 and "access_token" in r.json():
            t.ok("Login successful, JWT token received")
        else:
            t.fail("Login", f"status={r.status_code}")
    except Exception as e:
        t.fail("Login", str(e))

    # ── Summary ──
    return t.summary()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Life Link SOS Backend E2E Tests")
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=8000)
    args = parser.parse_args()

    base_url = f"http://{args.host}:{args.port}"
    print(f"[*] Testing backend at: {base_url}")

    success = run_tests(base_url)
    sys.exit(0 if success else 1)
