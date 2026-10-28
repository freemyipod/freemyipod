#!/usr/bin/env python3
"""Nano5g build and device tools."""
import argparse
import importlib
import sys

COMMANDS = {"image": "image", "diag": "diagnostics"}

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=tuple(COMMANDS))
    argv = sys.argv[1:] if argv is None else argv
    args = parser.parse_args(argv[:1])
    rest = argv[1:]
    return importlib.import_module("nano5g." + COMMANDS[args.command]).main(rest)

if __name__ == "__main__":
    raise SystemExit(main())
