import serial, json, time

port = serial.Serial("/dev/ttyACM0", 115200, timeout=1)
time.sleep(1)

def send(obj):
    port.write((json.dumps(obj) + "\n").encode())

def recv():
    while port.in_waiting == 0:
        time.sleep(0.01)
    line = port.readline().decode().strip()
    try: return json.loads(line)
    except: return None

def cmd(obj):
    send(obj)
    print(recv())

"""
# ping
send({"cmd": "ping"})
print(recv())

# list profiles
send({"cmd": "listProfiles"})
print(recv())

# get profile hiện tại
send({"cmd": "getProfile"})
prf = recv()
print(prf)

# chỉnh một field rồi set lại
prf["data"]["at"] = 0.3
send({"cmd": "setProfile", "data": prf["data"]})
print(recv())

# lưu vào file
send({"cmd": "saveProfile", "data": "/myprofile.json"})
print(recv())

# load từ file
send({"cmd": "loadProfile", "data": "/myprofile.json"})
print(recv())

# delete
send({"cmd": "deleteProfile", "data": "/myprofile.json"})
print(recv())

# stream sensor 5 giây
send({"cmd": "setMode", "data": "stream"})
recv()  # ok
start = time.time()
while time.time() - start < 5:
    while port.in_waiting:
        line = port.readline().decode().strip()
        try: print(json.loads(line))
        except: pass
send({"cmd": "setMode", "data": "rr"})
recv()

# reboot
send({"cmd": "reboot"})
print(recv())
"""