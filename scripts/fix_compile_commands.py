import json
import os
import re

Import("env")

REMOVE_FLAGS = (
    "-mlongcalls",
    "-fstrict-volatile-bitfields",
    "-fno-tree-switch-conversion",
    "-freorder-blocks",
    "-Wwrite-strings",
    "-fstack-protector",
    "-ggdb",
    "-MMD",
    "-MP",
    "-fno-jump-tables",
)

SYSTEM_INCLUDE_MARKER = "xtensa-esp32s3-elf/sys-include"
CLANG_COMPAT_FLAGS = (
    "-D__GNUC__=12",
    "-D__GNUC_MINOR__=0",
    "-D__GNUC_PATCHLEVEL__=0",
)


def sanitize_command(command):
    for flag in REMOVE_FLAGS:
        command = re.sub(rf"(?<!\S){re.escape(flag)}(?!\S)\s*", "", command)
    return re.sub(r"\s+", " ", command).strip()


def get_toolchain_system_includes():
    toolchain_dir = env.PioPlatform().get_package_dir("toolchain-xtensa-esp32s3")
    if not toolchain_dir:
        return ""

    gcc_lib_dir = os.path.join(toolchain_dir, "lib", "gcc", "xtensa-esp32s3-elf")
    gcc_version = None
    if os.path.isdir(gcc_lib_dir):
        versions = sorted(
            name
            for name in os.listdir(gcc_lib_dir)
            if os.path.isdir(os.path.join(gcc_lib_dir, name))
        )
        if versions:
            gcc_version = versions[-1]

    if gcc_version is None:
        return ""

    base = os.path.join(toolchain_dir, "xtensa-esp32s3-elf")
    paths = [
        os.path.join(base, "include"),
        os.path.join(base, "sys-include"),
        os.path.join(gcc_lib_dir, gcc_version, "include"),
        os.path.join(gcc_lib_dir, gcc_version, "include-fixed"),
        os.path.join(base, "include", "c++", gcc_version),
        os.path.join(
            base,
            "include",
            "c++",
            gcc_version,
            "xtensa-esp32s3-elf",
            "no-rtti",
        ),
        os.path.join(base, "include", "c++", gcc_version, "backward"),
    ]

    flags = []
    for path in paths:
        if os.path.isdir(path):
            flags.extend(["-isystem", path.replace("\\", "/")])

    return " ".join(flags)


def add_system_includes(command):
    system_includes = get_toolchain_system_includes()
    if not system_includes or SYSTEM_INCLUDE_MARKER in command:
        return command

    if " -c " in command:
        return command.replace(" -c ", f" -c {system_includes} ", 1)

    source_match = re.search(r"\s(\S+\.(?:cpp|c|cc|cxx|S))\s*$", command)
    if source_match:
        insert_at = source_match.start(1)
        return f"{command[:insert_at]}{system_includes} {command[insert_at:]}"

    return f"{system_includes} {command}"


def add_clang_compat_flags(command):
    if any(flag in command for flag in CLANG_COMPAT_FLAGS):
        return command

    compat = " ".join(CLANG_COMPAT_FLAGS)
    if " -c " in command:
        return command.replace(" -c ", f" -c {compat} ", 1)

    return f"{compat} {command}"


def fix_cpp_properties():
    props_path = os.path.join(env["PROJECT_DIR"], ".vscode", "c_cpp_properties.json")
    if not os.path.isfile(props_path):
        return

    with open(props_path, encoding="utf-8") as handle:
        contents = handle.read()

    contents = re.sub(
        r'"compilerArgs":\s*\[\s*"-mlongcalls",\s*""\s*\]',
        '"compileCommands": "${workspaceFolder}/compile_commands.json",\n            "compilerArgs": []',
        contents,
    )

    with open(props_path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(contents)


def fix_compile_commands(source, target, env):
    db_path = os.path.join(env["PROJECT_DIR"], "compile_commands.json")
    if not os.path.isfile(db_path):
        return

    with open(db_path, encoding="utf-8") as handle:
        entries = json.load(handle)

    for entry in entries:
        if "command" in entry:
            entry["command"] = add_clang_compat_flags(
                add_system_includes(sanitize_command(entry["command"]))
            )

    with open(db_path, "w", encoding="utf-8", newline="\n") as handle:
        json.dump(entries, handle, indent=4)
        handle.write("\n")

    fix_cpp_properties()


env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", fix_compile_commands)

if "compiledb" in COMMAND_LINE_TARGETS:
    fix_compile_commands([], [], env)
