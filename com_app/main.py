import tkinter as tk
from tkinter import ttk, messagebox, filedialog, colorchooser
import serial
import serial.tools.list_ports
import json
import threading
import time
import os

# ─────────────────────────────────────────────
#  Theme — monochrome
# ─────────────────────────────────────────────
BG       = "#000000"
PANEL    = "#0d0d0d"
BORDER   = "#2a2a2a"
ACCENT   = "#ffffff"
ACCENT2  = "#aaaaaa"
TEXT     = "#e8e8e8"
TEXT_DIM = "#555555"
ENTRY_BG = "#141414"
BTN_BG   = "#1c1c1c"
BTN_HOV  = "#2a2a2a"
RED      = "#ff4444"
GREEN    = "#88ff88"

FONT_MONO  = ("JetBrains Mono", 10)
FONT_SMALL = ("JetBrains Mono", 9)
FONT_HEAD  = ("JetBrains Mono", 11, "bold")
FONT_TITLE = ("JetBrains Mono", 14, "bold")

INPUT_HANDLERS = ["Digital", "Hysteresis", "Rapid Trigger"]
USB_MODES      = ["Keyboard", "Gamepad", "Mouse"]
GLOW_TYPES     = ["Single Fade", "Ripple", "Smooth", "Burn-in", "Analog", "Solid"]

# ─────────────────────────────────────────────
#  Serial / Protocol
# ─────────────────────────────────────────────
class MufukiConn:
    def __init__(self):
        self.port      = None
        self.lock      = threading.Lock()
        self.connected = False

    def connect(self, portname, baud=115200):
        try:
            self.port = serial.Serial(portname, baud, timeout=2)
            time.sleep(0.5)
            r = self.cmd({"cmd": "ping"})
            if r and r.get("ok"):
                self.connected = True
                return True
        except Exception as e:
            print("Connect error:", e)
        self.connected = False
        return False

    def disconnect(self):
        self.connected = False
        if self.port and self.port.is_open:
            self.port.close()
        self.port = None

    def send(self, obj):
        if not self.port or not self.port.is_open: return
        with self.lock:
            self.port.write((json.dumps(obj) + "\n").encode())
            self.port.flush()

    def recv(self, timeout=3.0):
        if not self.port or not self.port.is_open: return None
        deadline = time.time() + timeout
        with self.lock:
            while time.time() < deadline:
                if self.port.in_waiting:
                    line = self.port.readline().decode(errors="ignore").strip()
                    if line:
                        try: return json.loads(line)
                        except: continue
                time.sleep(0.01)
        return None

    def cmd(self, obj, timeout=3.0):
        self.send(obj)
        return self.recv(timeout)

    def get_profile(self):  return self.cmd({"cmd": "getProfile"})
    def set_profile(self, d): return self.cmd({"cmd": "setProfile", "data": d})
    def save_profile(self, p): return self.cmd({"cmd": "saveProfile", "data": p})
    def load_profile(self, p): return self.cmd({"cmd": "loadProfile", "data": p})
    def delete_profile(self, p): return self.cmd({"cmd": "deleteProfile", "data": p})
    def reboot(self): self.cmd({"cmd": "reboot"})
    def list_profiles(self):
        r = self.cmd({"cmd": "listProfiles"})
        return r.get("data", []) if r else []
    def list_anim(self):
        r = self.cmd({"cmd": "animList"})
        return r.get("data", []) if r else []
    def play_anim(self, name):
        return self.cmd({"cmd": "animPlay", "data": name})
    def stop_anim(self):
        return self.cmd({"cmd": "animStop"})

conn         = MufukiConn()
profile_data = {}

# ─────────────────────────────────────────────
#  Helpers
# ─────────────────────────────────────────────
def styled_frame(parent, **kw):
    return tk.Frame(parent, bg=kw.pop("bg", PANEL),
                    highlightbackground=BORDER,
                    highlightthickness=1, **kw)

def label(parent, text, font=FONT_MONO, color=TEXT, **kw):
    return tk.Label(parent, text=text, font=font,
                    fg=color, bg=kw.pop("bg", parent["bg"]), **kw)

def accent_label(parent, text, **kw):
    return label(parent, text, color=ACCENT, font=FONT_HEAD, **kw)

def entry(parent, textvariable=None, width=12, **kw):
    return tk.Entry(parent, textvariable=textvariable, width=width,
                    bg=ENTRY_BG, fg=TEXT, insertbackground=ACCENT,
                    relief="flat", font=FONT_MONO,
                    highlightbackground=BORDER, highlightthickness=1, **kw)

def btn(parent, text, command=None, color=ACCENT, w=14, **kw):
    return tk.Button(parent, text=text, command=command,
                     bg=BTN_BG, fg=color, font=FONT_SMALL,
                     relief="flat", cursor="hand2", width=w,
                     activebackground=BTN_HOV, activeforeground=color,
                     highlightbackground=BORDER, highlightthickness=1, **kw)

def section(parent, title):
    f = tk.Frame(parent, bg=PANEL)
    f.pack(fill="x", padx=12, pady=(12, 4))
    tk.Label(f, text=f"── {title} ──", font=FONT_HEAD,
             fg=ACCENT, bg=PANEL).pack(anchor="w")
    return f

def dropdown(parent, var, options, width=16):
    cb = ttk.Combobox(parent, textvariable=var, values=options,
                      width=width, state="readonly", font=FONT_SMALL)
    return cb

