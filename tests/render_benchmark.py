# Rendering benchmark for comparing CPU usage between native desktop and Xvfb environments
#
# Two modes:
# 1. Native: Runs vkcube on the current desktop environment
# 2. Xvfb: Finds an available X display, starts Xvfb server, and runs vkcube under it
#
# The script monitors vkcube's CPU usage and logs the results to files in the out/ directory.
# In Xvfb mode, the script automatically manages the virtual display server lifecycle.
#
# Usage:
# $ python tests/render_benchmark.py native

import argparse
import glob
import os
import subprocess
import time


def find_available_display():
    """Find an available X display number by checking /tmp/.X*-lock files"""
    existing_displays = []

    # Check for existing X lock files
    lock_files = glob.glob("/tmp/.X*-lock")
    for lock_file in lock_files:
        display_num = lock_file.split(".X")[1].split("-")[0]
        if display_num.isdigit():
            existing_displays.append(int(display_num))

    # Find the next available display number
    display_num = 0
    while display_num in existing_displays:
        display_num += 1

    return display_num


parser = argparse.ArgumentParser()
parser.add_argument(
    "mode", help="A string saying which mode to run in. Must be either native or xvfb."
)
args = parser.parse_args()

if args.mode.lower() not in ["native", "xvfb"]:
    print(f"Unrecognized mode: {args.mode}")
    exit()

# Ensure output directory exists
os.makedirs("tests/out", exist_ok=True)

mode = args.mode.lower()
xvfb_process = None

# If running in Xvfb mode, start Xvfb server
if mode == "xvfb":
    display_num = find_available_display()
    display_str = f":{display_num}"
    print(f"Starting Xvfb on display {display_str}")

    xvfb_cmd = ["Xvfb", display_str, "-screen", "0", "1024x768x24"]
    xvfb_process = subprocess.Popen(
        xvfb_cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )

    # Give Xvfb time to start
    time.sleep(2)

    # Set DISPLAY environment variable
    os.environ["DISPLAY"] = display_str

    # Set vkvfb layer environment variables for xvfb mode
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    build_dir = os.path.join(project_root, "build")

    os.environ["LD_LIBRARY_PATH"] = build_dir
    os.environ["VK_LAYER_PATH"] = build_dir
    os.environ["VK_LOADER_LAYERS_ENABLE"] = "VK_LAYER_VKVFB_vkvfb"

with open(f"tests/out/out_server_{args.mode}.txt", "w") as f:
    f.write(f"Running test for server: {args.mode}\n\n")
    f.flush()
    env = os.environ.copy()
    p = subprocess.Popen(
        ["vkcube", "--width", "640", "--height", "480"], stdout=f, stderr=f, env=env
    )
    time.sleep(5)

    vkcube_ret = p.poll()
    if vkcube_ret:
        f.write(f"vkcube exited with code: {vkcube_ret}\n")
        f.flush()
        exit()

    ps_out = subprocess.run(
        ["ps", "--no-headers", "-p", str(p.pid), "-o", "%cpu"],
        capture_output=True,
        text=True,
    )
    f.write(f"vkcube CPU usage: {ps_out.stdout.strip()}%\n")
    p.terminate()

# Clean up Xvfb if it was started
if xvfb_process is not None:
    xvfb_process.terminate()
