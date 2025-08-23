# Rendering benchmark: cpu usage + frame-rates for native desktop present vs no-op present on Xvfb desktop
# - Benchmark program
#   - Logs session type, cpu usage, frame rate, device name to a file
#   - Session type from CLI, cpu usage from script, frame rate + device name from layer
# - Vulkan layer
#   - Passthrough
#   - No-present
# - Compare how no-present Xvfb compares to present Xvfb and Xorg.

# Flags:
# - Display

# Writes benchmark.txt with:
# - Device name
# - Average cpu usage over a few seconds
# - Swapchain image size

import subprocess
import time
import argparse
import os


parser = argparse.ArgumentParser()
parser.add_argument("server_type", help="A string saying what type of xserver you're running on. Must be Xorg or Xvfb.")
parser.add_argument("display", help='Which display to run the test on e.g. ":0"')
args = parser.parse_args()

if args.server_type.lower() not in ["xorg", "xvfb"]:
    print(f'Unrecognized server type: {args.server_type}')
    exit()

if not args.display[0] == ":":
    print('Invalid display string. Expected for a string like ":0"')
    exit()

with open(f"out/out_server_{args.server_type}.txt", "w") as f:
    f.write(f"Running test for server: {args.server_type}\n\n")
    f.flush()
    env = os.environ
    env["DISPLAY"] = args.display
    p = subprocess.Popen("vkcube", stdout=f, stderr=f, env=env)
    time.sleep(1)

    vkcube_ret = p.poll()
    if vkcube_ret:
        f.write(f"vkcube exited with code: {vkcube_ret}\n")
        f.flush()
        exit()

    ps_out = subprocess.run(["ps", "-p", str(p.pid), "-o", "%cpu"], capture_output=True, text=True)
    f.write("vkcube cpu usage:\n")
    f.write(ps_out.stdout)
    p.terminate()

