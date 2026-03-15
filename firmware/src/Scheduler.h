#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <Arduino.h>

#define MAX_SCHED_NUM   (50)
#define CRON_MAX_NAME_LEN   (32)
#define CRON_MAX_ACTION_LEN (200)

typedef enum e_sched_type
{
    SCHED_EVERY_DAY,
    SCHED_EVERY_HOUR,
    SCHED_REMINDER,
    SCHED_INTERVAL_MINUTE,
    SCHED_CRON
} SCHED_TYPE;

class ScheduleBase{
protected:
    SCHED_TYPE sched_type;      //子クラスの判別用（本当はtypeinfoを使いたいがビルドに失敗した）
    struct tm prev_time;
    void (*callback)(void);

public:
    bool destroy;
    ScheduleBase();
    virtual void run(struct tm now_time);
    SCHED_TYPE get_sched_type(){ return sched_type; };

};


class ScheduleEveryDay: public ScheduleBase{
private:
    int sched_hour;
    int sched_min;
    int is_executed;
public:
    ScheduleEveryDay(int hour, int min, void (*func)(void));
    void run(struct tm now_time);
};

class ScheduleEveryHour: public ScheduleBase{
private:
    int start_hour;
    int end_hour;
public:
    ScheduleEveryHour(void (*func)(void), int _start_hour, int _end_hour);
    void run (struct tm now_time);
};


class ScheduleReminder: public ScheduleBase{
private:
    int sched_hour;
    int sched_min;
    int is_executed;
    String remind_text;
public:
    ScheduleReminder(int hour, int min, String _remind_text);
    void run(struct tm now_time);
    String get_time_string();
    String get_remind_string(){ return remind_text; };
};


class ScheduleIntervalMinute: public ScheduleBase{
private:
    int interval_min;
    int start_hour;
    int end_hour;
    int is_executed;
    String remind_text;
public:
    ScheduleIntervalMinute(int _interval_min, void (*func)(void), int _start_hour, int _end_hour);
    void run(struct tm now_time);
};


class ScheduleCron: public ScheduleBase{
private:
    String name;
    String cron_expr;   // 5-field cron: min hour dom month dow
    String action;
    bool enabled;
    int last_run_min;   // minute of last execution (-1 = never)

    // Parsed cron fields (comma-separated lists stored as arrays)
    static const int MAX_VALS = 12;
    int min_vals[MAX_VALS];   int min_count;   bool min_any;
    int hour_vals[MAX_VALS];  int hour_count;  bool hour_any;
    int dom_vals[MAX_VALS];   int dom_count;   bool dom_any;
    int mon_vals[MAX_VALS];   int mon_count;   bool mon_any;
    int dow_vals[MAX_VALS];   int dow_count;   bool dow_any;

    void parseCron();
    bool parseField(const char* field, int* vals, int& count, bool& isAny);
    bool validateFieldRange(int* vals, int count, bool isAny, int minVal, int maxVal);
    bool matchField(int value, int* vals, int count, bool isAny);

public:
    ScheduleCron(const char* _name, const char* _cron, const char* _action, bool _enabled = true);
    void run(struct tm now_time);
    String getName() { return name; }
    String getCronExpr() { return cron_expr; }
    String getAction() { return action; }
    bool isEnabled() { return enabled; }
    void setEnabled(bool e) { enabled = e; }
};


extern void add_schedule(ScheduleBase* schedule);
extern ScheduleBase* get_schedule(int idx);
extern void run_schedule(void);
extern bool remove_schedule_by_name(const char* name);
extern ScheduleCron* find_cron_schedule(const char* name);
extern int get_cron_schedule_count();



#endif