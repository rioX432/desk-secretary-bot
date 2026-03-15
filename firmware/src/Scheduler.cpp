#include <Arduino.h>
#include <string.h>
#include "Scheduler.h"
#include "Robot.h"
#include "driver/PlayMP3.h"

ScheduleBase* scheduleList[MAX_SCHED_NUM] = {nullptr};


void add_schedule(ScheduleBase* schedule)
{
    for(int i=0; i<MAX_SCHED_NUM; i++){
        if(scheduleList[i]==nullptr){
            scheduleList[i] = schedule;
            Serial.printf("add_schedule: %d\n",i);
            break;
        }
    }
}

ScheduleBase* get_schedule(int idx)
{
    if(idx > MAX_SCHED_NUM){
        return nullptr;
    }
    return scheduleList[idx];
}

void run_schedule(void)
{
    struct tm now_time;
    
    if (!getLocalTime(&now_time)) {
        Serial.printf("failed to get time\n");
        return;
    }

    for(int i=0; i<MAX_SCHED_NUM; i++){
        if(scheduleList[i]!=nullptr){
            //Serial.printf("run schedule: %d\n",i);
            ScheduleBase* schedule = scheduleList[i];
            schedule->run(now_time);

            if(schedule->destroy){
                Serial.printf("destroy schedule: %d\n",i);
                delete schedule;
                scheduleList[i] = nullptr;
            }

        }

        
    }
}



ScheduleBase::ScheduleBase()
{
    destroy = false;
}

ScheduleEveryDay::ScheduleEveryDay(int hour, int min, void (*func)(void))
{
    struct tm now_time = {};

    sched_type = SCHED_EVERY_DAY;
    sched_hour = hour;
    sched_min = min;
    callback = func;

    //If the time has already passed, set the status to executed.
    if (getLocalTime(&now_time)) {
        int sched = sched_hour * 60 + sched_min;
        int now = now_time.tm_hour * 60 + now_time.tm_min;

        if(now >= sched){
            is_executed = true;
        }else{
            is_executed = false;
        }

        prev_time = now_time;
    }
    else{
        Serial.printf("failed to get time\n");
        is_executed = false;
        prev_time = now_time;
    }

}

void ScheduleEveryDay::run(struct tm now_time)
{
    int sched = sched_hour * 60 + sched_min;
    int now = now_time.tm_hour * 60 + now_time.tm_min;

    if(now >= sched){
        if(!is_executed){
            Serial.printf("Schedule::run callback()\n");
            callback();
            is_executed = true;
        }
    }

    // Reset schedule when date is updated.
    if(prev_time.tm_hour == 23 && now_time.tm_hour == 0){
        Serial.printf("Reset schedule\n");
        is_executed = false;
    }
    prev_time = now_time;
}



ScheduleEveryHour::ScheduleEveryHour(void (*func)(void), int _start_hour, int _end_hour)
{
    struct tm now_time = {};

    sched_type = SCHED_EVERY_HOUR;
    start_hour = _start_hour;
    end_hour = _end_hour;
    callback = func;

    getLocalTime(&now_time);
    prev_time = now_time;

    //Test code
    //prev_time.tm_hour = 22;
}

void ScheduleEveryHour::run(struct tm now_time)
{
    if(start_hour <= now_time.tm_hour && now_time.tm_hour <= end_hour){
        if(now_time.tm_hour != prev_time.tm_hour){
            Serial.printf("ScheduleEveryHour::run callback()\n");
            callback();
        }
    }
    prev_time = now_time;
}



ScheduleReminder::ScheduleReminder(int hour, int min, String _remind_text)
{
    struct tm now_time = {};

    sched_type = SCHED_REMINDER;
    sched_hour = hour;
    sched_min = min;
    remind_text = _remind_text;

    //If the time has already passed, set the status to executed.
    if (getLocalTime(&now_time)) {
        int sched = sched_hour * 60 + sched_min;
        int now = now_time.tm_hour * 60 + now_time.tm_min;

        if(now >= sched){
            is_executed = true;
            destroy = true;
        }else{
            is_executed = false;
        }
    }
    else{
        Serial.printf("failed to get time\n");
        is_executed = false;
    }

}

#include "llm/ChatGPT/FunctionCall.h"
void ScheduleReminder::run(struct tm now_time)
{
    int sched = sched_hour * 60 + sched_min;
    int now = now_time.tm_hour * 60 + now_time.tm_min;

    if(now >= sched){
        if(!is_executed){
            Serial.printf("ScheduleReminder::run callback()\n");

            String fname = String(APP_DATA_PATH) + String(FNAME_ALARM_MP3);
            playMP3SD(fname.c_str());
            robot->speech(remind_text + "、をリマインドします");

            is_executed = true;
            destroy = true;

        }
    }


}

String ScheduleReminder::get_time_string()
{
    return String(sched_hour) + String(":") + String(sched_min);
}

ScheduleIntervalMinute::ScheduleIntervalMinute(int _interval_min, void (*func)(void), int _start_hour, int _end_hour)
{
    struct tm now_time = {};

    sched_type = SCHED_INTERVAL_MINUTE;
    interval_min = _interval_min;
    start_hour = _start_hour;
    end_hour = _end_hour;
    callback = func;

    getLocalTime(&now_time);
    prev_time = now_time;

}