def scrollable(parent):
    canvas = tk.Canvas(parent, bg=PANEL, highlightthickness=0)
    sb     = tk.Scrollbar(parent, orient="vertical", command=canvas.yview)
    inner  = tk.Frame(canvas, bg=PANEL)
    inner.bind("<Configure>",
               lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
    canvas.create_window((0, 0), window=inner, anchor="nw")
    canvas.configure(yscrollcommand=sb.set)
    canvas.pack(side="left", fill="both", expand=True)
    sb.pack(side="right", fill="y")
    for seq in ("<MouseWheel>", "<Button-4>", "<Button-5>"):
        canvas.bind(seq, lambda e: canvas.yview_scroll(
            -1 if e.num == 4 or e.delta > 0 else 1, "units"))
    return inner

status_var = None
def set_status(msg, color=TEXT_DIM):
    if status_var: status_var.set(msg)

# ─────────────────────────────────────────────
#  Tab: Main
# ─────────────────────────────────────────────
def build_main(parent):
    top = styled_frame(parent)
    top.pack(fill="x", padx=16, pady=(16, 8))

    accent_label(top, "  MUFUKI COMPANION", bg=PANEL).pack(anchor="w", padx=12, pady=(10,4))
    tk.Frame(top, bg=BORDER, height=1).pack(fill="x", padx=12, pady=2)

    conn_row = tk.Frame(top, bg=PANEL)
    conn_row.pack(fill="x", padx=12, pady=8)

    port_var = tk.StringVar()
    ports    = [p.device for p in serial.tools.list_ports.comports()]
    port_cb  = dropdown(conn_row, port_var, ports, width=18)
    port_cb.pack(side="left", padx=(0,6))
    if ports: port_var.set(ports[0])

    def refresh_ports():
        ps = [p.device for p in serial.tools.list_ports.comports()]
        port_cb["values"] = ps
        if ps: port_var.set(ps[0])

    status_dot = tk.Label(conn_row, text="●", font=FONT_HEAD, fg=RED,  bg=PANEL)
    status_dot.pack(side="left", padx=4)
    status_lbl = tk.Label(conn_row, text="Disconnected", font=FONT_SMALL,
                          fg=TEXT_DIM, bg=PANEL)
    status_lbl.pack(side="left", padx=4)

    info_frame = styled_frame(parent)

    def show_info():
        info_frame.pack(fill="x", padx=16, pady=4)
        refresh_info()

    def refresh_info():
        for w in info_frame.winfo_children(): w.destroy()
        if not conn.connected or not profile_data: return
        g = profile_data
        rows_data = [
            ("USB Mode",      USB_MODES[g.get("um",0)] if g.get("um",0)<3 else "?"),
            ("BLE",           "On" if g.get("bl") else "Off"),
            ("Input Handler", INPUT_HANDLERS[g.get("ih",0)] if g.get("ih",0)<3 else "?"),
            ("Actuation",     f'{g.get("at",0):.2f}'),
            ("Mappings",      str(len(g.get("mp",[])))),
        ]
        for k, v in rows_data:
            f = tk.Frame(info_frame, bg=PANEL); f.pack(fill="x", padx=12, pady=1)
            tk.Label(f, text=k, font=FONT_SMALL, fg=TEXT_DIM,
                     bg=PANEL, width=18, anchor="w").pack(side="left")
            tk.Label(f, text=v, font=FONT_SMALL, fg=ACCENT,
                     bg=PANEL).pack(side="left")

    def toggle_conn():
        if conn.connected:
            conn.disconnect()
            status_dot.config(fg=RED)
            status_lbl.config(text="Disconnected")
            conn_btn.config(text="Connect")
            set_status("Disconnected")
            info_frame.pack_forget()
        else:
            p = port_var.get()
            if not p:
                messagebox.showwarning("No port","Select a serial port first."); return
            set_status(f"Connecting to {p}...")
            if conn.connect(p):
                status_dot.config(fg=GREEN)
                status_lbl.config(text=f"Connected  {p}")
                conn_btn.config(text="Disconnect")
                set_status(f"Connected to {p}", GREEN)
                load_profile_from_device()
                show_info()
            else:
                messagebox.showerror("Failed", f"Cannot connect to {p}")
                set_status("Connection failed", RED)

    btn(conn_row, "↺ Refresh", refresh_ports, w=10).pack(side="left", padx=4)
    conn_btn = btn(conn_row, "Connect", toggle_conn, w=12)
    conn_btn.pack(side="left", padx=4)

    # device model
    model_frame = styled_frame(parent)
    model_frame.pack(fill="x", padx=16, pady=8)
    accent_label(model_frame, "  DEVICE", bg=PANEL).pack(anchor="w", padx=12, pady=(8,4))
    c = tk.Canvas(model_frame, width=380, height=200, bg="#080808", highlightthickness=0)
    c.pack(padx=12, pady=(0,12))

    def draw_device():
        c.delete("all")
        c.create_rectangle(20, 20, 360, 180, fill="#111111", outline="#333333", width=2)
        c.create_rectangle(30, 30, 100, 95, fill="#0a0a0a", outline="#444444", width=1, dash=(3,3))
        c.create_text(65, 62, text="S3", fill="#888888", font=("JetBrains Mono",12,"bold"))
        c.create_rectangle(115, 35, 255, 115, fill="#050510", outline="#333333", width=1)
        c.create_rectangle(120, 40, 250, 110, fill="#080815", outline=ACCENT, width=1)
        c.create_text(185, 75, text="OLED", fill=ACCENT, font=("JetBrains Mono",10))
        for i in range(3):
            x = 50 + i * 110
            c.create_rectangle(x, 130, x+80, 175, fill="#111111", outline="#444444", width=2)
            c.create_oval(x+33, 145, x+47, 159, fill="#080808", outline="#555555", width=1)
            c.create_text(x+40, 185, text=f"K{i+1}", fill=TEXT_DIM, font=FONT_SMALL)
        for i in range(4):
            y = 38 + i*22
            c.create_rectangle(265, y, 290, y+16, fill="#111111", outline="#333333", width=1)
        c.create_rectangle(30, 100, 100, 120, fill="#0a0a0a", outline="#333333", width=1)
        c.create_text(65, 110, text="MPU6050", fill=TEXT_DIM, font=("JetBrains Mono",7))

    draw_device()

    act = tk.Frame(parent, bg=BG)
    act.pack(fill="x", padx=16, pady=4)
    btn(act, "⟳  Reload Profile", lambda: load_profile_from_device(), w=18).pack(side="left", padx=4)
    btn(act, "⏻  Reboot Device",
        lambda: conn.reboot() if conn.connected and messagebox.askyesno("Reboot","Reboot Mufuki?") else None,
        color=RED, w=18).pack(side="left", padx=4)

# ─────────────────────────────────────────────
#  Tab: Basic
# ─────────────────────────────────────────────
v_ih = v_at = v_ws = v_ut = v_lt = v_dz = v_df = v_ft = None
v_ug = v_gt = v_rl = v_rb = v_dr = v_rs = v_ri = None
v_cr = v_cg = v_cb_col = None
ih_cb_ref = gt_cb_ref = ft_cb_ref = None

def build_basic(parent):
    global v_ih, v_at, v_ws, v_ut, v_lt, v_dz, v_df, v_ft
    global v_ug, v_gt, v_rl, v_rb, v_dr, v_rs, v_ri
    global v_cr, v_cg, v_cb_col, ih_cb_ref, gt_cb_ref, ft_cb_ref

    inner = scrollable(parent)

    v_ih     = tk.IntVar()
    v_at     = tk.DoubleVar(value=0.3)
    v_ws     = tk.DoubleVar(value=0.3)
    v_ut     = tk.DoubleVar(value=0.8)
    v_lt     = tk.DoubleVar(value=0.2)
    v_dz     = tk.IntVar()
    v_df     = tk.BooleanVar()
    v_ft     = tk.IntVar()
    v_ug     = tk.BooleanVar()
    v_gt     = tk.IntVar()
    v_rl     = tk.BooleanVar()
    v_rb     = tk.IntVar(value=128)
    v_dr     = tk.BooleanVar()
    v_rs     = tk.IntVar(value=1)
    v_ri     = tk.IntVar(value=5)
    v_cr     = tk.IntVar(value=255)
    v_cg     = tk.IntVar(value=255)
    v_cb_col = tk.IntVar(value=255)

    # ── Input Handler ──
    section(inner, "INPUT")

    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=4)
    tk.Label(f, text="Input Handler", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=22, anchor="w").pack(side="left")
    ih_var = tk.StringVar(value=INPUT_HANDLERS[0])
    ih_cb  = dropdown(f, ih_var, INPUT_HANDLERS, width=16)
    ih_cb.pack(side="left", padx=4)
    ih_cb_ref = ih_cb

    # frames per handler
    digital_frame = tk.Frame(inner, bg=PANEL)
    hyster_frame  = tk.Frame(inner, bg=PANEL)
    rt_frame      = tk.Frame(inner, bg=PANEL)

    def float_slider(parent_f, lbl, var, from_=0.0, to=1.0, res=0.01):
        f = tk.Frame(parent_f, bg=PANEL); f.pack(fill="x", padx=18, pady=2)
        tk.Label(f, text=lbl, font=FONT_SMALL, fg=TEXT_DIM,
                 bg=PANEL, width=22, anchor="w").pack(side="left")
        entry(f, textvariable=var, width=7).pack(side="left", padx=4)
        tk.Scale(f, variable=var, from_=from_, to=to, resolution=res,
                 orient="horizontal", bg=PANEL, fg=TEXT, troughcolor=ENTRY_BG,
                 highlightthickness=0, length=160, activebackground=ACCENT,
                 sliderlength=12).pack(side="left", padx=4)

    float_slider(digital_frame, "Actuation",       v_at)
    float_slider(hyster_frame,  "Upper Threshold",  v_ut)
    float_slider(hyster_frame,  "Lower Threshold",  v_lt)
    float_slider(rt_frame,      "Window Size",       v_ws)

    def on_handler_change(*_):
        idx = INPUT_HANDLERS.index(ih_var.get())
        v_ih.set(idx)
        digital_frame.pack_forget()
        hyster_frame.pack_forget()
        rt_frame.pack_forget()
        if idx == 0:   digital_frame.pack(fill="x")
        elif idx == 1: hyster_frame.pack(fill="x")
        else:          rt_frame.pack(fill="x")

    ih_cb.bind("<<ComboboxSelected>>", on_handler_change)
    digital_frame.pack(fill="x")

    # dead zone & filter
    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=4)
    tk.Label(f, text="Dead Zone", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=22, anchor="w").pack(side="left")
    entry(f, textvariable=v_dz, width=8).pack(side="left", padx=4)

    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=4)
    tk.Label(f, text="Filter", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=22, anchor="w").pack(side="left")
    tk.Checkbutton(f, text="Enable", variable=v_df, bg=PANEL, fg=TEXT,
                   selectcolor=ENTRY_BG, activebackground=PANEL,
                   font=FONT_SMALL).pack(side="left")
    ft_var = tk.StringVar(value="Oversampling")
    ft_cb  = dropdown(f, ft_var, ["Oversampling", "EMA"], width=14)
    ft_cb.pack(side="left", padx=8)
    ft_cb.bind("<<ComboboxSelected>>", lambda e: v_ft.set(ft_cb.current()))
    ft_cb_ref = ft_cb

    # ── Effects ──
    section(inner, "EFFECTS")

    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=2)
    tk.Label(f, text="Underglow", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=22, anchor="w").pack(side="left")
    tk.Checkbutton(f, text="Enable", variable=v_ug, bg=PANEL, fg=TEXT,
                   selectcolor=ENTRY_BG, activebackground=PANEL,
                   font=FONT_SMALL).pack(side="left")

    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=2)
    tk.Label(f, text="Glow Type", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=22, anchor="w").pack(side="left")
    gt_var = tk.StringVar(value=GLOW_TYPES[0])
    gt_cb  = dropdown(f, gt_var, GLOW_TYPES, width=16)
    gt_cb.pack(side="left", padx=4)
    gt_cb.bind("<<ComboboxSelected>>", lambda e: v_gt.set(gt_cb.current()))
    gt_cb_ref = gt_cb

    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=2)
    tk.Label(f, text="RGB", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=22, anchor="w").pack(side="left")
    tk.Checkbutton(f, text="Enable", variable=v_rl, bg=PANEL, fg=TEXT,
                   selectcolor=ENTRY_BG, activebackground=PANEL,
                   font=FONT_SMALL).pack(side="left")

    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=2)
    tk.Label(f, text="Rainbow", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=22, anchor="w").pack(side="left")
    tk.Checkbutton(f, text="Enable", variable=v_dr, bg=PANEL, fg=TEXT,
                   selectcolor=ENTRY_BG, activebackground=PANEL,
                   font=FONT_SMALL).pack(side="left")

    # RGB color sliders + preview + picker
    section(inner, "RGB COLOR")

    color_preview = tk.Label(inner, text="         ", bg="#ffffff",
                             relief="flat", height=2)
    color_preview.pack(fill="x", padx=18, pady=(0,6))

    def update_preview(*_):
        r2 = v_cr.get(); g2 = v_cg.get(); b2 = v_cb_col.get()
        color_preview.config(bg=f"#{r2:02x}{g2:02x}{b2:02x}")

    def open_picker():
        rgb, _ = colorchooser.askcolor(
            color=f"#{v_cr.get():02x}{v_cg.get():02x}{v_cb_col.get():02x}",
            parent=parent, title="Pick RGB Color")
        if rgb:
            v_cr.set(int(rgb[0])); v_cg.set(int(rgb[1])); v_cb_col.set(int(rgb[2]))
            update_preview()

    for lbl, var, ch in [("R", v_cr, "#ff5555"), ("G", v_cg, "#55ff55"), ("B", v_cb_col, "#5599ff")]:
        f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=2)
        tk.Label(f, text=lbl, font=FONT_SMALL, fg=ch,
                 bg=PANEL, width=4, anchor="w").pack(side="left")
        entry(f, textvariable=var, width=5).pack(side="left", padx=4)
        tk.Scale(f, variable=var, from_=0, to=255, resolution=1,
                 orient="horizontal", bg=PANEL, fg=TEXT, troughcolor=ENTRY_BG,
                 highlightthickness=0, length=180, activebackground=ACCENT,
                 sliderlength=12, command=update_preview).pack(side="left", padx=4)

    btn(inner, "🎨  Color Picker", open_picker, w=18).pack(padx=18, pady=4, anchor="w")

    # RGB settings
    section(inner, "RGB SETTINGS")
    for lbl, var in [("RGB Brightness", v_rb), ("Rainbow Step", v_rs), ("RGB Interval", v_ri)]:
        f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=2)
        tk.Label(f, text=lbl, font=FONT_SMALL, fg=TEXT_DIM,
                 bg=PANEL, width=22, anchor="w").pack(side="left")
        entry(f, textvariable=var, width=5).pack(side="left", padx=4)
        tk.Scale(f, variable=var, from_=0, to=255, resolution=1,
                 orient="horizontal", bg=PANEL, fg=TEXT, troughcolor=ENTRY_BG,
                 highlightthickness=0, length=160, activebackground=ACCENT,
                 sliderlength=12).pack(side="left", padx=4)

    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=12)

    def apply_basic():
        if not conn.connected:
            messagebox.showwarning("Not connected","Connect first."); return
        d = profile_data.copy()
        d.update({
            "ih": v_ih.get(), "at": v_at.get(), "ws": v_ws.get(),
            "ut": v_ut.get(), "lt": v_lt.get(), "dz": v_dz.get(),
            "df": v_df.get(), "ft": v_ft.get(),
            "ug": v_ug.get(), "gt": v_gt.get(), "rl": v_rl.get(),
            "rb": v_rb.get(), "dr": v_dr.get(),
            "rs": v_rs.get(), "ri": v_ri.get(),
            "rc": [v_cr.get(), v_cg.get(), v_cb_col.get()],
        })
        r = conn.set_profile(d)
        if r and r.get("ok"):
            profile_data.update(d); set_status("Basic settings applied", GREEN)
        else:
            set_status("Failed to apply", RED)

    btn(f, "✓  Apply", apply_basic, color=ACCENT, w=16).pack(side="left")

