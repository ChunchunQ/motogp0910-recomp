#!/usr/bin/env python3
"""Exercise real Windows LAN peers (native Windows or Wine); no GPU required."""
import argparse
from pathlib import Path
import subprocess
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("probe", type=Path)
    parser.add_argument("--wine", type=Path)
    parser.add_argument("--game-root", type=Path, help="For lan_import_probe: real title root")
    args = parser.parse_args()
    probe = args.probe.resolve(strict=True)
    runner = [str(args.wine.resolve(strict=True))] if args.wine else []
    command = runner + [str(probe)]

    def guest_path(path):
        return "Z:" + str(path).replace("/", "\\") if args.wine else str(path)

    if args.game_root:
        command.append(guest_path(args.game_root.resolve(strict=True)))

    subprocess.run(command, check=True, timeout=30)
    for mode in (("automatic", "direct", "delayed") if args.game_root else ("automatic", "direct")):
        with tempfile.TemporaryDirectory(prefix="motogp-lan-") as directory:
            root = Path(directory).resolve()
            ready, stop = root / "ready.txt", root / "stop.txt"
            with (root / "host.log").open("w+") as log:
                host = subprocess.Popen(command + ["host", guest_path(ready), guest_path(stop)],
                                        stdout=log, stderr=subprocess.STDOUT)
                try:
                    deadline = time.monotonic() + 15
                    while not ready.exists():
                        if host.poll() is not None or time.monotonic() >= deadline:
                            raise RuntimeError("Host did not publish a live socket")
                        time.sleep(0.025)
                    subprocess.run(command + ["client", mode], check=True, timeout=10)
                    stop.touch()
                    if host.wait(timeout=10):
                        raise RuntimeError("Host did not observe a join and packet exchange")
                finally:
                    stop.touch()
                    if host.poll() is None:
                        try:
                            host.wait(timeout=5)
                        except subprocess.TimeoutExpired:
                            host.kill()
                            host.wait()
                    log.seek(0)
                    print(log.read(), end="")
            scope = "generated title session wrappers" if args.game_root else "bidirectional opaque data"
            print(f"Two Windows processes: {mode} discovery / join / {scope} passed")
    print("This validates the service/guest imports, not the game's lobby UI or a Radmin race.")


if __name__ == "__main__":
    main()
