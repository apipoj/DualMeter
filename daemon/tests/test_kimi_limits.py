#!/usr/bin/env python3
"""Tests for the Kimi Code usage poller.

Run: python -m pytest daemon/tests/test_kimi_limits.py -x -q
"""
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from daemon.kimi_limits import (  # noqa: E402
    parse_usages,
    resolve_credentials,
    usages_endpoint,
)

NOW = datetime(2026, 10, 27, 12, 0, tzinfo=timezone.utc).timestamp()


def test_ratio_pools_map_to_session_and_weekly():
    body = {"usages": {
        "limit_5h": {"used_ratio": 0.625, "reset_time": "2026-10-27T14:00:00Z"},
        "limit_7d": {"used_ratio": 0.125, "reset_time": "2026-10-30T12:00:00Z"},
    }}
    assert parse_usages(body, NOW) == {
        "kok": True, "ks": 62, "ksr": 120, "kw": 12, "kwr": 3 * 24 * 60,
    }


def test_ratio_is_clamped_to_100_percent():
    body = {"usages": {"limit_5h": {"used_ratio": 1.4}}}
    assert parse_usages(body, NOW) == {"kok": True, "ks": 100, "ksr": -1}


def test_legacy_counters_fill_missing_ratio_windows():
    body = {
        "usage": {"limit": "200", "used": "50", "resetTime": "2026-10-28T12:00:00Z"},
        "limits": [{
            "window": {"duration": 300, "timeUnit": "TIME_UNIT_MINUTE"},
            "detail": {"limit": "100", "remaining": "75"},
        }],
    }
    assert parse_usages(body, NOW) == {
        "kok": True, "ks": 25, "ksr": -1, "kw": 25, "kwr": 24 * 60,
    }


def test_non_5h_rate_window_is_not_shown_as_session():
    body = {"limits": [{
        "window": {"duration": 1, "timeUnit": "TIME_UNIT_DAY"},
        "detail": {"limit": "100", "used": "10"},
    }]}
    assert parse_usages(body, NOW) == {"kok": False}


def test_elapsed_reset_reads_as_zero_minutes():
    body = {"usages": {"limit_5h": {"used_ratio": 0, "reset_time": "2026-10-27T11:00:00Z"}}}
    assert parse_usages(body, NOW)["ksr"] == 0


def test_monthly_pool_fills_in_when_plan_has_no_7d_window():
    # Shape returned by a live subscription without limit_7d (2026-10-27).
    body = {
        "limits": [{"window": {"duration": 300, "timeUnit": "TIME_UNIT_MINUTE"},
                    "detail": {"limit": "100", "remaining": "100"}}],
        "usages": {
            "limit_5h": {"used_ratio": 0, "reset_time": "2026-10-27T15:00:00Z"},
            "limit_month_total": {"used_ratio": 0.25, "reset_time": "2026-11-16T00:00:00Z"},
            "limit_month_code": {"used_ratio": 0, "reset_time": "2026-11-16T00:00:00Z"},
        },
    }
    payload = parse_usages(body, NOW)
    assert payload["ks"] == 0 and payload["ksr"] == 180
    assert payload["kw"] == 25 and payload["kwm"] is True
    assert payload["kwr"] == int((datetime(2026, 11, 16, tzinfo=timezone.utc).timestamp() - NOW) / 60)


def test_weekly_pool_is_not_flagged_monthly():
    body = {"usages": {"limit_7d": {"used_ratio": 0.5}}}
    assert "kwm" not in parse_usages(body, NOW)


def test_endpoint_accepts_bare_and_suffixed_base_urls():
    assert usages_endpoint("https://api.kimi.com") == "https://api.kimi.com/coding/v1/usages"
    assert usages_endpoint("https://api.kimi.com/coding/") == "https://api.kimi.com/coding/v1/usages"
    assert usages_endpoint("https://x.test/coding/v1") == "https://x.test/coding/v1/usages"


def test_config_api_key_wins(tmp_path, monkeypatch):
    monkeypatch.delenv("KIMI_CODE_API_KEY", raising=False)
    cfg = tmp_path / "config"
    cfg.write_text("chime = off\nkimi_api_key = sk-config  # pasted\n")
    token, headers = resolve_credentials(cfg, NOW)
    assert token == "sk-config"
    assert "X-Msh-Platform" not in headers


def test_cli_token_used_only_while_fresh(tmp_path, monkeypatch):
    monkeypatch.delenv("KIMI_CODE_API_KEY", raising=False)
    home = tmp_path / "kimi"
    (home / "credentials").mkdir(parents=True)
    cred = home / "credentials" / "kimi-code-env-0123abcd.json"
    monkeypatch.setenv("KIMI_CODE_HOME", str(home))
    cfg = tmp_path / "missing-config"

    cred.write_text(json.dumps({"access_token": "tok", "refresh_token": "r",
                                "expires_at": NOW + 3600}))
    token, headers = resolve_credentials(cfg, NOW)
    assert token == "tok"
    assert headers["X-Msh-Platform"] == "kimi_code_cli"

    cred.write_text(json.dumps({"access_token": "tok", "refresh_token": "r",
                                "expires_at": NOW + 30}))
    assert resolve_credentials(cfg, NOW) is None


def test_unconfigured_returns_none(tmp_path, monkeypatch):
    monkeypatch.delenv("KIMI_CODE_API_KEY", raising=False)
    monkeypatch.setenv("KIMI_CODE_HOME", str(tmp_path / "nope"))
    assert resolve_credentials(tmp_path / "config", NOW) is None
