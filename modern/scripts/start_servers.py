"""Start the Moxian server chain: Distribute -> Agent -> Map"""
import subprocess
import time
import sys

SWORKING = r"d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking"

def start_server(name, args=None, wait=5):
    cmd = [f"{SWORKING}\\{name}.exe"]
    if args:
        cmd.extend(args)
    print(f"Starting {name}... ", end="", flush=True)
    try:
        p = subprocess.Popen(cmd, cwd=SWORKING)
        time.sleep(wait)
        if p.poll() is not None:
            print(f"EXITED (code {p.returncode})")
            return None
        else:
            print(f"running (PID {p.pid})")
            return p
    except Exception as e:
        print(f"ERROR: {e}")
        return None

def check_process(name):
    try:
        r = subprocess.run(["tasklist", "/FI", f"IMAGENAME eq {name}.exe"], 
                          capture_output=True, text=True, timeout=5)
        return name in r.stdout
    except:
        return False

def check_port(port):
    try:
        r = subprocess.run(["netstat", "-ano"], capture_output=True, text=True, timeout=5)
        for line in r.stdout.split('\n'):
            if str(port) in line and 'LISTEN' in line:
                return True
        return False
    except:
        return False

print("=" * 60)
print("Starting Moxian Server Chain")
print("=" * 60)

# Start DistributeServer
p1 = start_server("DistributeServer", wait=5)
if not p1:
    print("WARNING: DistributeServer exited immediately!")

# Start AgentServer
p2 = start_server("AgentServer", wait=5)
if not p2:
    print("WARNING: AgentServer exited immediately!")

# Start MapServer with map 17
p3 = start_server("MapServer", args=["17"], wait=10)
if not p3:
    print("WARNING: MapServer exited immediately!")

print("\n" + "=" * 60)
print("Process Status:")
print("=" * 60)
for name in ["DistributeServer", "AgentServer", "MapServer"]:
    running = check_process(name)
    print(f"  {name}: {'RUNNING' if running else 'NOT RUNNING'}")

print("\nPort Status:")
for port, name in [(16001, "DistributeServer-svr"), (14400, "DistributeServer-usr"),
                    (17001, "AgentServer-svr"), (14600, "AgentServer-usr"),
                    (18017, "MapServer-17")]:
    listening = check_port(port)
    print(f"  Port {port} ({name}): {'LISTENING' if listening else 'NOT LISTENING'}")

print("\nDone!")
