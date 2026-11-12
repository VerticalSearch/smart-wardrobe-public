import requests, json, base64, time, sys

img_path = "/home/pi/raspberry_pi_gateway/captures/capture_1780814378494369625.jpg"
if len(sys.argv) > 1:
    img_path = sys.argv[1]

img = open(img_path, "rb").read()
b64 = "data:image/jpeg;base64," + base64.b64encode(img).decode()
r = requests.post("http://119.45.230.229:8000/api/wardrobe/add", json={"image_base64": b64}, timeout=30)
print("Upload status:", r.status_code)
body = r.json()
print(json.dumps(body, ensure_ascii=False, indent=2))

# Check for style/category in response
item = body.get("item", {})
item_id = item.get("id", "")
if item_id:
    print("\n--- Polling for recognition result ---")
    for attempt in range(3):
        time.sleep(1.5)
        r2 = requests.get(f"http://119.45.230.229:8000/api/wardrobe/{item_id}", timeout=10)
        if r2.status_code == 200:
            detail = r2.json()
            print(f"Poll {attempt+1}: image_status={detail.get('image_status')}, name={detail.get('name')}, style={detail.get('style')}, category={detail.get('category')}")
            if detail.get("image_status") == "done":
                break
        else:
            print(f"Poll {attempt+1}: HTTP {r2.status_code}, endpoint may not exist")
            break
    
    # Also try the full wardrobe list
    print("\n--- Wardrobe list (last item) ---")
    r3 = requests.get("http://119.45.230.229:8000/api/wardrobe", timeout=10)
    if r3.status_code == 200:
        items = r3.json()
        if isinstance(items, list) and items:
            last = items[-1]
            print(f"Last item: id={last.get('id')}, name={last.get('name')}, image_status={last.get('image_status')}")
            for k, v in last.items():
                if k not in ("img", "original_img", "clean_img"):
                    print(f"  {k}: {repr(v)[:200]}")