# ─────────────────────────────────────────────
#  Tab: USB
# ─────────────────────────────────────────────
v_um = v_bl = v_bn = None
um_cb_ref = None

def build_usb(parent):
    global v_um, v_bl, v_bn, um_cb_ref

    inner = scrollable(parent)
    v_um  = tk.IntVar()
    v_bl  = tk.BooleanVar()
    v_bn  = tk.StringVar()

    section(inner, "USB MODE")
    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=4)
    tk.Label(f, text="Device Mode", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=22, anchor="w").pack(side="left")
    um_var = tk.StringVar(value=USB_MODES[0])
    um_cb  = dropdown(f, um_var, USB_MODES, width=14)
    um_cb.pack(side="left", padx=4)
    um_cb.bind("<<ComboboxSelected>>", lambda e: v_um.set(um_cb.current()))
    um_cb_ref = um_cb

    tk.Label(inner, text="  ⚠  Changing USB mode requires device reboot",
             font=FONT_SMALL, fg=ACCENT2, bg=PANEL).pack(anchor="w", padx=18, pady=2)

    section(inner, "BLUETOOTH")
    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=4)
    tk.Label(f, text="BLE Enable", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=22, anchor="w").pack(side="left")
    tk.Checkbutton(f, text="", variable=v_bl, bg=PANEL,
                   selectcolor=ENTRY_BG, activebackground=PANEL).pack(side="left")

    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=4)
    tk.Label(f, text="BLE Name", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=22, anchor="w").pack(side="left")
    entry(f, textvariable=v_bn, width=24).pack(side="left", padx=4)

    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=12)

    def apply_usb():
        if not conn.connected:
            messagebox.showwarning("Not connected","Connect first."); return
        new_mode = v_um.get()
        old_mode = profile_data.get("um", 0)
        d = profile_data.copy()
        d.update({"um": new_mode, "bl": v_bl.get(), "bn": v_bn.get()})
        r = conn.set_profile(d)
        if r and r.get("ok"):
            profile_data.update(d)
            if new_mode != old_mode:
                if messagebox.askyesno("Reboot required",
                    "USB mode changed. Reboot Mufuki now?"):
                    conn.reboot()
            set_status("USB config applied", GREEN)
        else:
            set_status("Failed", RED)

    btn(f, "✓  Apply", apply_usb, color=ACCENT, w=16).pack(side="left")

