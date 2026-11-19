#ifndef _OS3K_POWER_H_
#define _OS3K_POWER_H_

/** @file os3k_power.h
 * @brief Idle CPU suspension and automatic power-off policy.
 *
 * The flags, active timeout, and saved user timeout are global.
 * The interfaces below apply to AS3000 3.01 and Neo 3.21 firmware.
 */

#include <stdbool.h>
#include <stdint.h>

/** @brief Read the flag permitting the ROM idle path to suspend the CPU.
 *
 * Distinct from the automatic power-off flag: idle suspension
 * can end on an interrupt without turning the device off. The consumer tests
 * for exactly 1; normal boot and SetIdleSleepEnabled() supply canonical values.
 * @return True when idle suspension is permitted.
 */
bool IsIdleSleepEnabled(void);

/** @brief Permit or suppress CPU suspension in the ROM idle path.
 *
 * Only updates the flag; does not itself suspend, power off,
 * or change the automatic power-off alarm. Disable it temporarily when an operation requires continued CPU execution.
 * @param enabled True to permit idle suspension, false to suppress it.
 */
void SetIdleSleepEnabled(bool enabled);

/** @brief Read whether automatic power-off alarm scheduling is enabled.
 *
 * This is separate from IsIdleSleepEnabled().
 * @return True when automatic power-off scheduling is enabled.
 */
bool IsAutoPowerOffEnabled(void);

/** @brief Enable/disable automatic power-off and update the RTC alarm on a change.
 *
 * When the flag changes, enabling with a nonzero active timeout
 * schedules the RTC alarm for now plus timeout_minutes * 60 seconds; disabling
 * clears the RTC interrupt-enable register and masks the RTC interrupt. Repeating the same flag value does nothing,
 * so it does not refresh the alarm after a timeout-only change. No applet
 * message is processed by the setter. RTC scheduling is distinct from the
 * millisecond TimerStart() service.
 * @param enabled True to schedule automatic power-off, false to disable its alarm.
 */
void SetAutoPowerOffEnabled(bool enabled);

/** @brief Read the current automatic power-off interval in minutes.
 *
 * This can differ from the saved user preference after a
 * temporary change. Fresh initialization uses four minutes.
 * @return Active timeout byte; the public setter accepts 4 through 59.
 */
uint8_t GetAutoPowerOffMinutes(void);

/** @brief Temporarily replace the active automatic power-off interval.
 *
 * Changes the active byte only, retaining the user's saved
 * timeout. Does not itself rearm the RTC alarm. Call
 * RestoreAutoPowerOffMinutes() to discard a temporary override.
 * @param minutes Interval from 4 through 59 inclusive. Invalid values leave state unchanged.
 * @return 0 on success, -1 for an out-of-range value (full signed 32-bit result).
 */
int32_t SetAutoPowerOffMinutes(uint8_t minutes);

/** @brief Restore the active automatic power-off interval from the saved preference.
 *
 * Does not itself rearm the RTC alarm or change either flag.
 * The ROM's user-setting dialog updates both saved and active timeout bytes;
 * SetAutoPowerOffMinutes() updates only the active one.
 */
void RestoreAutoPowerOffMinutes(void);

/** @brief Battery chemistry selected by the ROM's hardware detection.
 *
 * The unknown value preserves the full 32-bit return convention with -fshort-enums.
 */
typedef enum _BatteryType_e {
    BATTERY_TYPE_NIMH = 0, /**< Rechargeable NiMH discharge curve. */
    BATTERY_TYPE_ALKALINE = 1, /**< Alkaline discharge curve. */
    BATTERY_TYPE_UNKNOWN = UINT32_MAX /**< Measurement/type detection is not initialized or unavailable. */
} BatteryType_e;

/** @brief Select a battery diagnostic value or factory calibration operation.
 *
 * Selectors 0–10 read cached fields. They do not refresh measurements. The
 * charging counters are opaque implementation state, not physical measurements.
 * All results are 32-bit integers, including zero-extended word fields. No pointer result or output structure
 * is involved. The calibration selector has materially different side effects.
 */
