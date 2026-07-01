import base64, os, json, time, serial

port = serial.Serial("/dev/ttyACM0", 115200, timeout=1)
CHUNK = 192  # 192 bytes → base64 thành 256 chars, an toàn

def upload_vis(port, path, name=None):
    name = name or os.path.basename(path).replace(".vis", "")
    size = os.path.getsize(path)

    def send(obj):
        port.write((json.dumps(obj) + "\n").encode())
        port.flush()

    def recv():
        deadline = time.time() + 5.0
        while time.time() < deadline:
            if port.in_waiting:
                line = port.readline().decode().strip()
                if line:
                    try: return json.loads(line)
                    except: continue
        return None

    # bắt đầu
    send({"cmd": "animStart", "data": {"name": name, "size": size}})
    r = recv()
    if not r or not r.get("ok"):
        print("animStart failed:", r); return False

    sent = 0
    chunk_idx = 0
    with open(path, "rb") as f:
        while True:
            chunk = f.read(CHUNK)
            if not chunk: break
            b64 = base64.b64encode(chunk).decode()
            send({"cmd": "animChunk", "data": {"i": chunk_idx, "d": b64}})
            r = recv()
            if not r or not r.get("ok"):
                print(f"chunk {chunk_idx} failed:", r); return False
            sent += len(chunk)
            chunk_idx += 1
            print(f"Uploading {sent}/{size} bytes ({chunk_idx} chunks)", end="\r")

    send({"cmd": "animEnd", "data": name})
    r = recv()
    print(f"\nUpload {'ok' if r and r.get('ok') else 'failed'}: {r}")
    return r and r.get("ok")

upload_vis(port, "idle.vis")