# ─────────────────────────────────────────────
#  Layout / Mapping helpers
# ─────────────────────────────────────────────
KEY_NAMES = [
    "NONE","A","B","C","D","E","F","G","H","I","J","K","L","M",
    "N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
    "1","2","3","4","5","6","7","8","9","0",
    "ENTER","ESCAPE","BACKSPACE","TAB","SPACE",
    "MINUS","EQUAL","BRACKET_LEFT","BRACKET_RIGHT",
    "BACKSLASH","SEMICOLON","APOSTROPHE","GRAVE",
    "COMMA","PERIOD","SLASH","CAPS_LOCK",
    "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12",
    "ARROW_RIGHT","ARROW_LEFT","ARROW_DOWN","ARROW_UP",
]
KEY_CODES = [
    0x00,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,
    0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,
    0x2D,0x2E,0x2F,0x30,0x31,0x33,0x34,0x35,0x36,0x37,0x38,0x3A,0x3B,0x3C,
    0x3D,0x3E,0x3F,0x40,0x41,0x42,0x43,0x44,0x4F,0x50,0x51,0x52,
]
SRC_LABELS      = ["Hall0","Hall1","Hall2","Btn0","Btn1","Btn2",
                   "GyroX","GyroY","GyroZ","AccX","AccY","AccZ",
                   "AngleX","AngleY","AngleZ"]