typedef enum _BatteryDiagnostic_e {
    BATTERY_DIAG_CHARGE_ACTIVE = 0, /**< Opaque charging-control word (0 or 1), not a bitmask. */
    BATTERY_DIAG_CALIBRATION = 1, /**< Current 32-bit calibration divisor used to normalize the raw sample. */
    BATTERY_DIAG_RAW_SAMPLE = 2, /**< Last timing-loop sample, before calibration. */
    BATTERY_DIAG_NORMALIZED_SAMPLE = 3, /**< Calibrated sample used by capacity and voltage queries. */
    BATTERY_DIAG_CHARGE_REFERENCE_SAMPLE = 4, /**< Cached charging reference in raw-sample units, not millivolts. */
    BATTERY_DIAG_CHARGE_MINIMUM_SAMPLE = 5, /**< Cached minimum charging sample in raw-sample units. */
    BATTERY_DIAG_CHARGE_PLATEAU_COUNT = 6, /**< Opaque charging-monitor counter, not elapsed time. */
    BATTERY_DIAG_CHARGE_STABILITY_COUNT = 7, /**< Opaque charging-stability counter, not elapsed time. */
    BATTERY_DIAG_CHARGE_PHASE = 8, /**< Opaque charging-monitoring phase word. */
    BATTERY_DIAG_CHARGE_STATE = 9, /**< Cached charging state-machine word. */
    BATTERY_DIAG_CHARGE_TIMER = 10, /**< Opaque charging state-machine count, not elapsed time. */
    BATTERY_DIAG_STORED_CALIBRATION = 41, /**< Read calibration at flash 0x4003F0 after the ROM validates its small-ROM descriptor; returns zero if validation fails. */
    BATTERY_DIAG_CALIBRATE = 42, /**< Factory A/D calibration: take sixteen measurements, reject samples differing from their mean by more than 20, then replace calibration and attempt to persist it to flash. Requires the factory reference setup: AC power with batteries removed. Returns 1 for unstable samples, otherwise 0; even a stable zero sample is accepted. Zero is not proof of a valid divisor or successful flash persistence. */
    BATTERY_DIAG_INVALID = UINT32_MAX /**< Unsupported selector; returns zero. Also fixes this enum's width at 32 bits. */
} BatteryDiagnostic_e;

/** @brief Return the hardware revision code used by battery support.
 *
 * Returns a cached firmware revision identifier.
 * Battery measurement/display
 * paths treat zero as unsupported and nonzero as supported. This is a numeric
 * revision code, not a canonical bool or a Neo 2/wireless detector.
 * @return Cached unsigned revision byte (AS3000 3.01 0–7, Neo 3.21 0x10).
 */
uint8_t GetHardwareRevision(void);

/** @brief Refresh the cached battery measurement through the hardware timing path.
 *
 * Does nothing when GetHardwareRevision() is zero. Otherwise
 * updates the raw and calibrated samples used by the battery queries.
 * The calibration divisor must be nonzero; the ROM does not guard division by
 * zero. This does not return the measurement;
 * use GetBatteryDischargePercent(), GetBatteryMillivolts(), or BatteryDiagnostic().
 * AS3000 3.01 also samples battery chemistry; Neo's chemistry is detected during
 * initialization rather than by this refresh. A missing/immediate hardware
 * transition can produce zero.
 */
void RefreshBatteryMeasurements(void);

/** @brief Read the cached battery chemistry without refreshing hardware.
 *
 * Values 0 and 1 denote NiMH and alkaline,
 * respectively. Unknown initial state is 0xFFFFFFFF.
 * @return Cached BatteryType_e value; full 32-bit result in D0.
 */
BatteryType_e GetBatteryType(void);

/** @brief Estimate the percentage of battery capacity already consumed.
 *
 * Evaluates a ROM piecewise-linear curve against the cached
 * normalized sample, selecting the NiMH or alkaline curve. Zero means full;
 * 100 means exhausted. The battery display shows 100 minus this value as its
 * remaining capacity. Unknown chemistry returns zero, so zero alone does not
 * establish a valid full-battery measurement. No hardware sample is taken.
 * @return Unsigned percentage consumed, 0 through 100 for valid curve inputs.
 */
uint32_t GetBatteryDischargePercent(void);

/** @brief Convert the cached normalized battery sample to millivolts.
 *
 * Applies the firmware conversion to the cached sample.
 * No hardware refresh is performed and neither chemistry nor calibration
 * validity is checked here.
 * @return Estimated millivolts, or zero when the cached sample is zero.
 */
uint32_t GetBatteryMillivolts(void);

/** @brief Read battery diagnostic state or invoke factory calibration.
 *
 * The numeric selector is a full 32-bit value. Read-only
 * selectors expose cached state; BATTERY_DIAG_CALIBRATE measures and can write
 * factory calibration. Unsupported selectors return zero and do nothing.
 * @param selector BatteryDiagnostic_e operation to perform.
 * @return Selected value or operation result as documented on the enum entry.
 */
uint32_t BatteryDiagnostic(BatteryDiagnostic_e selector);

#endif
