# uploadAni.py
import serial, json, time, os, struct

port = serial.Serial("/dev/ttyACM0", 115200, timeout=2)
time.sleep(1)

def send(obj):
    port.write((json.dumps(obj) + "\n").encode())

def recv():
    line = port.readline().decode().strip()
    try: return json.loads(line)
    except: return None

def upload_vis(path, name=None):
    name = name or os.path.basename(path).replace(".vis", "")
    size = os.path.getsize(path)

    send({"cmd": "animStart", "data": {"name": name, "size": size}})
    r = recv()
    if not r or not r.get("ok"):
        print("animStart failed:", r); return

    CHUNK = 256
    sent = 0
    with open(path, "rb") as f:
        while True:
            chunk = f.read(CHUNK)
            if not chunk: break
            port.write(chunk)
            r = recv()  # ACK
            sent += len(chunk)
            print(f"Uploading {sent}/{size} bytes", end="\r")

    send({"cmd": "animEnd"})
    r = recv()
    print(f"\nUpload {'ok' if r and r.get('ok') else 'failed'}")

def play_vis(name):
    send({"cmd": "animPlay", "data": name})
    print(recv())

def stop_vis():
    send({"cmd": "animStop"})
    print(recv())

# usage
upload_vis("idle.vis", "idle")
play_vis("idle.vis")