DST_AXIS_LABELS = ["LX","LY","RX","RY","LT","RT","MouseX","MouseY","MouseWheel"]
DST_THR_LABELS  = ["Key","GPButton","MouseBtn"]

def code_to_name(code):
    try: return KEY_NAMES[KEY_CODES.index(code)]
    except: return "NONE"

def name_to_code(name):
    try: return KEY_CODES[KEY_NAMES.index(name)]
    except: return 0

# ─────────────────────────────────────────────
#  Tab: Layout / Mapping
# ─────────────────────────────────────────────
rebuild_lm_ref = None

def build_layout_mapping(parent):
    global rebuild_lm_ref
    container = tk.Frame(parent, bg=BG)
    container.pack(fill="both", expand=True)

    def rebuild(mode=None):
        for w in container.winfo_children(): w.destroy()
        m = mode if mode is not None else profile_data.get("um", 0)
        if m == 0: build_layout_tab(container)
        else:      build_mapping_tab(container)

    rebuild_lm_ref = rebuild
    rebuild()
    return container

def build_layout_tab(parent):
    inner    = scrollable(parent)
    section(inner, "KEYBOARD LAYOUT")

    lo       = profile_data.get("lo", [0]*6)
    key_vars = []
    labels   = ["Hall 0", "Hall 1", "Hall 2", "F1", "F2", "F3"]

    for i, lbl in enumerate(labels):
        f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=3)
        tk.Label(f, text=lbl, font=FONT_SMALL, fg=TEXT_DIM,
                 bg=PANEL, width=20, anchor="w").pack(side="left")
        var = tk.StringVar(value=code_to_name(lo[i] if i < len(lo) else 0))
        dropdown(f, var, KEY_NAMES, width=18).pack(side="left", padx=4)
        key_vars.append(var)

    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=12)

    def apply_layout():
        if not conn.connected:
            messagebox.showwarning("Not connected","Connect first."); return
        d = profile_data.copy()
        d["lo"] = [name_to_code(v.get()) for v in key_vars]
        r = conn.set_profile(d)
        if r and r.get("ok"):
            profile_data.update(d); set_status("Layout applied", GREEN)
        else:
            set_status("Failed", RED)

    btn(f, "✓  Apply Layout", apply_layout, color=ACCENT, w=18).pack(side="left")

