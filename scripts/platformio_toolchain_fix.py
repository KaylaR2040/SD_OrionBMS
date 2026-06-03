import os

from SCons.Script import Import


Import("env")


def _configure_toolchain(toolchain_bin):
    env.PrependENVPath("PATH", toolchain_bin)

    tools = {
        "AR": "arm-none-eabi-ar",
        "AS": "arm-none-eabi-as",
        "CC": "arm-none-eabi-gcc",
        "CXX": "arm-none-eabi-g++",
        "GDB": "arm-none-eabi-gdb",
        "LINK": "arm-none-eabi-gcc",
        "NM": "arm-none-eabi-nm",
        "OBJCOPY": "arm-none-eabi-objcopy",
        "OBJDUMP": "arm-none-eabi-objdump",
        "RANLIB": "arm-none-eabi-gcc-ranlib",
        "READELF": "arm-none-eabi-readelf",
        "SIZETOOL": "arm-none-eabi-size",
        "STRIP": "arm-none-eabi-strip",
    }

    env.Replace(**{name: os.path.join(toolchain_bin, exe) for name, exe in tools.items()})


preferred_toolchain = os.path.expanduser("~/.platformio/packages/toolchain-gccarmnoneeabi/bin")
preferred_gcc = os.path.join(preferred_toolchain, "arm-none-eabi-gcc")

if os.path.isfile(preferred_gcc):
    _configure_toolchain(preferred_toolchain)
