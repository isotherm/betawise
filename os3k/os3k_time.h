#ifndef _OS3K_TIME_H_
#define _OS3K_TIME_H_

/** @file os3k_time.h
 * @brief Firmware elapsed-time counters, blocking delays, and one-shot timers.
 *
 * The millisecond timer service and the hardware RTC are separate clocks.
 * Timers share five system-wide slots and expire once. Their notification
 * storage and callback code must remain valid until expiry or cancellation.
 */

#include <stdint.h>

/** @brief Opaque timer handle containing a slot and an eight-bit generation.
 *
 * Handles can be reused after the generation wraps. Retain a handle only for
 * the lifetime of its timer, and cancel timers before releasing their storage.
 */
typedef uint16_t TimerId_t;

/** @brief Values written to a timer flag and passed to its callback. */
typedef enum _TimerEvent_e {
    TIMER_EVENT_EXPIRED = 0x01, /**< The one-shot delay elapsed. */
    TIMER_EVENT_SERVICE_STOPPED = 0xFF /**< The ROM stopped the timer service, abandoning this timer. */
} TimerEvent_e;

/** @brief Callback for a one-shot timer.
 *
 * Expiry calls this directly from the timer interrupt, after writing the flag
 * and releasing the timer slot. Stopping the timer service also calls it, in
 * the context of that operation. Cancellation with TimerCancel() does not.
 * The ROM does not switch to the applet's A5 data base before calling this
 * pointer. Keep the handler short, avoid blocking calls, and arrange any
 * data base needed to access applet globals.
 * @param event Expiry or timer-service shutdown notification.
 */
typedef void (*TimerCallback_t)(TimerEvent_e event);

/** @brief Invalid timer handle written on allocation or range failure. */
#define TIMER_ID_INVALID UINT16_MAX
/** @brief Timer creation succeeded. */
#define TIMER_OK 0
/** @brief No timer slot is free, or the timer service is stopped. */
#define TIMER_ERROR_UNAVAILABLE (-301)
/** @brief Requested delay exceeds 10,000 milliseconds. */
#define TIMER_ERROR_DELAY_RANGE (-302)

/** @brief Busy-wait for a delay measured in hundredths of a second.
 *
 * With the normal timer service running, waits until
 * its centisecond counter reaches the initial count plus centiseconds plus
 * one. Even zero waits for the next centisecond boundary. This does not pump
 * applet messages. The deadline comparison is not safe across counter wrap.
 * With the timer service stopped, the ROM instead polls a hardware timer,
 * using the low 16 bits of centiseconds multiplied by ten with 16-bit wrap.
 * @param centiseconds Requested delay in units of 10 ms.
 */
void SleepCentiseconds(uint32_t centiseconds);

/** @brief Execute a short CPU-dependent busy delay.
 *
 * Runs a fixed instruction loop iterations times.
 * This does not use a clock, sleep the CPU, or pump applet messages. Timing
 * depends on CPU speed and interrupts; iterations are not a time unit.
 * @param iterations Number of loop iterations, from zero through 65,535.
 * @return Always zero, as a full 32-bit value.
 */
uint32_t BusyLoop(uint16_t iterations);

/** @brief Read elapsed whole seconds from the hardware RTC.
 *
 * Converts the RTC's nine-bit day count and hour,
 * minute, and second fields to seconds. Firmware initializes these fields to
 * zero at startup. This clock continues independently of the millisecond
 * timer service, including while that service is stopped. Its day field
 * wraps after 512 days. This is not a calendar timestamp.
 * @return RTC days * 86400 + hours * 3600 + minutes * 60 + seconds.
 */
uint32_t GetUptimeSeconds(void);

/** @brief Read the running timer-service counter in hundredths of a second.
 *
 * Divides the wrapping 32-bit millisecond count by
 * ten, rounding down. Consequently it resets when that millisecond count
 * wraps, rather than having a separate 32-bit centisecond wrap period.
 * @return Elapsed centiseconds, or zero with a ROM diagnostic if the timer
 * service is stopped.
 */
uint32_t GetUptimeCentiseconds(void);

/** @brief Read the running timer-service millisecond counter.
 *
 * Atomically combines the accumulated timer count
 * with the current hardware counter. The 32-bit result wraps after about
 * 49.7 days. This clock is separate from GetUptimeSeconds().
 * @return Elapsed milliseconds, or zero with a ROM diagnostic if the timer
 * service is stopped.
 */
uint32_t GetUptimeMilliseconds(void);

/** @brief Allocate a one-shot timer with optional flag and callback outputs.
 *
 * Uses the first free slot in a shared five-slot
 * pool. Delays below 3 ms are raised to 3 ms; values above 10,000 ms fail.
 * Timer creation does not initialize the notification byte. On expiry the
 * ROM releases the slot, writes TIMER_EVENT_EXPIRED to that byte if present,
 * then calls the callback if present. Stopping the service instead writes
 * and delivers TIMER_EVENT_SERVICE_STOPPED. Callers must initialize their
 * flag and keep it valid until the timer finishes or is cancelled.
 *
 * @param milliseconds Delay in milliseconds, from zero through 10,000.
 * @param notification Optional byte to receive a TimerEvent_e value. Use
 * volatile storage because expiry writes it from an interrupt.
 * @param callback Optional handler; see TimerCallback_t for its execution context.
 * @param timer_id Optional output handle for TimerCancel(). With the service
 * running this is first set to TIMER_ID_INVALID, then replaced on success.
 * It is left unchanged when the service is stopped.
 * @retval TIMER_OK Timer created.
 * @retval TIMER_ERROR_UNAVAILABLE All slots occupied, or service stopped.
 * @retval TIMER_ERROR_DELAY_RANGE Delay greater than 10,000 ms.
 */
int32_t TimerStart(uint16_t milliseconds, volatile uint8_t* notification,
                   TimerCallback_t callback, TimerId_t* timer_id);

/** @brief Cancel a timer without changing its flag or invoking its callback.
 *
 * Checks both the slot and generation in the handle.
 * An invalid, expired, already cancelled, or stale handle is ignored, as is
 * any request made while the timer service is stopped. Generation wrap can
 * eventually make an old handle refer to a later timer.
 * @param timer_id Handle returned by TimerStart().
 */
void TimerCancel(TimerId_t timer_id);

#endif
