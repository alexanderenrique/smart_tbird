Import("env")
from pathlib import Path


def copy_hex(source, target, env):
    built = Path(str(target[0]))
    dest = Path(env["PROJECT_DIR"]) / "firmware.hex"
    dest.write_bytes(built.read_bytes())
    print(f"Copied {built.name} → {dest}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", copy_hex)
