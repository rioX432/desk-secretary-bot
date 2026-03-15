#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <ArduinoYaml.h>
#include "Scheduler.h"
#include "llm/ChatGPT/ChatGPT.h"
#include "llm/ChatGPT/FunctionCall.h"
#include "Robot.h"
#include "MySchedule.h"
#include "Expression.h"
#include <Avatar.h>
using namespace m5avatar;

extern Avatar avatar;

#define CRON_SCHEDULE_FILE  "/app/AiStackChanEx/SC_Schedule.yaml"

void sched_fn_sleep(void)
{
    Serial.printf("sched_fn_sleep\n");
    setCpuFrequencyMhz(80);                         // 動作周波数を落とす
    M5.Display.setBrightness(8);                    // LDCの輝度を下げる
    avatar.setExpression(Expression::Sleepy);
}

void sched_fn_wake(void)
{
    Serial.printf("sched_fn_wake\n");
    setCpuFrequencyMhz(240);                         // 動作周波数を元に戻す
    M5.Display.setBrightness(127);                   // LDCの輝度を元に戻す
    avatar.setExpression(Expression::Neutral);
}

void sched_fn_morning_info(void)
{
    Serial.printf("sched_fn_morning_info\n");
    robot->llm->chat("今日の日付と天気とメモの内容を教えて。海上の天気は不要です。");
}

void sched_fn_announce_time(void)
{
    struct tm now_time;
    Serial.printf("sched_fn_announce_time\n");   

    if (!getLocalTime(&now_time)) {
        Serial.printf("failed to get time\n");
        return;
    }

    robot->speech(String(now_time.tm_hour) + "時です");
}

void init_schedule(void)
{
//    add_schedule(new ScheduleEveryDay(6, 30, sched_fn_wake));               //6:30 省エネモードから復帰
//    if(robot->m_config.getExConfig().llm.type == LLM_TYPE_CHATGPT){
//        //※今日の天気などはChatGPTのFunction Callingが前提の機能のため、その他LLM有効時は登録しない
//        add_schedule(new ScheduleEveryDay(7, 00, sched_fn_morning_info));       //7:00 今日の日付、天気、メモの内容を話す
//        add_schedule(new ScheduleEveryDay(7, 30, sched_fn_morning_info));       //7:30 同上
//    }
//    add_schedule(new ScheduleEveryDay(23, 30, sched_fn_sleep));             //23:30 省エネモードに遷移
//    add_schedule(new ScheduleEveryHour(sched_fn_announce_time, 7, 23));     //時報（7時から23時の間）
//    add_schedule(new ScheduleIntervalMinute(5, sched_fn_recv_mail, 7, 23)); //5分置きに受信メールを確認（7時から23時の間）

    // Load cron schedules from SD card
    loadCronSchedules();
}


bool loadCronSchedules()
{
    if (!SD.exists(CRON_SCHEDULE_FILE)) {
        Serial.printf("[CRON] Schedule file not found: %s\n", CRON_SCHEDULE_FILE);
        return false;
    }

    File file = SD.open(CRON_SCHEDULE_FILE, FILE_READ);
    if (!file) {
        Serial.printf("[CRON] Failed to open: %s\n", CRON_SCHEDULE_FILE);
        return false;
    }

    DynamicJsonDocument doc(2048);
    auto err = deserializeYml(doc, file);
    file.close();

    if (err) {
        Serial.printf("[CRON] YAML parse error: %s\n", err.c_str());
        return false;
    }

    if (!doc["schedules"].is<JsonArray>()) {
        Serial.printf("[CRON] No 'schedules' array in YAML\n");
        return false;
    }

    JsonArray schedules = doc["schedules"];
    int count = 0;
    for (JsonObject sched : schedules) {
        const char* name = sched["name"];
        const char* cron = sched["cron"];
        const char* action = sched["action"];
        bool enabled = sched["enabled"] | true;

        if (!name || !cron || !action) {
            Serial.printf("[CRON] Skipping invalid entry (missing fields)\n");
            continue;
        }

        // Skip if a schedule with this name already exists
        if (find_cron_schedule(name) != nullptr) {
            Serial.printf("[CRON] Schedule '%s' already exists, skipping\n", name);
            continue;
        }

        add_schedule(new ScheduleCron(name, cron, action, enabled));
        count++;
    }

    Serial.printf("[CRON] Loaded %d schedule(s) from SD\n", count);
    return true;
}


bool saveCronSchedules()
{
    DynamicJsonDocument doc(2048);
    JsonArray arr = doc.createNestedArray("schedules");

    for (int i = 0; i < MAX_SCHED_NUM; i++) {
        ScheduleBase* sched = get_schedule(i);
        if (sched != nullptr && sched->get_sched_type() == SCHED_CRON) {
            ScheduleCron* cron = (ScheduleCron*)sched;
            JsonObject obj = arr.createNestedObject();
            obj["name"] = cron->getName();
            obj["cron"] = cron->getCronExpr();
            obj["action"] = cron->getAction();
            obj["enabled"] = cron->isEnabled();
        }
    }

    File file = SD.open(CRON_SCHEDULE_FILE, FILE_WRITE);
    if (!file) {
        Serial.printf("[CRON] Failed to open for write: %s\n", CRON_SCHEDULE_FILE);
        return false;
    }

    serializeYml(doc, file);
    file.close();

    Serial.printf("[CRON] Saved %d schedule(s) to SD\n", arr.size());
    return true;
}