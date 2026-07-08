Import("env")

probe_script = "$PROJECT_DIR/scripts/updi_probe.sh"
alternatives_script = "$PROJECT_DIR/scripts/updi_try_alternatives.sh"
prog = "~/.platformio/packages/framework-arduino-megaavr-megatinycore/tools/prog.py"
python = "~/.platformio/penv/bin/python"
port = "$UPLOAD_PORT"
mcu = "$BOARD_MCU"

env.AddCustomTarget(
    name="updi-probe",
    dependencies=None,
    actions=env.VerboseAction(f"bash {probe_script}", "Running UPDI probe..."),
    title="UPDI Probe",
    description="Verbose SerialUPDI link test (multiple baud rates, fuse read)",
)

env.AddCustomTarget(
    name="updi-try-alternatives",
    dependencies="${BUILD_DIR}/${PROGNAME}.hex",
    actions=env.VerboseAction(
        f"bash {alternatives_script} $BUILD_DIR/${{PROGNAME}}.hex",
        "Trying alternate UPDI upload tools...",
    ),
    title="UPDI Try Alternatives",
    description="prog.py, pymcuprog, and avrdude serialupdi with power-cycle prompts",
)

env.AddCustomTarget(
    name="updi-upload-verbose",
    dependencies="${BUILD_DIR}/${PROGNAME}.hex",
    actions=env.VerboseAction(
        f"{python} {prog} "
        f"-t uart -u {port} -b $UPLOAD_SPEED -d {mcu} "
        "--fuses 2:0x02 -v -v -f $SOURCE -a write",
        "Verbose UPDI upload...",
    ),
    title="UPDI Upload (verbose)",
    description="Flash firmware with prog.py -v -v debug logging",
)

env.AddCustomTarget(
    name="updi-erase-locked",
    dependencies=None,
    actions=env.VerboseAction(
        f"{python} -m pip install -q pymcuprog && "
        f"{python} -m pymcuprog erase --chip-erase-locked-device "
        f"-t uart -u {port} -d {mcu} -v debug",
        "UPDI chip erase (locked device)...",
    ),
    title="UPDI Erase Locked",
    description="pymcuprog chip erase for locked UPDI devices",
)