def build_mapping_tab(parent):
    inner    = scrollable(parent)
    mappings = profile_data.get("mp", [])

    section(inner, "MAPPING RULES")
    list_frame = tk.Frame(inner, bg=PANEL)
    list_frame.pack(fill="x", padx=12, pady=4)

    def refresh_list():
        for w in list_frame.winfo_children(): w.destroy()
        if not mappings:
            tk.Label(list_frame, text="No mappings defined.",
                     font=FONT_SMALL, fg=TEXT_DIM, bg=PANEL).pack(padx=12, pady=8)
            return
        for i, m in enumerate(mappings):
            mf = tk.Frame(list_frame, bg=ENTRY_BG,
                          highlightbackground=BORDER, highlightthickness=1)
            mf.pack(fill="x", padx=4, pady=2)
            src = SRC_LABELS[m["s"]] if m["s"] < len(SRC_LABELS) else "?"
            if m["ax"]:
                d_idx = m.get("d", 1) - 1
                dst   = DST_AXIS_LABELS[d_idx] if 0 <= d_idx < len(DST_AXIS_LABELS) else "?"
                info  = f"{src} → {dst}  [{m.get('a',0):.0f}~{m.get('b',0):.0f}]→[{m.get('c',0):.0f}~{m.get('d2',0):.0f}]"
            else:
                d_idx = m.get("d", 0)
                dst   = DST_THR_LABELS[d_idx] if d_idx < len(DST_THR_LABELS) else "?"
                info  = f"{src} → {dst}  key={code_to_name(m.get('kc',0))}  pos>{m.get('a',0):.2f}"
            tk.Label(mf, text=f"  [{i}]  {info}", font=FONT_SMALL,
                     fg=TEXT, bg=ENTRY_BG, anchor="w").pack(
                     side="left", fill="x", expand=True, padx=4, pady=4)

            def del_m(idx=i):
                mappings.pop(idx)
                d = profile_data.copy(); d["mp"] = mappings
                r = conn.set_profile(d)
                if r and r.get("ok"):
                    profile_data.update(d); refresh_list()
            btn(mf, "✕", del_m, color=RED, w=3).pack(side="right", padx=4, pady=2)

    refresh_list()

    section(inner, "ADD MAPPING")
    add_frame = tk.Frame(inner, bg=PANEL)
    add_frame.pack(fill="x", padx=12, pady=4)

    type_var    = tk.StringVar(value="Axis")
    src_var     = tk.StringVar(value=SRC_LABELS[0])
    dst_ax_var  = tk.StringVar(value=DST_AXIS_LABELS[0])
    dst_thr_var = tk.StringVar(value=DST_THR_LABELS[0])
    key_var     = tk.StringVar(value="SPACE")
    in_min      = tk.DoubleVar(value=0.0)
    in_max      = tk.DoubleVar(value=1.0)
    out_min     = tk.DoubleVar(value=-127.0)
    out_max     = tk.DoubleVar(value=127.0)
    pos_thr     = tk.DoubleVar(value=0.5)
    neg_thr     = tk.DoubleVar(value=0.0)
    abs_thr     = tk.DoubleVar(value=0.0)
    clamp_var   = tk.BooleanVar(value=True)
    cmb_var     = tk.StringVar(value="None")

    def lrow(lbl, *widgets):
        f = tk.Frame(add_frame, bg=PANEL); f.pack(fill="x", pady=2)
        tk.Label(f, text=lbl, font=FONT_SMALL, fg=TEXT_DIM,
                 bg=PANEL, width=16, anchor="w").pack(side="left")
        for w in widgets: w(f).pack(side="left", padx=3)

    lrow("Type",   lambda p: dropdown(p, type_var, ["Axis","Threshold"], width=12))
    lrow("Source", lambda p: dropdown(p, src_var, SRC_LABELS, width=12))

    ax_frame = tk.Frame(add_frame, bg=PANEL)
    for lbl, var in [("In Min",in_min),("In Max",in_max),("Out Min",out_min),("Out Max",out_max)]:
        f = tk.Frame(ax_frame, bg=PANEL); f.pack(fill="x", pady=1)
        tk.Label(f, text=lbl, font=FONT_SMALL, fg=TEXT_DIM,
                 bg=PANEL, width=16, anchor="w").pack(side="left")
        entry(f, textvariable=var, width=8).pack(side="left", padx=3)
    f = tk.Frame(ax_frame, bg=PANEL); f.pack(fill="x", pady=1)
    tk.Label(f, text="Clamp", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=16, anchor="w").pack(side="left")
    tk.Checkbutton(f, variable=clamp_var, bg=PANEL,
                   selectcolor=ENTRY_BG, activebackground=PANEL).pack(side="left")
    f2 = tk.Frame(ax_frame, bg=PANEL); f2.pack(fill="x", pady=1)
    tk.Label(f2, text="Dst Axis", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=16, anchor="w").pack(side="left")
    dropdown(f2, dst_ax_var, DST_AXIS_LABELS, width=12).pack(side="left", padx=3)
    f3 = tk.Frame(ax_frame, bg=PANEL); f3.pack(fill="x", pady=1)
    tk.Label(f3, text="Combine", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=16, anchor="w").pack(side="left")
    dropdown(f3, cmb_var, ["None","+","-"], width=8).pack(side="left", padx=3)

    thr_frame = tk.Frame(add_frame, bg=PANEL)
    for lbl, var in [("Pos Thresh",pos_thr),("Neg Thresh",neg_thr),("Abs Thresh",abs_thr)]:
        f = tk.Frame(thr_frame, bg=PANEL); f.pack(fill="x", pady=1)
        tk.Label(f, text=lbl, font=FONT_SMALL, fg=TEXT_DIM,
                 bg=PANEL, width=16, anchor="w").pack(side="left")
        entry(f, textvariable=var, width=8).pack(side="left", padx=3)
    f = tk.Frame(thr_frame, bg=PANEL); f.pack(fill="x", pady=1)
    tk.Label(f, text="Dst Target", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=16, anchor="w").pack(side="left")
    dropdown(f, dst_thr_var, DST_THR_LABELS, width=12).pack(side="left", padx=3)
    f2 = tk.Frame(thr_frame, bg=PANEL); f2.pack(fill="x", pady=1)
    tk.Label(f2, text="Keycode", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=16, anchor="w").pack(side="left")
    dropdown(f2, key_var, KEY_NAMES, width=18).pack(side="left", padx=3)

    def toggle_type(*_):
        if type_var.get() == "Axis":
            thr_frame.pack_forget(); ax_frame.pack(fill="x")
        else:
            ax_frame.pack_forget(); thr_frame.pack(fill="x")

    type_var.trace("w", toggle_type)
    ax_frame.pack(fill="x")

    def add_mapping():
        if not conn.connected:
            messagebox.showwarning("Not connected","Connect first."); return
        src_i = SRC_LABELS.index(src_var.get())
        cmb_i = {"None":0,"+":1,"-":2}.get(cmb_var.get(), 0)
        if type_var.get() == "Axis":
            dst_i = DST_AXIS_LABELS.index(dst_ax_var.get()) + 1
            m = {"s":src_i,"d":dst_i,"ax":True,"cb":cmb_i,"kc":0,
                 "a":in_min.get(),"b":in_max.get(),
                 "c":out_min.get(),"d2":out_max.get(),"cl":clamp_var.get()}
        else:
            dst_i = DST_THR_LABELS.index(dst_thr_var.get())
            m = {"s":src_i,"d":dst_i,"ax":False,"cb":0,"kc":name_to_code(key_var.get()),
                 "a":pos_thr.get(),"b":neg_thr.get(),"c":abs_thr.get(),"d2":0.0,"cl":False}
        mappings.append(m)
        d = profile_data.copy(); d["mp"] = mappings
        r = conn.set_profile(d)
        if r and r.get("ok"):
            profile_data.update(d); set_status("Mapping added", GREEN); refresh_list()
        else:
            set_status("Failed", RED)

    btn(add_frame, "+ Add Mapping", add_mapping, color=ACCENT, w=18).pack(pady=8)

