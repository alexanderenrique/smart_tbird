Import("env")

from pathlib import Path


def copy_hex(source, target, env):
    built_hex = Path(str(target[0]))
    output_hex = Path(env["PROJECT_DIR"]) / "firmware.hex"
    output_hex.write_bytes(built_hex.read_bytes())
    print(f"Copied {built_hex.name} -> {output_hex}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", copy_hex)
