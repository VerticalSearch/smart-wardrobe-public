import requests, json

r = requests.get("http://119.45.230.229:8000/api/wardrobe", timeout=15)
items = r.json() if isinstance(r.json(), list) else []
print(f"Total items: {len(items)}")
for item in items:
    status = item.get("image_status", "?")
    name = item.get("name", "?")
    desc = item.get("desc", "")[:60]
    iid = item.get("id", "?")
    print(f"  {iid} | status={status} | name={name} | desc={desc}")
    # Check for any style/category fields
    for key in ("style", "category", "type", "style_id", "styleId"):
        if key in item:
            print(f"    HAS {key}={item[key]}")
