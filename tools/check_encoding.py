import requests, json
r = requests.get("http://119.45.230.229:8000/api/wardrobe", timeout=10)
items = r.json() if isinstance(r.json(), list) else []
for i, item in enumerate(items):
    name = item.get("name", "")
    desc = item.get("desc", "")
    iid = item.get("id", "")
    img_status = item.get("image_status", "")
    print(f"[{i}] {iid} status={img_status}")
    print(f"    name bytes: {name.encode('utf-8')[:60]}")
    print(f"    name repr:  {repr(name)[:100]}")
    print(f"    desc repr:  {repr(desc)[:100]}")