void ScheduleIntervalMinute::run(struct tm now_time)
{
    if(start_hour <= now_time.tm_hour && now_time.tm_hour <= end_hour){
        int diff = now_time.tm_min - prev_time.tm_min;
        if(diff < 0){
            diff = now_time.tm_min + (60 - prev_time.tm_min);
        }

        if(diff >= interval_min){
            Serial.printf("ScheduleIntervalMinute::run callback()\n");
            callback();
            prev_time = now_time;
        }

    }
}


// ---- ScheduleCron ----

ScheduleCron::ScheduleCron(const char* _name, const char* _cron, const char* _action, bool _enabled)
{
    sched_type = SCHED_CRON;
    name = String(_name);
    cron_expr = String(_cron);
    action = String(_action);
    enabled = _enabled;
    last_run_min = -1;
    callback = nullptr;

    min_count = hour_count = dom_count = mon_count = dow_count = 0;
    min_any = hour_any = dom_any = mon_any = dow_any = false;

    parseCron();
}

bool ScheduleCron::parseField(const char* field, int* vals, int& count, bool& isAny)
{
    count = 0;
    isAny = false;

    if (strcmp(field, "*") == 0) {
        isAny = true;
        return true;
    }

    // Parse comma-separated values
    char buf[64];
    strncpy(buf, field, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* token = strtok(buf, ",");
    while (token != nullptr && count < MAX_VALS) {
        vals[count++] = atoi(token);
        token = strtok(nullptr, ",");
    }

    return count > 0;
}

void ScheduleCron::parseCron()
{
    // Parse 5-field cron: min hour dom month dow
    char buf[128];
    strncpy(buf, cron_expr.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* fields[5] = {nullptr};
    int fi = 0;
    char* token = strtok(buf, " ");
    while (token != nullptr && fi < 5) {
        fields[fi++] = token;
        token = strtok(nullptr, " ");
    }

    if (fi < 5) {
        Serial.printf("[CRON] Invalid cron expression (need 5 fields): %s\n", cron_expr.c_str());
        enabled = false;
        return;
    }

    parseField(fields[0], min_vals, min_count, min_any);
    parseField(fields[1], hour_vals, hour_count, hour_any);
    parseField(fields[2], dom_vals, dom_count, dom_any);
    parseField(fields[3], mon_vals, mon_count, mon_any);
    parseField(fields[4], dow_vals, dow_count, dow_any);

    Serial.printf("[CRON] Parsed schedule '%s': %s -> %s\n",
                  name.c_str(), cron_expr.c_str(), action.c_str());
}

bool ScheduleCron::matchField(int value, int* vals, int count, bool isAny)
{
    if (isAny) return true;
    for (int i = 0; i < count; i++) {
        if (vals[i] == value) return true;
    }
    return false;
}

void ScheduleCron::run(struct tm now_time)
{
    if (!enabled) return;

    // Compute a unique minute identifier to avoid duplicate execution
    int current_min_id = now_time.tm_hour * 60 + now_time.tm_min;
    if (current_min_id == last_run_min) return;

    // Check all 5 cron fields
    bool match = matchField(now_time.tm_min, min_vals, min_count, min_any)
              && matchField(now_time.tm_hour, hour_vals, hour_count, hour_any)
              && matchField(now_time.tm_mday, dom_vals, dom_count, dom_any)
              && matchField(now_time.tm_mon + 1, mon_vals, mon_count, mon_any)  // tm_mon is 0-11
              && matchField(now_time.tm_wday, dow_vals, dow_count, dow_any);     // tm_wday: 0=Sun

    if (match) {
        Serial.printf("[CRON] Firing schedule '%s': %s\n", name.c_str(), action.c_str());
        last_run_min = current_min_id;

        // Send action text to LLM as if user spoke it
        if (robot != nullptr && robot->llm != nullptr) {
            robot->chat(action);
        }
    }
}


// ---- Schedule management helpers ----

bool remove_schedule_by_name(const char* name)
{
    for (int i = 0; i < MAX_SCHED_NUM; i++) {
        if (scheduleList[i] != nullptr &&
            scheduleList[i]->get_sched_type() == SCHED_CRON) {
            ScheduleCron* cron = (ScheduleCron*)scheduleList[i];
            if (cron->getName() == String(name)) {
                Serial.printf("[CRON] Removing schedule '%s' at idx %d\n", name, i);
                delete cron;
                scheduleList[i] = nullptr;
                return true;
            }
        }
    }
    return false;
}

ScheduleCron* find_cron_schedule(const char* name)
{
    for (int i = 0; i < MAX_SCHED_NUM; i++) {
        if (scheduleList[i] != nullptr &&
            scheduleList[i]->get_sched_type() == SCHED_CRON) {
            ScheduleCron* cron = (ScheduleCron*)scheduleList[i];
            if (cron->getName() == String(name)) {
                return cron;
            }
        }
    }
    return nullptr;
}

int get_cron_schedule_count()
{
    int count = 0;
    for (int i = 0; i < MAX_SCHED_NUM; i++) {
        if (scheduleList[i] != nullptr &&
            scheduleList[i]->get_sched_type() == SCHED_CRON) {
            count++;
        }
    }
    return count;
}

