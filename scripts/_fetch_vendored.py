import os
import subprocess
import tarfile
import tempfile

BASE = r"D:\code\SDL\MonsterWar\external"
JOBS = [
    # (url, dest_dir_under_external, target_top_dir)
    ("https://github.com/libsdl-org/libpng/archive/refs/heads/v1.6.47-SDL.tar.gz",
     os.path.join(BASE, "SDL_image-release-3.2.4", "external", "libpng")),
    ("https://github.com/libsdl-org/zlib/archive/refs/heads/v1.3.1-SDL.tar.gz",
     os.path.join(BASE, "SDL_image-release-3.2.4", "external", "zlib")),
    ("https://github.com/libsdl-org/freetype/archive/refs/heads/VER-2-13-2-SDL.tar.gz",
     os.path.join(BASE, "SDL_ttf-release-3.2.2", "external", "freetype")),
]

def download(url, out):
    if os.path.exists(out):
        os.remove(out)
    # -C - resume, --retry-all-errors, long timeout for flaky network
    cmd = [
        "curl", "-L", "--retry", "20", "--retry-all-errors",
        "--retry-delay", "5", "-C", "-", "--connect-timeout", "30",
        "-o", out, url,
    ]
    r = subprocess.run(cmd)
    return r.returncode == 0

def extract(tarball, dest):
    os.makedirs(dest, exist_ok=True)

    def member_filter(member, path):
        if member.issym() or member.islnk():
            return None
        return member

    with tarfile.open(tarball, "r:gz") as tf:
        # 去掉顶层目录（archive 顶层是 repo-ref/）
        members = tf.getmembers()
        prefix = os.path.commonprefix([m.name for m in members]).split("/")[0]
        for m in members:
            if m.name == prefix:
                continue
            m.name = m.name[len(prefix) + 1:]
            if m.name == "":
                continue
            tf.extract(m, dest, filter=member_filter)

with tempfile.TemporaryDirectory() as td:
    for i, (url, dest) in enumerate(JOBS, 1):
        tarball = os.path.join(td, "v.tgz")
        print(f"[{i}/{len(JOBS)}] downloading {url}")
        if not download(url, tarball):
            print(f"[{i}/{len(JOBS)}] FAILED: {url}")
            raise SystemExit(1)
        print(f"[{i}/{len(JOBS)}] extracting -> {dest}")
        extract(tarball, dest)
        size = sum(os.path.getsize(os.path.join(dp, f))
                   for dp, _, fs in os.walk(dest) for f in fs)
        print(f"[{i}/{len(JOBS)}] done: {dest} ({size} bytes)")

print("ALL VENDORED SOURCES READY")
