# Battery life and voltage sag

Script: [`tools/battery_calc.py`](../tools/battery_calc.py). **All results are estimates from datasheet figures and the assumptions listed
in the script; nothing was measured.** Replace the assumptions with measured values (test plan 12, 13, 18) and rerun.

## Datasheet inputs (Si24R1, VDD = 3 V)

TX 12 mA at 0 dBm (16 mA at 4 dBm, 25 mA at 7 dBm); RX 14 mA at 250 kbps; idle-TX 380 uA; shutdown 1 uA; crystal start 1.5-2 ms.

## Assumptions

| Item | Value |
|---|---|
| Sleep current (MCU power-down + radio shutdown + leakage) | 3 uA (2 uA optimistic, 10 uA pessimistic) |
| Frame airtime at 250 kbps | 0.55 ms; 1.5 transmissions per call on average |
| Receive window for the application reply | 5 ms per attempt |
| MCU active current at 4 MHz | 1.5 mA, awake only about 50 ms per call (LED and lockout periods are power-down sleeps) |
| LED current | 1 mA for the 1 s success light |
| Served polling | every 8 s for 300 s (37 wake-ups), about 75 uC each |
| Usable capacity | CR2450 65 % of 620 mAh, CR2032 65 % of 225 mAh, 2xAAA 85 % of 1000 mAh |

## Result (run of the script)

Charge per call at 0 dBm: **about 5.9 mC (0.0016 mAh)** including the polling window. Estimated life with 3 uA sleep:

| Calls per day | CR2450 | CR2032 | 2 x AAA |
|---|---|---|---|
| 10 | about 12.5 years | about 4.5 years | about 26 years |
| 50 | about 7 years | about 2.6 years | about 15 years |
| 200 | about 2.8 years | about 1.0 year | about 6 years |

Real life is limited by cell shelf life and self-discharge (a CR2450 loses about 1 % per year, and alkaline cells have a 5-10 year shelf life),
so treat anything above 5-8 years as "not the limit". With 10 uA of sleep current the CR2450 figure at 20 calls/day drops to about 4 years;
**the sleep current is the number to measure first.**

## Sag during a TX pulse

Single-pole model: pulse current 12 mA for 0.55 ms through the cell's internal resistance, with 10 uF effective local capacitance.

| Cell | Fresh | Near end of life (assumed 100 ohm, 2.2 V) |
|---|---|---|
| CR2450 | about 2.8 V at the radio | about 1.7 V: below the 1.9 V radio minimum |
| 2 x AAA | about 3.1 V | about 2.0 V |

Conclusion: a CR2450 works while fresh, but the last part of its capacity is not usable at 12 mA pulses; the low-battery flag (below 2.4 V
measured unloaded) warns first. If the measured sag (test 12) is not comfortably above 2.2 V, **use 2 x AAA**, which the PCB supports on the
same rail, or lower `BELL_TX_POWER` to `-4 dBm` (9.5 mA is the datasheet figure for -6 dBm).

## Supply measurement in firmware

`pm_read_vcc_mv()` measures VCC against the 1.1 V bandgap (no divider, no extra drain). Accuracy is about +-10 % unless calibrated:
measure VCC with a meter once and adjust the constant 1125300 in `power_manager.cpp`.
