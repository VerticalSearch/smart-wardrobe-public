"""Test: upload a photo, poll for async recognition result."""
import requests, base64, time, json, sys

img_path = sys.argv[1] if len(sys.argv) > 1 else "/home/pi/raspberry_pi_gateway/captures/capture_1780814378494369625.jpg"

img = open(img_path, "rb").read()
b64 = "data:image/jpeg;base64," + base64.b64encode(img).decode()
print("[1] Uploading...")
r = requests.post("http://119.45.230.229:8000/api/wardrobe/add", json={"image_base64": b64}, timeout=30)
print(f"    HTTP {r.status_code}")
body = r.json()
print(f"    POST response: {json.dumps(body, ensure_ascii=False, indent=2)}")

item_id = body.get("item", {}).get("id", "")
if not item_id:
    print("ERROR: No item_id in response")
    sys.exit(1)

print(f"\n[2] Polling item {item_id} for recognition completion...")
for attempt in range(10):
    time.sleep(3)
    r2 = requests.get(f"http://119.45.230.229:8000/api/wardrobe", timeout=10)
    if r2.status_code == 200:
        items = r2.json()
        if isinstance(items, list):
            for item in items:
                if item.get("id") == item_id:
                    status = item.get("image_status", "")
                    name = item.get("name", "")
                    desc = item.get("desc", "")
                    print(f"    [{attempt+1}] image_status={status}, name={name}")
                    if status == "done":
                        print(f"\n[3] Recognition complete!")
                        print(f"    Full item: {json.dumps(item, ensure_ascii=False, indent=2)[:1500]}")
                        
                        # Check if style_id_from_response would work
                        from camera_test import style_id_from_response
                        sid = style_id_from_response(item)
                        print(f"\n    style_id_from_response(item) = {sid}")
                        sys.exit(0)
            print(f"    [{attempt+1}] item not in list yet")
    else:
        print(f"    [{attempt+1}] GET /api/wardrobe failed: {r2.status_code}")

print("\n[FAIL] Recognition did not complete within 30s")
