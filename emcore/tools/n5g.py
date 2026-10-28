#!/usr/bin/env python3
"""Nano5g build and device tools."""
import argparse
import importlib

COMMANDS = {"image": "image"}

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=tuple(COMMANDS))
    args, rest = parser.parse_known_args(argv)
    return importlib.import_module("nano5g." + COMMANDS[args.command]).main(rest)

if __name__ == "__main__":
    raise SystemExit(main())
