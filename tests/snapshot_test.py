#!/usr/bin/env python3

import glob
import os
import re
import subprocess
import time

from PIL import Image


def find_available_display():
    """Find an available X display number by checking /tmp/.X*-lock files"""
    existing_displays = []

    lock_files = glob.glob("/tmp/.X*-lock")
    for lock_file in lock_files:
        display_num = lock_file.split(".X")[1].split("-")[0]
        if display_num.isdigit():
            existing_displays.append(int(display_num))

    display_num = 0
    while display_num in existing_displays:
        display_num += 1

    return display_num


os.makedirs("tests/out", exist_ok=True)

display_num = find_available_display()
display_str = f":{display_num}"
print(f"Starting Xvfb on display {display_str}")

xvfb_cmd = ["Xvfb", display_str, "-screen", "0", "1024x768x24"]
xvfb_process = subprocess.Popen(
    xvfb_cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
)

time.sleep(2)

os.environ["DISPLAY"] = display_str

project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
build_dir = os.path.join(project_root, "build")

os.environ["LD_LIBRARY_PATH"] = build_dir
os.environ["VK_LAYER_PATH"] = build_dir
os.environ["VK_LOADER_LAYERS_ENABLE"] = "VK_LAYER_VKVFB_vkvfb"

env = os.environ.copy()
vkcube_process = subprocess.Popen(
    ["vkcube", "--width", "640", "--height", "480"],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    env=env,
)

time.sleep(2)

try:
    out, _ = vkcube_process.communicate(timeout=2)
    print(f"vkcube exited with code: {vkcube_process.returncode}")
    print(out.decode())
    xvfb_process.terminate()
    exit(1)
except subprocess.TimeoutExpired:
    pass


def find_pixbuf_path():
    """Find a pixbuf shared memory path that matches our pixbuf pattern."""
    pixbuf_pattern = r"0x\d+"
    shm_dir = "/dev/shm"
    if not os.path.exists(shm_dir):
        return None
    for filename in os.listdir(shm_dir):
        if re.match(pixbuf_pattern, filename):
            return filename
    return None


pixbuf_path = find_pixbuf_path()
if not pixbuf_path:
    print("No matching pixbuf path found in /dev/shm")
    vkcube_process.terminate()
    xvfb_process.terminate()
    exit(1)

print(f"Found pixbuf path: {pixbuf_path}")

snapshot_exe = os.path.join(build_dir, "snapshot_vfb")
result = subprocess.run([snapshot_exe, pixbuf_path], capture_output=True, text=True)

if result.returncode == 0:
    print("Snapshot captured successfully:")
    print(result.stdout.strip())

    # Convert raw data to PNG
    try:
        with open("tests/out/snapshot_dim.txt", "r") as f:
            width, height = map(int, f.read().strip().split())

        with open("tests/out/snapshot.raw", "rb") as f:
            raw_data = f.read()

        img = Image.frombytes("RGBA", (width, height), raw_data)
        img.save("tests/out/snapshot.png")
        print("Converted to PNG: tests/out/snapshot.png")

    except Exception as e:
        print(f"Failed to convert to PNG: {e}")

else:
    print(f"Snapshot failed with return code {result.returncode}")
    if result.stderr:
        print("Error:", result.stderr.strip())

vkcube_process.terminate()
xvfb_process.terminate()
