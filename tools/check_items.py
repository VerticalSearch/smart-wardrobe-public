import requests, json
r = requests.get("http://119.45.230.229:8000/api/wardrobe?status=all", timeout=10)
items = r.json() if isinstance(r.json(), list) else []
print("Count:", len(items))
for i in items:
    for k, v in i.items():
        if k not in ("img", "original_img", "clean_img"):
            s = repr(v)
            if len(s) > 150:
                s = s[:150] + "..."
            print(f"  {k}: {s}")