# ─────────────────────────────────────────────
#  Tab: Profile
# ─────────────────────────────────────────────
def build_profile(parent):
    inner = scrollable(parent)
    section(inner, "DEVICE PROFILES")

    listbox = tk.Listbox(inner, bg=ENTRY_BG, fg=TEXT, font=FONT_MONO,
                         selectbackground=ACCENT, selectforeground=BG,
                         relief="flat", highlightthickness=1,
                         highlightbackground=BORDER, height=7)
    listbox.pack(fill="x", padx=18, pady=4)

    def refresh_prf():
        listbox.delete(0,"end")
        if not conn.connected: return
        for p in conn.list_profiles(): listbox.insert("end", p)

    btns = tk.Frame(inner, bg=PANEL); btns.pack(fill="x", padx=18, pady=4)

    def do_load():
        sel = listbox.curselection()
        if not sel: return
        path = listbox.get(sel[0])
        if not path.startswith("/"): path = "/" + path
        r = conn.load_profile(path)
        if r and r.get("ok"): load_profile_from_device(); set_status(f"Loaded {path}", GREEN)
        else: set_status("Load failed", RED)

    def do_delete():
        sel = listbox.curselection()
        if not sel: return
        path = listbox.get(sel[0])
        if not path.startswith("/"): path = "/" + path
        if messagebox.askyesno("Delete", f"Delete {path}?"):
            r = conn.delete_profile(path)
            if r and r.get("ok"): set_status(f"Deleted {path}", GREEN); refresh_prf()
            else: set_status("Delete failed", RED)

    btn(btns, "↺ Refresh", refresh_prf,  w=12).pack(side="left", padx=4)
    btn(btns, "⬇ Load",    do_load,      w=10).pack(side="left", padx=4)
    btn(btns, "✕ Delete",  do_delete, color=RED, w=10).pack(side="left", padx=4)

    section(inner, "SAVE AS")
    sf = tk.Frame(inner, bg=PANEL); sf.pack(fill="x", padx=18, pady=4)
    save_var = tk.StringVar(value="myprofile")
    entry(sf, textvariable=save_var, width=24).pack(side="left", padx=(0,8))

    def do_save():
        path = save_var.get().strip()
        if not path: return
        if not path.startswith("/"): path = "/" + path
        if not path.endswith(".json"): path += ".json"
        r = conn.save_profile(path)
        if r and r.get("ok"): set_status(f"Saved {path}", GREEN); refresh_prf()
        else: set_status("Save failed", RED)

    btn(sf, "⬆ Save to Device", do_save, w=18).pack(side="left")

    section(inner, "EXPORT / IMPORT")
    ef = tk.Frame(inner, bg=PANEL); ef.pack(fill="x", padx=18, pady=4)

    def export_json():
        if not profile_data: messagebox.showwarning("Empty","No profile loaded."); return
        path = filedialog.asksaveasfilename(defaultextension=".json",
                                            filetypes=[("JSON","*.json")])
        if path:
            with open(path,"w") as f: json.dump(profile_data, f, indent=2)
            set_status("Exported", GREEN)

    def import_json():
        path = filedialog.askopenfilename(filetypes=[("JSON","*.json")])
        if path:
            with open(path) as f: d = json.load(f)
            if conn.connected:
                r = conn.set_profile(d)
                if r and r.get("ok"): profile_data.update(d); set_status("Imported", GREEN)
            else:
                profile_data.update(d); set_status("Imported (offline)", ACCENT2)

    btn(ef, "⬆ Export JSON", export_json, w=16).pack(side="left", padx=4)
    btn(ef, "⬇ Import JSON", import_json, w=16).pack(side="left", padx=4)

    # ── Animation ──
    section(inner, "ANIMATION")

    anim_lb = tk.Listbox(inner, bg=ENTRY_BG, fg=TEXT, font=FONT_MONO,
                         selectbackground=ACCENT, selectforeground=BG,
                         relief="flat", highlightthickness=1,
                         highlightbackground=BORDER, height=5)
    anim_lb.pack(fill="x", padx=18, pady=4)

    def refresh_anim():
        anim_lb.delete(0,"end")
        if not conn.connected: return
        for a in conn.list_anim(): anim_lb.insert("end", a)

    anim_btns = tk.Frame(inner, bg=PANEL); anim_btns.pack(fill="x", padx=18, pady=4)

    def do_play():
        sel = anim_lb.curselection()
        if not sel: return
        name = anim_lb.get(sel[0])
        r = conn.play_anim(name)
        if r and r.get("ok"): set_status(f"Playing {name}", GREEN)
        else: set_status("Play failed", RED)

    def do_stop():
        r = conn.stop_anim()
        if r and r.get("ok"): set_status("Animation stopped", ACCENT2)

    btn(anim_btns, "↺ Refresh", refresh_anim, w=12).pack(side="left", padx=4)
    btn(anim_btns, "▶ Play",    do_play,      w=10).pack(side="left", padx=4)
    btn(anim_btns, "■ Stop",    do_stop, color=ACCENT2, w=10).pack(side="left", padx=4)

# ─────────────────────────────────────────────
#  Tab: More
# ─────────────────────────────────────────────
v_sb = v_ss = v_so = v_lg = v_sl = None

def build_more(parent):
    global v_sb, v_ss, v_so, v_lg, v_sl
    inner = scrollable(parent)
    v_sb = tk.IntVar(value=200)
    v_ss = tk.IntVar(value=30000)
    v_so = tk.IntVar(value=60000)
    v_lg = tk.IntVar()
    v_sl = tk.StringVar()

    section(inner, "DISPLAY")
    for lbl, var, lo, hi in [
        ("Screen Brightness", v_sb, 0, 255),
        ("Screensaver (ms)",  v_ss, 0, 300000),
        ("Screen Off (ms)",   v_so, 0, 300000),
        ("Logo Type",         v_lg, 0, 12),
    ]:
        f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=2)
        tk.Label(f, text=lbl, font=FONT_SMALL, fg=TEXT_DIM,
                 bg=PANEL, width=24, anchor="w").pack(side="left")
        entry(f, textvariable=var, width=8).pack(side="left", padx=4)
        tk.Scale(f, variable=var, from_=lo, to=hi, orient="horizontal",
                 bg=PANEL, fg=TEXT, troughcolor=ENTRY_BG, highlightthickness=0,
                 length=160, activebackground=ACCENT,
                 sliderlength=12).pack(side="left", padx=4)

    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=4)
    tk.Label(f, text="Screen Logo", font=FONT_SMALL, fg=TEXT_DIM,
             bg=PANEL, width=24, anchor="w").pack(side="left")
    entry(f, textvariable=v_sl, width=24).pack(side="left", padx=4)

    f = tk.Frame(inner, bg=PANEL); f.pack(fill="x", padx=18, pady=12)

    def apply_more():
        if not conn.connected:
            messagebox.showwarning("Not connected","Connect first."); return
        d = profile_data.copy()
        d.update({"sb":v_sb.get(),"ss":v_ss.get(),"so":v_so.get(),
                  "lg":v_lg.get(),"sl":v_sl.get()})
        r = conn.set_profile(d)
        if r and r.get("ok"):
            profile_data.update(d); set_status("Display settings applied", GREEN)
        else:
            set_status("Failed", RED)

    btn(f, "✓  Apply", apply_more, color=ACCENT, w=16).pack(side="left")

