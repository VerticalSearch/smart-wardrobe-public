from wardrobe_gateway.weather_care import WeatherCareController, WeatherSnapshot, is_humid_weather, parse_open_meteo


def test_parse_open_meteo_and_humid_detection():
    snapshot = parse_open_meteo(
        {
            "current": {
                "time": "2026-06-06T08:00",
                "temperature_2m": 26.5,
                "relative_humidity_2m": 82,
                "precipitation": 0,
                "rain": 0,
                "weather_code": 3,
            },
            "hourly": {"precipitation_probability": [20]},
        },
        location="test",
        latitude=1,
        longitude=2,
        fetched_at=100,
    )

    assert snapshot.humidity_percent == 82
    assert snapshot.public_dict()["humid"] is True
    assert is_humid_weather(snapshot, humidity_threshold=75)


def test_weather_care_starts_stops_and_respects_cooldown():
    class Client:
        def __init__(self):
            self.snapshot = WeatherSnapshot(
                location="test",
                latitude=1,
                longitude=2,
                temperature_c=25,
                humidity_percent=86,
                precipitation_mm=0,
                rain_mm=0,
                precipitation_probability=10,
                weather_code=3,
                observed_at="now",
                fetched_at=0,
            )

        def fetch(self):
            return self.snapshot

    sent = []

    def send_command(payload, source):
        sent.append((payload, source))
        return {"ok": True}

    controller = WeatherCareController(
        Client(),
        send_command,
        duration_minutes=1,
        cooldown_seconds=3600,
        interval_seconds=30,
    )

    first = controller.tick(now_epoch=1000)
    controller.tick(now_epoch=1010)
    controller.tick(now_epoch=1061)
    cooldown = controller.tick(now_epoch=1200)

    assert first["started"] is True
    assert cooldown["reason"] == "cooldown"
    assert [payload["action_id"] for payload, _ in sent] == [8, 9]
    assert all(source == "weather-care" for _, source in sent)
