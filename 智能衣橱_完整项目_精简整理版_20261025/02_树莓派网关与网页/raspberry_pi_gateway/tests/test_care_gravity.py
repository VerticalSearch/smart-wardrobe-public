from wardrobe_gateway.care_gravity import parse_care_gravity_line, parse_care_gravity_reading


def test_single_gravity_sample_maps_to_left_channel():
    assert parse_care_gravity_line("@GRAV|1.23") == (1.23, None)


def test_dual_gravity_sample_maps_to_both_channels():
    assert parse_care_gravity_line("@GRAV|1.23|0.98") == (1.23, 0.98)


def test_key_value_gravity_sample_accepts_channel_names():
    assert parse_care_gravity_line("@WG|left=2.5|right=2.1") == (2.5, 2.1)


def test_unrelated_debug_line_is_ignored():
    assert parse_care_gravity_line("@DBG1|task|ok") is None


def test_care_line_keeps_tared_hx711_counts():
    assert parse_care_gravity_line("@CARE|left=-712|right=70") == (-712.0, 70.0)
    reading = parse_care_gravity_reading("@CARE|left=-712|right=70")
    assert reading is not None
    assert reading.unit == "counts"
