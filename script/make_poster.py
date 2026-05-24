#!/usr/bin/env python3
import os, sys, subprocess, math
from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
OUTPUT = os.path.join(SCRIPT_DIR, "luanti_games_poster.png")
THUMBS_DIR = os.path.join(SCRIPT_DIR, "thumbs")

URLS = [
    "8ffd1409ff","864a6133e4","55ab4fc1b5","32ff17b9d2","7f0ec19e49",
    "94295f6d70","f8309307c6","8078c595ae","f79c1195b4","fdbb5f5922",
    "f334649d84","a22e7ccb4b","7550c290b5","0195f6298c","156cbe1636",
    "4aa8824793","0791c8033d","47ffa95610","40f8802c3d","c93bcca656",
    "5f903e3995","79ce26c109","7fc1e7bf61","1f33d10745","mMhvdPN9dM",
    "18d0bb722d","CZYwsfu1Le","67c591b9b8","7f5771dfff","07343a92e7",
    "d70eb6f359","09924ef1ef","baa23f09e9","d3872b989b","35a046589c",
    "4e29904768","2f8309e348","9f2f5e75ae","3ad44301ec","b26a8ea48b",
    "LXAgbOkLFz","2f16b29e47","d526fc7fd7","632a9643c5","123f9526f6",
    "e809f546da","JRJSgu2BV4","47e6111fee","91324ef4f9","67bbae5661",
    "29ef5ac4ba","fb2aa8e11c","09fda35d46","7838836851","19f2b13916",
    "2e576f28a8","68ovEdgCu3","896660af5d","69famAhfZr","a967584017",
    "56264b87b1","4a7b0188bd","af3f03b10e","NKSVMnvh05","c3abeb188f",
    "d936c3fce1","26d9d831da","42514241e1","6fef0fbdc5","093cf275b9",
    "39c5592543","45eee1a96a","70e7aac248","ae11bae5ce","fa2a1ba450",
    "c98bed8dfb","99c3a4cf08","Awo7bssEjT","db1a942e1a","5f58f02d32",
    "619f24249d","64f7f444f6","aaf0300234","1aa559039d","881d766d45",
    "q6iL3giyrX","84650036a6","4a55e8cc0f","73d666db5c","cgM5uSzzAY",
    "6f56447689","ed825dcdea","3acdee0855","ea71397554","71173753d0",
    "r9yczgJPla","e0d72942c8","9b136e5c60","c9e5327d47","5472db30c0",
]
BASE = "https://content.luanti.org/thumbnails/2/"
TOTAL = 100

def download():
    os.makedirs(THUMBS_DIR, exist_ok=True)
    for i, h in enumerate(URLS[:TOTAL]):
        fname = os.path.join(THUMBS_DIR, f"{i:03d}_{h}.webp")
        if os.path.exists(fname) and os.path.getsize(fname) > 100:
            print(f"[{i+1}/{TOTAL}] Skip {h}")
            continue
        url = BASE + h + ".webp"
        r = subprocess.run(["curl", "-skL", "-o", fname, url], capture_output=True)
        if r.returncode == 0:
            print(f"[{i+1}/{TOTAL}] OK {h}")
        else:
            print(f"[{i+1}/{TOTAL}] FAIL {h}")

def make_poster():
    imgs = []
    for fname in sorted(os.listdir(THUMBS_DIR)):
        if fname.endswith(".webp"):
            try:
                img = Image.open(os.path.join(THUMBS_DIR, fname))
                imgs.append(img)
            except: pass
    if not imgs:
        print("No images!"); return

    n = min(len(imgs), 100)
    # 10 cols x 10 rows = 100 images, renders as 10:10 = 1:1 squares
    # For 16:9 poster, use 12 cols x 8 rows = 96
    cols = 12
    rows = 8
    n = min(n, cols * rows)
    imgs = imgs[:n]
    
    print(f"Grid: {cols}x{rows}, {len(imgs)} images")

    thumb = 160
    pw, ph = cols * thumb, rows * thumb
    poster = Image.new("RGB", (pw, ph), (20, 20, 20))

    for i, img in enumerate(imgs):
        # Crop to square
        w, h = img.size
        s = min(w, h)
        img = img.crop(((w-s)//2, (h-s)//2, (w-s)//2+s, (h-s)//2+s))
        img = img.resize((thumb, thumb), Image.LANCZOS)
        poster.paste(img, ((i % cols) * thumb, (i // cols) * thumb))

    poster.save(OUTPUT, quality=95)
    print(f"Saved {OUTPUT} ({pw}x{ph})")

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "dl":
        download()
    elif len(sys.argv) > 1 and sys.argv[1] == "poster":
        make_poster()
    else:
        download()
        make_poster()
