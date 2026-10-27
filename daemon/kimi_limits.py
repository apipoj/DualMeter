"""Fetch Kimi Code 5h/7d usage from the Kimi Code usages endpoint.

Same free-ride posture as the Claude path: the daemon never refreshes tokens.
Credentials, first match wins:

1. `kimi_api_key` in the daemon config, or the KIMI_CODE_API_KEY env var
   (an API key from the Kimi Code console).
2. The Kimi Code CLI's OAuth token in ~/.kimi-code/credentials/kimi-code*.json
   (or $KIMI_CODE_HOME), used only while unexpired — the CLI owns refreshing.
   Its access tokens last ~15 minutes, so this path only stays live while the
   CLI is in use; an API key is the always-on option.

The endpoint and response shape mirror CodexBar's Kimi provider.
"""

from __future__ import annotations

import json
import os
import platform
import time
import uuid
from datetime import datetime
from pathlib import Path
from typing import Any

import httpx

DEFAULT_BASE_URL = "https://api.kimi.com"
TIMEOUT_SEC = 15.0
SESSION_MINUTES = 300
WEEKLY_MINUTES = 7 * 24 * 60


def _read_config(config_file: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    try:
        if config_file.exists():
            for line in config_file.read_text().splitlines():
                line = line.split("#", 1)[0].strip()
                if "=" not in line:
                    continue
                key, val = line.split("=", 1)
                out[key.strip().lower()] = val.strip()
    except OSError:
        pass
    return out


def _kimi_home() -> Path:
    override = os.environ.get("KIMI_CODE_HOME", "").strip()
    return Path(override).expanduser() if override else Path.home() / ".kimi-code"


def _cli_access_token(home: Path, now: float) -> str | None:
    # Older CLIs write kimi-code.json; newer ones kimi-code-env-<hash>.json.
    # The most recently written file is the live login.
    files = sorted((home / "credentials").glob("kimi-code*.json"),
                   key=lambda f: f.stat().st_mtime, reverse=True)
    if not files:
        return None
    try:
        cred = json.loads(files[0].read_text())
    except (OSError, json.JSONDecodeError):
        return None
    if not isinstance(cred, dict):
        return None
    token = str(cred.get("access_token") or "").strip()
    expires_at = cred.get("expires_at")
    if not token or not isinstance(expires_at, (int, float)):
        return None
    # Leave a minute of slack so we never send a token that dies in flight.
    return token if expires_at > now + 60 else None


def _device_id(home: Path) -> str:
    try:
        existing = (home / "device_id").read_text().strip()
        if existing:
            return existing
    except OSError:
        pass
    # Don't write into the CLI's home; a per-process id is fine for us.
    return str(uuid.uuid4())


def _cli_headers(home: Path) -> dict[str, str]:
    os_version = platform.mac_ver()[0] or platform.release()
    return {
        "User-Agent": "DualMeter/1.0",
        "X-Msh-Platform": "kimi_code_cli",
        "X-Msh-Version": "1.0.0",
        "X-Msh-Device-Name": platform.node() or "unknown",
        "X-Msh-Device-Model": f"{platform.system()} {os_version} {platform.machine()}",
        "X-Msh-Os-Version": os_version,
        "X-Msh-Device-Id": _device_id(home),
    }


def resolve_credentials(config_file: Path, now: float | None = None) -> tuple[str, dict[str, str]] | None:
    """Return (bearer token, extra headers), or None when Kimi isn't set up."""
    now = time.time() if now is None else now
    cfg = _read_config(config_file)
    api_key = cfg.get("kimi_api_key") or os.environ.get("KIMI_CODE_API_KEY", "").strip()
    if api_key:
        return api_key, {"User-Agent": "DualMeter/1.0"}
    home = _kimi_home()
    token = _cli_access_token(home, now)
    if token:
        return token, _cli_headers(home)
    return None


def usages_endpoint(base_url: str) -> str:
    base = base_url.rstrip("/")
    if base.endswith("/coding/v1"):
        return base + "/usages"
    if base.endswith("/coding"):
        return base + "/v1/usages"
    return base + "/coding/v1/usages"


def _parse_iso(value: Any) -> float | None:
    if not isinstance(value, str) or not value:
        return None
    try:
        return datetime.fromisoformat(value.replace("Z", "+00:00")).timestamp()
    except ValueError:
        return None


def _reset_mins(resets_at: float | None, now: float) -> int:
    if resets_at is None:
        return -1
    mins = (resets_at - now) / 60.0
    return int(round(mins)) if mins > 0 else 0


def _ratio_window(pool: Any) -> tuple[float, float | None] | None:
    if not isinstance(pool, dict):
        return None
    ratio = pool.get("used_ratio")
    if isinstance(ratio, bool) or not isinstance(ratio, (int, float)) or ratio < 0:
        return None
    return min(1.0, float(ratio)) * 100.0, _parse_iso(pool.get("reset_time"))


def _count_window(detail: Any) -> tuple[float, float | None] | None:
    """Legacy {limit, used|remaining} counters → (used %, reset epoch)."""
    if not isinstance(detail, dict):
        return None
    try:
        limit = int(detail.get("limit"))
    except (TypeError, ValueError):
        return None
    if limit <= 0:
        return None
    used = None
    try:
        used = int(detail.get("used"))
    except (TypeError, ValueError):
        try:
            remaining = int(detail.get("remaining"))
            if 0 <= remaining <= limit:
                used = limit - remaining
        except (TypeError, ValueError):
            pass
    if used is None or used < 0:
        return None
    reset = None
    for key in ("resetTime", "resetAt", "reset_time", "reset_at"):
        reset = _parse_iso(detail.get(key))
        if reset is not None:
            break
    return max(0.0, min(100.0, used / limit * 100.0)), reset


def _window_minutes(window: Any) -> int | None:
    if not isinstance(window, dict):
        return None
    duration = window.get("duration")
    unit = {"TIME_UNIT_MINUTE": 1, "TIME_UNIT_HOUR": 60, "TIME_UNIT_DAY": 1440}.get(
        window.get("timeUnit"))
    if not isinstance(duration, int) or duration <= 0 or unit is None:
        return None
    return duration * unit


def parse_usages(body: dict[str, Any], now: float) -> dict[str, Any]:
    """Map a /coding/v1/usages response to DualMeter `k*` payload fields."""
    pools = body.get("usages") if isinstance(body.get("usages"), dict) else {}
    session = _ratio_window(pools.get("limit_5h"))
    weekly = _ratio_window(pools.get("limit_7d"))
    monthly = False

    # Older responses only carry counters: `usage` is the weekly quota and the
    # first `limits` entry is the short rate window (5h today).
    if weekly is None:
        weekly = _count_window(body.get("usage"))
    # Plans without a 7-day pool still meter a monthly one; show that in the
    # second column (the firmware relabels it "MO" when kwm is set).
    if weekly is None:
        weekly = _ratio_window(pools.get("limit_month_total"))
        monthly = weekly is not None
    if session is None:
        limits = body.get("limits")
        if isinstance(limits, list) and limits and isinstance(limits[0], dict):
            minutes = _window_minutes(limits[0].get("window"))
            if minutes in (None, SESSION_MINUTES):
                session = _count_window(limits[0].get("detail"))

    if session is None and weekly is None:
        return {"kok": False}
    payload: dict[str, Any] = {"kok": True}
    if session is not None:
        payload["ks"] = int(round(session[0]))
        payload["ksr"] = _reset_mins(session[1], now)
    if weekly is not None:
        payload["kw"] = int(round(weekly[0]))
        payload["kwr"] = _reset_mins(weekly[1], now)
        if monthly:
            payload["kwm"] = True
    return payload


async def poll_kimi_payload(config_file: Path) -> dict[str, Any] | None:
    """Return DualMeter Kimi fields, or None if Kimi isn't configured."""
    creds = resolve_credentials(config_file)
    if creds is None:
        return None
    token, headers = creds
    base_url = (_read_config(config_file).get("kimi_base_url")
                or os.environ.get("KIMI_CODE_BASE_URL", "").strip()
                or DEFAULT_BASE_URL)
    if not base_url.startswith("https://"):
        return {"kok": False}
    headers = {**headers, "Authorization": f"Bearer {token}", "Accept": "application/json"}
    try:
        async with httpx.AsyncClient(timeout=TIMEOUT_SEC) as client:
            resp = await client.get(usages_endpoint(base_url), headers=headers)
    except httpx.HTTPError:
        return {"kok": False}
    if resp.status_code != 200:
        return {"kok": False}
    try:
        body = resp.json()
    except ValueError:
        return {"kok": False}
    if not isinstance(body, dict):
        return {"kok": False}
    return parse_usages(body, time.time())
