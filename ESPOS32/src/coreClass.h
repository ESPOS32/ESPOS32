#pragma once
#include <Arduino.h>
#include <functional>

extern bool isDebug;

#define SP Serial.println
#define DEBUGF(msg, ...) _debug(F(msg), ##__VA_ARGS__)
#define DEBUG(...) _debug(__VA_ARGS__)
inline void _debug(const __FlashStringHelper *msg) {
    if (!isDebug) return;
    Serial.println(msg);
}
inline void _debug(const __FlashStringHelper *msg, int mode) {
    if (!isDebug) return;
    if (mode == 1) Serial.print(msg);
    else Serial.println(msg);
}
template<typename T>
inline void _debug(const T &msg) {
    if (!isDebug) return;
    Serial.println(msg);
}
template<typename T>
inline void _debug(const T &msg, int mode) {
    if (!isDebug) return;
    if (mode == 1) Serial.print(msg);
    else Serial.println(msg);
}
template<typename T>
inline void _debug(const T &msg, bool newline) {
    if (!isDebug) return;
    if (newline) Serial.println(msg);
    else Serial.print(msg);
}
static const char debugSeparator[] = "|-------------------------------------------";


class BlinkTask {
public:
    using Callback = std::function<void()>;

    BlinkTask() {
        registerTask(this);
    }

    // --- START ---
    void start(Callback startCb,
               uint32_t on_ms,
               Callback endCb,
               uint32_t wait_ms,
               uint32_t repeatCount,
               Callback finishCb)
    {
        startCallback  = startCb;
        endCallback    = endCb;
        finishCallback = finishCb;

        durationOn   = on_ms;
        durationWait = wait_ms;

        repeat    = repeatCount;
        remaining = repeatCount;   // 0 = infinite

        uint32_t now = lastTickTime ? lastTickTime : millis();

        state        = STATE_START;
        pausedState  = STATE_IDLE;
        pausedElapsed = 0;
        pausedRemainingTotal = 0;
        finishedFlag  = false;

        t_state_enter = now;
        t_first_start = now;
    }

    // --- PAUSE ---
    void pause() {
        if (state == STATE_PAUSED || state == STATE_FINISHED)
            return;

        pausedElapsed = lastTickTime - t_state_enter;
        pausedState   = state;

        // remainingTotal() PAUSE alatt megáll → elmentjük
        if (repeat == 0) {
            pausedRemainingTotal = 0xFFFFFFFF;
        } else {
            uint64_t total = (uint64_t)(durationOn + durationWait) * repeat;
            uint64_t el    = elapsedTotal();
            pausedRemainingTotal = (el >= total) ? 0 : (uint32_t)(total - el);
        }

        state = STATE_PAUSED;
    }

    // --- RESUME ---
    void resume() {
        if (state != STATE_PAUSED)
            return;

        t_state_enter = lastTickTime - pausedElapsed;
        state         = pausedState;
    }

    // --- STOP (finish nélkül) ---
    void stop() {
        state = STATE_FINISHED;
    }

    // --- END (finish-sel) ---
    void end() {
        state = STATE_FINISHED;

        if (!finishedFlag) {
            finishedFlag = true;
            if (finishCallback) finishCallback();
        }
    }

    bool isPaused() const   { return state == STATE_PAUSED; }
    bool isFinished() const { return state == STATE_FINISHED; }
    bool isRunning() const  { return state != STATE_PAUSED && state != STATE_FINISHED; }

    // --- STATIC TICK ---
    static void tickAll() {
        uint32_t now = millis();
        lastTickTime = now;

        for (uint16_t i = 0; i < taskCount; i++) {
            tasks[i]->tick(now);
        }
    }

    // --- PHASE TIMING ---
    uint32_t elapsedPhase() const {
        if (state == STATE_PAUSED)
            return pausedElapsed;

        return lastTickTime - t_state_enter;
    }

    uint32_t remainingPhase() const {
        bool onPhase =
            (state == STATE_ON) ||
            (state == STATE_PAUSED && pausedState == STATE_ON);

        uint32_t dur = onPhase ? durationOn : durationWait;
        uint32_t el  = elapsedPhase();
        return (el >= dur) ? 0 : (dur - el);
    }

    // --- TOTAL TIMING ---
    uint32_t elapsedTotal() const {
        // A total elapsed NEM áll meg PAUSE alatt
        return lastTickTime - t_first_start;
    }

    uint32_t remainingTotal() const {
        // PAUSE alatt a remainingTotal MEGÁLL
        if (state == STATE_PAUSED)
            return pausedRemainingTotal;

        if (repeat == 0) return 0xFFFFFFFF;

        uint64_t total = (uint64_t)(durationOn + durationWait) * repeat;
        uint64_t el    = elapsedTotal();

        return (el >= total) ? 0 : (uint32_t)(total - el);
    }

    // --- PHASE QUERY ---
    // 0 = stopped/idle
    // 1 = ON
    // 2 = WAIT
    // 3 = finished
    uint8_t phase() const {
        if (state == STATE_FINISHED)
            return 3;

        if (state == STATE_PAUSED) {
            if (pausedState == STATE_ON)  return 1;
            if (pausedState == STATE_WAIT) return 2;
            return 0;
        }

        if (state == STATE_ON)  return 1;
        if (state == STATE_WAIT) return 2;

        return 0;
    }

private:
    enum State {
        STATE_IDLE,
        STATE_START,
        STATE_ON,
        STATE_WAIT,
        STATE_PAUSED,
        STATE_FINISHED
    };

    State state       = STATE_IDLE;
    State pausedState = STATE_IDLE;

    uint32_t durationOn   = 0;
    uint32_t durationWait = 0;

    uint32_t repeat    = 0;
    uint32_t remaining = 0;

    uint32_t t_state_enter = 0;
    uint32_t t_first_start = 0;

    uint32_t pausedElapsed = 0;
    uint32_t pausedRemainingTotal = 0;

    bool finishedFlag = false;

    Callback startCallback  = nullptr;
    Callback endCallback    = nullptr;
    Callback finishCallback = nullptr;

    // --- INTERNAL TICK ---
    void tick(uint32_t now) {
        switch (state) {

        case STATE_START:
            if (startCallback) startCallback();
            t_state_enter = now;
            state = STATE_ON;
            break;

        case STATE_ON:
            if (now - t_state_enter >= durationOn) {
                if (endCallback) endCallback();
                t_state_enter = now;
                state = STATE_WAIT;
            }
            break;

        case STATE_WAIT:
            if (now - t_state_enter >= durationWait) {

                if (repeat == 0) {
                    state = STATE_START;
                    t_state_enter = now;
                    break;
                }

                if (remaining > 0) remaining--;

                if (remaining == 0) {
                    end();   // természetes befejezés
                } else {
                    state = STATE_START;
                    t_state_enter = now;
                }
            }
            break;

        case STATE_PAUSED:
            break;

        case STATE_FINISHED:
            break;

        default:
            break;
        }
    }

    // --- TASK REGISTRY ---
    static void registerTask(BlinkTask* t) {
        if (taskCount < MAX_TASKS) {
            tasks[taskCount++] = t;
        }
    }

    static constexpr uint16_t MAX_TASKS = 32;
    static BlinkTask* tasks[MAX_TASKS];
    static uint16_t taskCount;

    static uint32_t lastTickTime;
};

// static init
BlinkTask* BlinkTask::tasks[MAX_TASKS] = { nullptr };
uint16_t BlinkTask::taskCount = 0;
uint32_t BlinkTask::lastTickTime = 0;