# ─────────────────────────────────────────────
#  Sync profile → UI
# ─────────────────────────────────────────────
def load_profile_from_device():
    global profile_data
    if not conn.connected: return
    r = conn.get_profile()
    if r and r.get("ok"):
        profile_data = r.get("data", {})
        sync_ui()
        set_status("Profile loaded", GREEN)

def sync_ui():
    g = profile_data
    if not g: return
    ih = g.get("ih", 0)
    if v_ih and ih_cb_ref:
        v_ih.set(ih); ih_cb_ref.current(ih)
        ih_cb_ref.event_generate("<<ComboboxSelected>>")
    if v_at:     v_at.set(g.get("at", 0.3))
    if v_ws:     v_ws.set(g.get("ws", 0.3))
    if v_ut:     v_ut.set(g.get("ut", 0.8))
    if v_lt:     v_lt.set(g.get("lt", 0.2))
    if v_dz:     v_dz.set(g.get("dz", 0))
    if v_df:     v_df.set(g.get("df", False))
    if v_ft and ft_cb_ref:
        v_ft.set(g.get("ft", 0)); ft_cb_ref.current(min(g.get("ft",0), 1))
    if v_ug:     v_ug.set(g.get("ug", False))
    if v_gt and gt_cb_ref:
        v_gt.set(g.get("gt", 0)); gt_cb_ref.current(g.get("gt", 0))
    if v_rl:     v_rl.set(g.get("rl", False))
    if v_rb:     v_rb.set(g.get("rb", 128))
    if v_dr:     v_dr.set(g.get("dr", False))
    if v_rs:     v_rs.set(g.get("rs", 1))
    if v_ri:     v_ri.set(g.get("ri", 5))
    rc = g.get("rc", [255,255,255])
    if v_cr:     v_cr.set(rc[0] if len(rc)>0 else 255)
    if v_cg:     v_cg.set(rc[1] if len(rc)>1 else 255)
    if v_cb_col: v_cb_col.set(rc[2] if len(rc)>2 else 255)
    if v_um and um_cb_ref:
        v_um.set(g.get("um",0)); um_cb_ref.current(g.get("um",0))
    if v_bl:     v_bl.set(g.get("bl", False))
    if v_bn:     v_bn.set(g.get("bn", "Mufuki"))
    if v_sb:     v_sb.set(g.get("sb", 200))
    if v_ss:     v_ss.set(g.get("ss", 30000))
    if v_so:     v_so.set(g.get("so", 60000))
    if v_lg:     v_lg.set(g.get("lg", 0))
    if v_sl:     v_sl.set(g.get("sl", ""))
    if rebuild_lm_ref: rebuild_lm_ref(g.get("um", 0))

# ─────────────────────────────────────────────
#  Main
# ─────────────────────────────────────────────
def main():
    global status_var, v_ih

    root = tk.Tk()
    root.title("Mufuki Companion")
    root.configure(bg=BG)
    root.geometry("740x620")
    root.minsize(640, 500)

    style = ttk.Style()
    style.theme_use("clam")
    style.configure("TNotebook",     background=BG,    borderwidth=0)
    style.configure("TNotebook.Tab", background=PANEL, foreground=TEXT_DIM,
                    font=FONT_SMALL, padding=[14,6],   borderwidth=0)
    style.map("TNotebook.Tab",
              background=[("selected", BG)],
              foreground=[("selected", ACCENT)])
    style.configure("TCombobox",
                    fieldbackground=ENTRY_BG, background=ENTRY_BG,
                    foreground=TEXT, selectbackground=ACCENT, selectforeground=BG,
                    arrowcolor=ACCENT)
    style.map("TCombobox",
              fieldbackground=[("readonly", ENTRY_BG)],
              foreground=[("readonly", TEXT)])
    root.option_add("*TCombobox*Listbox.background", ENTRY_BG)
    root.option_add("*TCombobox*Listbox.foreground", TEXT)
    root.option_add("*TCombobox*Listbox.selectBackground", ACCENT)
    root.option_add("*TCombobox*Listbox.selectForeground", BG)
    root.option_add("*TCombobox*Listbox.font", FONT_SMALL)

    hdr = tk.Frame(root, bg=PANEL, height=40)
    hdr.pack(fill="x"); hdr.pack_propagate(False)
    tk.Label(hdr, text="  ◈  MUFUKI", font=FONT_TITLE,
             fg=ACCENT, bg=PANEL).pack(side="left", padx=8)
    tk.Label(hdr, text="companion v3", font=FONT_SMALL,
             fg=TEXT_DIM, bg=PANEL).pack(side="left")

    nb = ttk.Notebook(root)
    nb.pack(fill="both", expand=True)

    tabs = {}
    for name in ["Main","Basic","USB","Layout/Mapping","Profile","More"]:
        f = tk.Frame(nb, bg=BG)
        nb.add(f, text=f"  {name}  ")
        tabs[name] = f

    v_ih = tk.IntVar()

    build_main(tabs["Main"])
    build_basic(tabs["Basic"])
    build_usb(tabs["USB"])
    build_layout_mapping(tabs["Layout/Mapping"])
    build_profile(tabs["Profile"])
    build_more(tabs["More"])

    status_var = tk.StringVar(value="Not connected")
    sb = tk.Frame(root, bg=PANEL, height=24)
    sb.pack(fill="x", side="bottom"); sb.pack_propagate(False)
    tk.Label(sb, textvariable=status_var, font=FONT_SMALL,
             fg=TEXT_DIM, bg=PANEL, anchor="w").pack(side="left", padx=10)

    root.mainloop()

if __name__ == "__main__":
    main()