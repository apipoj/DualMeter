#pragma once
#include <Arduino.h>

struct UsageData {
    float session_pct;       // Claude 5h utilization 0-100
    int session_reset_mins;  // Claude minutes until 5h reset
    float weekly_pct;        // Claude 7-day utilization
    int weekly_reset_mins;   // Claude minutes until weekly reset
    char status[16];         // "allowed", "limited", etc.
    bool chime;              // play the session-reset chime; false unless daemon opts in
    bool enterprise;         // true = Enterprise spending-limit account
    int time_pct;            // 0-100: fraction of billing period elapsed (Enterprise)
    int period_days;         // total billing period length in days (Enterprise)
    char reset_date[12];     // formatted reset date e.g. "Jul 1" (Enterprise)
    long clock_epoch;        // local wall-clock epoch (s) from daemon; 0 = not provided
    int  clock_fmt;          // 12 or 24 (hour format from daemon); defaults to 24
    bool ok;                 // Claude parse succeeded
    bool valid;              // false until first successful parse

    // Codex (OpenAI) 5h / 7d windows. used percent, matching Claude fields.
    float codex_session_pct;
    int   codex_session_reset_mins;
    float codex_weekly_pct;
    int   codex_weekly_reset_mins;
    bool  codex_ok;
    bool  codex_valid;
};
