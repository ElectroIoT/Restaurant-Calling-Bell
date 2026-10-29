#!/usr/bin/env python3
"""Battery-life and voltage-sag estimate for the table remote.

EVERYTHING HERE IS AN ESTIMATE FROM DATASHEET FIGURES AND STATED ASSUMPTIONS, NOT A MEASUREMENT.
Replace the assumptions with your own measurements (docs/test-plan.md, tests 12, 13 and 18) and rerun.

  python tools/battery_calc.py
"""

# ---- datasheet figures (docs/datasheets/Si24R1-datasheet-V1-eng.pdf, section 7.2, VDD=3 V) ----
I_TX_MA = {"-12 dBm": 8.5, "-6 dBm": 9.5, "0 dBm": 12.0, "4 dBm": 16.0, "7 dBm": 25.0}
I_RX_MA = 14.0            # 250 kbps
I_IDLE_MA = 0.38          # idle-TX (crystal running)
I_SHUTDOWN_UA = 1.0

# ---- assumptions (replace with measurements) ----
I_MCU_ACTIVE_MA = 1.5     # ATmega328P @ 4 MHz, 3 V, running (datasheet typical is lower; keep margin)
I_LED_MA = 1.0            # red LED + 1k at 3 V
I_SLEEP_UA = {"optimistic": 2.0, "assumed": 3.0, "pessimistic": 10.0}   # MCU power-down + radio shutdown + leakage
T_WAKE_MS = 2.0           # radio crystal start-up (datasheet 1.5-2 ms)
T_TX_MS = 0.55            # one 8-byte frame at 250 kbps incl. preamble/address/CRC
T_RX_REPLY_MS = 5.0       # listening for the application reply per attempt
ATTEMPTS = 1.5            # average transmissions per call (retries, collisions)
T_DEBOUNCE_MS = 30.0
T_LED_SUCCESS_MS = 1000.0
T_LOCKOUT_MS = 1500.0     # MCU is in power-down (watchdog sleep) during the LED and lockout periods
POLL_WINDOW_S = 300
POLL_PERIOD_S = 8
POLL_ENABLE = True
I_WDT_UA = 4.0            # watchdog + sleep during the polling window

BATTERIES = {
    # name: (rated mAh, usable fraction for this load, internal resistance fresh ohm, end-of-life ohm, OCV fresh V, OCV near end V)
    "CR2450": (620, 0.65, 20.0, 100.0, 3.0, 2.2),
    "CR2032": (225, 0.65, 15.0, 80.0, 3.0, 2.2),
    "2xAAA alkaline": (1000, 0.85, 0.6, 1.5, 3.1, 2.0),
}


def per_call_mc(tx="0 dBm"):
    """Charge per call in millicoulombs (mA*s)."""
    itx = I_TX_MA[tx]
    c_wake = I_IDLE_MA * (T_WAKE_MS / 1000.0)
    c_tx = ATTEMPTS * itx * (T_TX_MS / 1000.0)
    c_rx = ATTEMPTS * I_RX_MA * (T_RX_REPLY_MS / 1000.0)
    c_mcu = I_MCU_ACTIVE_MA * ((T_DEBOUNCE_MS + 20.0) / 1000.0) + (I_WDT_UA / 1000.0) * ((T_LED_SUCCESS_MS + T_LOCKOUT_MS) / 1000.0)
    c_led = I_LED_MA * (T_LED_SUCCESS_MS / 1000.0)
    c = c_wake + c_tx + c_rx + c_mcu + c_led
    if POLL_ENABLE:
        polls = POLL_WINDOW_S / POLL_PERIOD_S
        c_poll = (I_IDLE_MA * T_WAKE_MS / 1000.0 + itx * T_TX_MS / 1000.0 + I_RX_MA * T_RX_REPLY_MS / 1000.0
                  + I_MCU_ACTIVE_MA * 0.010) * polls + (I_WDT_UA / 1000.0) * POLL_WINDOW_S
        c += c_poll
    return c


def life_years(batt, calls_per_day, sleep_ua, tx="0 dBm"):
    rated, usable, *_ = BATTERIES[batt]
    cap_mah = rated * usable
    per_day_mah = calls_per_day * per_call_mc(tx) / 3600.0 + sleep_ua / 1000.0 * 24.0
    return cap_mah / per_day_mah / 365.0, per_day_mah


def sag(batt, tx="0 dBm", c_eff_uf=10.0):
    """Worst-case terminal voltage during a TX pulse (single-pole model, cell resistance + local capacitor)."""
    import math
    _, _, r_fresh, r_eol, v_fresh, v_end = BATTERIES[batt]
    itx = I_TX_MA[tx] / 1000.0
    t = T_TX_MS / 1000.0
    out = {}
    for label, r, v in (("fresh", r_fresh, v_fresh), ("end of life", r_eol, v_end)):
        no_cap = v - itx * r
        tau = r * c_eff_uf * 1e-6
        # cap supplies the pulse first; the cell recharges it with time constant tau
        with_cap = v - itx * r * (1 - math.exp(-t / tau)) if tau > 0 else no_cap
        out[label] = (no_cap, with_cap)
    return out


if __name__ == "__main__":
    print("Charge per call (0 dBm): %.2f mC (%.4f mAh), polling %s" % (per_call_mc(), per_call_mc() / 3600.0,
                                                                         "on" if POLL_ENABLE else "off"))
    print("  (LED and lockout periods are spent in power-down; the success LED itself is ~1 mC)\n")
    for batt in BATTERIES:
        print("== %s (rated %d mAh, %.0f%% assumed usable)" % (batt, BATTERIES[batt][0], BATTERIES[batt][1] * 100))
        print("   calls/day | optimistic sleep 2uA | assumed 3uA | pessimistic 10uA   (years)")
        for n in (10, 20, 50, 100, 200):
            row = [life_years(batt, n, s)[0] for s in I_SLEEP_UA.values()]
            print("   %9d | %20.1f | %11.1f | %17.1f" % (n, *row))
        print("   TX pulse sag at 0 dBm (12 mA), 10 uF effective:")
        for label, (nc, wc) in sag(batt).items():
            ok = "OK" if wc >= 2.0 else "BELOW 2.0 V - radio/MCU may reset"
            print("     %-12s no cap %.2f V, with cap %.2f V  -> %s" % (label, nc, wc, ok))
        print()
    print("Shelf life / self-discharge (CR2450 ~1 %/year) caps real-world life at roughly 5-8 years regardless of the numbers above.")
