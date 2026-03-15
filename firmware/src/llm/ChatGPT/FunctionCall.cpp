#include "FunctionCall.h"
#include <Avatar.h>
#include "Robot.h"
#include <AudioGeneratorMP3.h>
#include "driver/AudioOutputM5Speaker.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SD.h>
#include <SPIFFS.h>
#include "driver/WakeWord.h"
#include "Scheduler.h"
#include "MySchedule.h"
#include "StackchanExConfig.h"
#include "SDUtil.h"
#include "MCPClient.h"
#include "llm/LLMBase.h"
#include "rootCA/OpenWeatherMapRootCA.h"
using namespace m5avatar;



// 外部参照
extern Avatar avatar;
extern AudioGeneratorMP3 *mp3;
extern bool servo_home;
extern void sw_tone();

static String avatarText;

// Notepad content for save_note/read_note/delete_note
String note = "";

// OpenWeatherMap API key (loaded from SC_SecConfig.yaml)
String g_weather_api_key = "";

// タイマー機能関連
TimerHandle_t xAlarmTimer;
bool alarmTimerCallbacked = false;
bool alarmTimerCanceled = false;
void alarmTimerCallback(TimerHandle_t xTimer);
void powerOffTimerCallback(TimerHandle_t xTimer);



const String json_Functions =
"["
  "{"
    "\"name\": \"update_memory\","
    "\"description\": \"Update long-term memory.\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"memory\":{"
          "\"type\": \"string\","
          "\"description\": \"Summary of user attributes and memorable conversations.\""
        "}"
      "},"
      "\"required\": [\"memory\"]"
    "}"
  "},"
  "{"
    "\"name\": \"timer\","
    "\"description\": \"指定した時間が経過したら、指定した動作を実行する。指定できる動作はalarmとshutdown。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"time\":{"
          "\"type\": \"integer\","
          "\"description\": \"指定したい時間。単位は秒。\""
        "},"
        "\"action\":{"
          "\"type\": \"string\","
          "\"description\": \"指定したい動作。alarmは音で通知。shutdownは電源OFF。\","
          "\"enum\": [\"alarm\", \"shutdown\"]"
        "}"
      "},"
      "\"required\": [\"time\", \"action\"]"
    "}"
  "},"
  "{"
    "\"name\": \"timer_change\","
    "\"description\": \"実行中のタイマーの設定時間を変更する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"time\":{"
          "\"type\": \"integer\","
          "\"description\": \"変更後の時間。単位は秒。0の場合はタイマーをキャンセルする。\""
        "}"
      "},"
      "\"required\": [\"time\"]"
    "}"
  "},"
#if defined(ARDUINO_M5STACK_CORES3)
#if defined(ENABLE_WAKEWORD)
  "{"
    "\"name\": \"register_wakeword\","
    "\"description\": \"ウェイクワードを登録する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"wakeword_enable\","
    "\"description\": \"ウェイクワードを有効化する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"delete_wakeword\","
    "\"description\": \"ウェイクワードを削除する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"idx\":{"
          "\"type\": \"integer\","
          "\"description\": \"削除するウェイクワードの番号。\""
        "}"
      "},"
      "\"required\": [\"idx\"]"
    "}"
  "},"
#endif  //defined(ENABLE_WAKEWORD)
#endif  //defined(ARDUINO_M5STACK_CORES3)
  "{"
    "\"name\": \"get_date\","
    "\"description\": \"今日の日付を取得する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"get_time\","
    "\"description\": \"現在の時刻を取得する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"get_week\","
    "\"description\": \"今日が何曜日かを取得する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
#if !defined(USE_EXTENSION_FUNCTIONS)
  "}"
#else
  "},"
  "{"
    "\"name\": \"reminder\","
    "\"description\": \"指定した時間にリマインドする。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"hour\":{"
          "\"type\": \"integer\","
          "\"description\": \"hour (0-23)\""
        "},"
        "\"min\":{"
          "\"type\": \"integer\","
          "\"description\": \"minute\""
        "},"
        "\"text\":{"
          "\"type\": \"string\","
          "\"description\": \"リマインドする内容\""
        "}"
      "},"
      "\"required\": [\"hour\",\"min\",\"text\"]"
    "}"
  "},"
  "{"
    "\"name\": \"ask\","
    "\"description\": \"依頼を実行するのに必要な情報を会話の相手に質問する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"text\":{"
          "\"type\": \"string\","
          "\"description\": \"質問の内容。\""
        "}"
      "}"
    "}"
  "},"
  "{"
    "\"name\": \"save_note\","
    "\"description\": \"メモを保存する。買い物リストやTODOなど。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"text\":{"
          "\"type\": \"string\","
          "\"description\": \"メモの内容\""
        "}"
      "},"
      "\"required\": [\"text\"]"
    "}"
  "},"
  "{"
    "\"name\": \"read_note\","
    "\"description\": \"保存されているメモを読み上げる。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"delete_note\","
    "\"description\": \"保存されているメモを消去する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"get_weather\","
    "\"description\": \"指定した都市の現在の天気情報を取得する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"city\":{"
          "\"type\": \"string\","
          "\"description\": \"都市名（英語）。例: Tokyo, Osaka, New York\""
        "}"
      "},"
      "\"required\": [\"city\"]"
    "}"
  "},"
  "{"
    "\"name\": \"schedule_add\","
    "\"description\": \"定期スケジュールを登録する。cron式（分 時 日 月 曜日）で指定。例: 毎朝9時='0 9 * * *', 毎時='0 * * * *', 平日8時='0 8 * * 1,2,3,4,5'\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"name\":{"
          "\"type\": \"string\","
          "\"description\": \"スケジュール名（英数字、アンダースコア）\""
        "},"
        "\"cron\":{"
          "\"type\": \"string\","
          "\"description\": \"cron式（5フィールド: 分 時 日 月 曜日）。*は全て、カンマで複数指定可。曜日: 0=日,1=月,...,6=土\""
        "},"
        "\"action\":{"
          "\"type\": \"string\","
          "\"description\": \"実行時にAIに送る指示テキスト\""
        "}"
      "},"
      "\"required\": [\"name\",\"cron\",\"action\"]"
    "}"
  "},"
  "{"
    "\"name\": \"schedule_list\","
    "\"description\": \"登録されている定期スケジュールの一覧を取得する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"schedule_delete\","
    "\"description\": \"指定した名前の定期スケジュールを削除する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"name\":{"
          "\"type\": \"string\","
          "\"description\": \"削除するスケジュール名\""
        "}"
      "},"
      "\"required\": [\"name\"]"
    "}"
  "}"
#endif //if defined(USE_EXTENSION_FUNCTIONS)
"]";


void alarmTimerCallback(TimerHandle_t _xTimer){
  xAlarmTimer = NULL;
  //Serial.println("時間になりました。");
  alarmTimerCallbacked = true;
}

void powerOffTimerCallback(TimerHandle_t _xTimer){
  xAlarmTimer = NULL;
  //Serial.println("おやすみなさい。");
  avatar.setSpeechText("おやすみなさい。");
  delay(2000);
  avatar.setSpeechText("");
  M5.Power.powerOff();
}


FunctionCall::FunctionCall(llm_param_t param, LLMBase* llm, MCPClient** mcpClient)
  : _param(param),
    _llm(llm),
    _mcpClient(mcpClient)
{

}

// Function Call関連の初期化
void FunctionCall::init_func_call_settings(StackchanExConfig& system_config)
{

}


String FunctionCall::exec_calledFunc(const char* name, const char* args){
  String response = "";

  Serial.println(name);
  Serial.println(args);

  DynamicJsonDocument argsDoc(256);
  DeserializationError error = deserializeJson(argsDoc, args);
  if (error) {
    Serial.print(F("deserializeJson(arguments) failed: "));
    Serial.println(error.f_str());
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("エラーです");
    response = "エラーです";
    delay(1000);
    avatar.setSpeechText("");
    avatar.setExpression(Expression::Neutral);
  }else{

    //関数名がいずれかのMCPサーバに属するかを検索し、ヒットしたらリクエストを送信する
    for(int s=0; s<_param.llm_conf.nMcpServers; s++){
      if(_mcpClient[s]->search_tool(String(name))){
        DynamicJsonDocument tool_params(512);
        tool_params["name"] = String(name);
        tool_params["arguments"] = argsDoc;
        response = _mcpClient[s]->mcp_call_tool(tool_params);
        goto END;
      }
    }

    if(strcmp(name, "update_memory") == 0){
      const char* memory = argsDoc["memory"];
      Serial.println(memory);
      response = fn_update_memory(_llm, memory);
    }
    else if(strcmp(name, "timer") == 0){
      const int time = argsDoc["time"];
      const char* action = argsDoc["action"];
      Serial.printf("time:%d\n",time);
      Serial.println(action);
      response = timer(time, action);
    }
    else if(strcmp(name, "timer_change") == 0){
      const int time = argsDoc["time"];
      response = timer_change(time);    
    }
#if defined(ARDUINO_M5STACK_CORES3)
#if defined(ENABLE_WAKEWORD)
    else if(strcmp(name, "register_wakeword") == 0){
      response = register_wakeword();    
    }
    else if(strcmp(name, "wakeword_enable") == 0){
      response = wakeword_enable();    
    }
    else if(strcmp(name, "delete_wakeword") == 0){
      const int idx = argsDoc["idx"];
      Serial.printf("idx:%d\n",idx);   
      response = delete_wakeword(idx);    
    }
#endif  //defined(ENABLE_WAKEWORD)
#endif  //defined(ARDUINO_M5STACK_CORES3)
    else if(strcmp(name, "get_date") == 0){
      response = get_date();    
    }
    else if(strcmp(name, "get_time") == 0){
      response = get_time();    
    }
    else if(strcmp(name, "get_week") == 0){
      response = get_week();    
    }
#if defined(USE_EXTENSION_FUNCTIONS)
    else if(strcmp(name, "reminder") == 0){
      const int hour = argsDoc["hour"];
      const int min = argsDoc["min"];
      const char* text = argsDoc["text"];
      response = reminder(hour, min, text);
    }
    else if(strcmp(name, "ask") == 0){
      const char* text = argsDoc["text"];
      Serial.println(text);
      response = ask(text);
    }
    else if(strcmp(name, "save_note") == 0){
      const char* text = argsDoc["text"];
      response = save_note(text);
    }
    else if(strcmp(name, "read_note") == 0){
      response = read_note();
    }
    else if(strcmp(name, "delete_note") == 0){
      response = delete_note();
    }
    else if(strcmp(name, "get_weather") == 0){
      const char* city = argsDoc["city"];
      response = fn_get_weather(city);
    }
    else if(strcmp(name, "schedule_add") == 0){
      const char* sname = argsDoc["name"];
      const char* cron = argsDoc["cron"];
      const char* saction = argsDoc["action"];
      response = fn_schedule_add(sname, cron, saction);
    }
    else if(strcmp(name, "schedule_list") == 0){
      response = fn_schedule_list();
    }
    else if(strcmp(name, "schedule_delete") == 0){
      const char* sname = argsDoc["name"];
      response = fn_schedule_delete(sname);
    }
#endif  //if defined(USE_EXTENSION_FUNCTIONS)

  }

END:
  return response;
}


String FunctionCall::fn_update_memory(LLMBase* llm, const char* memory){
  String response = "";
  if(llm->enableMemory()){
    // Save to SPIFFS (existing behavior)
    if(llm->save_userInfo(memory)){
      response = "Memory update successful.";
    }else{
      response = "Memory update failure.";
    }

    // Also append to SD card MEMORY.md with timestamp
    struct tm timeInfo;
    String entry;
    if (getLocalTime(&timeInfo)) {
      char dateBuf[20];
      snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d %02d:%02d",
               timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
               timeInfo.tm_hour, timeInfo.tm_min);
      entry = String("- [") + dateBuf + "] " + String(memory);
    } else {
      entry = String("- ") + String(memory);
    }
    llm->appendMemory(entry);
  }
  else{
    response = "Memory is disabled.";
  }

  Serial.println(response);
  return response;
}


String FunctionCall::timer(int32_t time, const char* action){
  String response = "";

  if(xAlarmTimer != NULL){
    response = "別のタイマーを実行中です。";
  }
  else{
    if(strcmp(action, "alarm") == 0){
      xAlarmTimer = xTimerCreate("Timer", time * 1000, pdFALSE, 0, alarmTimerCallback);
      if(xAlarmTimer != NULL){
        xTimerStart(xAlarmTimer, 0);
        response = String("アラーム設定成功。") ;
      }
      else{
        response = "タイマーの設定が失敗しました。";
      }
    }
    else if(strcmp(action, "shutdown") == 0){
      xAlarmTimer = xTimerCreate("Timer", time * 1000, pdFALSE, 0, powerOffTimerCallback);
      if(xAlarmTimer != NULL){
        xTimerStart(xAlarmTimer, 0);
        response = String(time) + "秒後にパワーオフします。";
      }
      else{
        response = "タイマー設定が失敗しました。";
      }
    }
  }

  Serial.println(response);
  return response;
}

String FunctionCall::timer_change(int32_t time){
  String response = "";
  if(time == 0){
    xTimerDelete(xAlarmTimer, 0);
    xAlarmTimer = NULL;
    response = "タイマーをキャンセルしました。";
    alarmTimerCanceled = true;
  }
  else{
    xTimerChangePeriod(xAlarmTimer, time * 1000, 0);
    response = "タイマーの設定時間を" + String(time) + "秒に変更しました。";
  }

  return response;
}


String FunctionCall::get_date(){
  String response = "";
  struct tm timeInfo; 

  if (getLocalTime(&timeInfo)) {                            // timeinfoに現在時刻を格納
    response = String(timeInfo.tm_year + 1900) + "年"
               + String(timeInfo.tm_mon + 1) + "月"
               + String(timeInfo.tm_mday) + "日";
  }
  else{
    response = "時刻取得に失敗しました。";
  }
  return response;
}

String FunctionCall::get_time(){
  String response = "";
  struct tm timeInfo; 

  if (getLocalTime(&timeInfo)) {                            // timeinfoに現在時刻を格納
    response = String(timeInfo.tm_hour) + "時" + String(timeInfo.tm_min) + "分";
  }
  else{
    response = "時刻取得に失敗しました。";
  }
  return response;
}


String FunctionCall::get_week(){
  String response = "";
  struct tm timeInfo; 
  const char week[][8] = {"日", "月", "火", "水", "木", "金", "土"};

  if (getLocalTime(&timeInfo)) {                            // timeinfoに現在時刻を格納
    response = String(week[timeInfo.tm_wday]) + "曜日";
  }
  else{
    response = "時刻取得に失敗しました。";
  }
  return response;
}

#if defined(ARDUINO_M5STACK_CORES3)
#if defined(ENABLE_WAKEWORD)
bool register_wakeword_required = false;
String FunctionCall::register_wakeword(void){
  String response = "ウェイクワードを登録します。合図の後にウェイクワードを発声してください。";
  register_wakeword_required = true;
  return response;
}

bool wakeword_enable_required = false;
String FunctionCall::wakeword_enable(void){
  String response = "ウェイクワードを有効化しました。";
  wakeword_enable_required = true;
  return response;
}

String FunctionCall::delete_wakeword(int idx){
  SPIFFS.begin(true);
  String filename = filename_base + String(idx) + String(".bin");
  if (SPIFFS.exists(filename.c_str()))
  {
    SPIFFS.remove(filename.c_str());
    delete_mfcc(idx);
  }
  String response = String("ウェイクワード#") + String(idx) + String("を削除しました。");
  return response;
}
#endif  //defined(ENABLE_WAKEWORD)
#endif  //defined(ARDUINO_M5STACK_CORES3)


#if defined(USE_EXTENSION_FUNCTIONS)

String FunctionCall::reminder(int hour, int min, const char* text){
  String response = "";
  int ret;
  
  Serial.println("reminder");
  Serial.printf("%d:%d\n", hour, min);
  Serial.println(text);

  add_schedule(new ScheduleReminder(hour, min, String(text)));
  
  //response = String("Reminder setting successful");
  response = String(String("リマインドの設定成功。")
                    + String(hour) + ":" + String(min) + " "
                    + String(text));
  
  avatarText = String(hour) + ":" + String(min) + String("に設定しました");
  avatar.setSpeechText(avatarText.c_str());
  delay(2000);
  return response;
}


String FunctionCall::ask(const char* text){

  bool prev_servo_home = servo_home;
//#ifdef USE_SERVO
  servo_home = true;
//#endif
  avatar.setExpression(Expression::Happy);
  robot->speech(String(text));
  sw_tone();
  avatar.setSpeechText("どうぞ話してください");
  String ret = robot->listen();
//#ifdef USE_SERVO
  servo_home = prev_servo_home;
//#endif
  Serial.println("音声認識終了");
  if(ret != "") {
    Serial.println(ret);

  } else {
    Serial.println("音声認識失敗");
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("聞き取れませんでした");
    delay(2000);

  } 

  avatar.setSpeechText("");
  avatar.setExpression(Expression::Neutral);
  //M5.Speaker.begin();

  return ret;
}


String FunctionCall::save_note(const char* text){
  String response = "";
  String filename = String(APP_DATA_PATH) + String(FNAME_NOTEPAD);
  
  if (SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    auto fs = SD.open(filename.c_str(), FILE_WRITE, true);

    if(fs) {
      if(note == ""){
        note = String(text);
      }
      else{
        note = note + "\n" + String(text);
      }
      Serial.println(note);
      fs.write((uint8_t*)note.c_str(), note.length());
      fs.close();
      SD.end();
      //response = "Note saved successfully";
      response = String("メモの保存成功。メモの内容：" + String(text));
    }
    else{
      response = "メモの保存に失敗しました";
      SD.end();
    }

  }
  else{
    response = "メモの保存に失敗しました";
  }
  return response;
}

String FunctionCall::read_note(){
  String response = "";
  if(note == ""){
    response = "メモはありません";
  }
  else{
    response = "メモの内容は次の通り。" + note;
  }
  return response;
}

String FunctionCall::delete_note(){
  String response = "";
  String filename = String(APP_DATA_PATH) + String(FNAME_NOTEPAD);


  if (SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    auto fs = SD.open(filename.c_str(), FILE_WRITE, true);

    if(fs) {
      note = "";
      fs.write((uint8_t*)note.c_str(), note.length());
      fs.close();
      SD.end();
      response = "メモを消去しました";
    }
    else{
      response = "メモの消去に失敗しました";
      SD.end();
    }
  }
  else{
    response = "メモの消去に失敗しました";
  }
  return response;
}


String FunctionCall::get_bus_time(int nNext){
  String response = "";
  String filename = "";
  int now;
  int nNextCnt = 0;
  struct tm timeInfo;

  if (getLocalTime(&timeInfo)) {                            // timeinfoに現在時刻を格納
    Serial.printf("現在時刻 %02d:%02d  曜日 %d\n", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_wday);
    now = timeInfo.tm_hour * 60 + timeInfo.tm_min;
    
    switch(timeInfo.tm_wday){
      case 0:   //日
        filename = String(APP_DATA_PATH) + (FNAME_BUS_TIMETABLE_HOLIDAY);
        break;
      case 1:   //月
      case 2:   //火
      case 3:   //水
      case 4:   //木
      case 5:   //金
        filename = String(APP_DATA_PATH) + (FNAME_BUS_TIMETABLE);
        break;
      case 6:   //土
        filename = String(APP_DATA_PATH) + (FNAME_BUS_TIMETABLE_SAT);
        break;
    }

    if (SD.begin(GPIO_NUM_4, SPI, 25000000)) {
      auto fs = SD.open(filename.c_str(), FILE_READ);

      if(fs) {
        int hour, min;
        size_t max = 8;
        char buf[max];

        for(int line=0; line<200; line++){
          int len = fs.readBytesUntil(0x0a, (uint8_t*)buf, max);
          if(len == 0){
            Serial.printf("End of file. total line : %d\n", line);
            response = "最後の発車時刻を過ぎました。";
            break;
          }
          else{
            sscanf(buf, "%d:%d", &hour, &min);
            Serial.printf("%03d %02d:%02d\n", line, hour, min);

            int table = hour * 60 + min;
            if(now < table){
              if(nNextCnt == nNext){
                response = String(hour) + "時" + String(min) + "分";
                break;
              }
              else{
                nNextCnt ++;
              }
            }

          }
        }
        fs.close();

      } else {
        Serial.println("Failed to SD.open().");    
        response = "時刻表の読み取りに失敗しました。";  
      }

      SD.end();
    }
    else{
      response = "時刻表の読み取りに失敗しました。";
    }
  }
  else{
    response = "現在時刻の取得に失敗しました。";
  }

  return response;
}


String FunctionCall::fn_get_weather(const char* city){
  String response = "";

  if(g_weather_api_key == "" || g_weather_api_key == "null"){
    Serial.println("[Weather] API key not configured");
    return "天気APIキーが設定されていません。";
  }

  if(WiFi.status() != WL_CONNECTED){
    Serial.println("[Weather] WiFi not connected");
    return "WiFiに接続されていません。";
  }

  String url = "https://api.openweathermap.org/data/2.5/weather?q="
             + String(city)
             + "&units=metric&lang=ja&appid="
             + g_weather_api_key;

  Serial.printf("[Weather] Requesting weather for: %s\n", city);

  WiFiClientSecure client;
  client.setCACert(root_ca_openweathermap);
  client.setTimeout(10);  // 10 seconds

  HTTPClient http;
  if(!http.begin(client, url)){
    Serial.println("[Weather] HTTP begin failed");
    return "天気情報の取得に失敗しました。";
  }

  int httpCode = http.GET();
  Serial.printf("[Weather] HTTP response code: %d\n", httpCode);

  if(httpCode != HTTP_CODE_OK){
    http.end();
    Serial.printf("[Weather] HTTP error: %d\n", httpCode);
    return "天気情報の取得に失敗しました(HTTP " + String(httpCode) + ")。";
  }

  String payload = http.getString();
  http.end();

  // Parse JSON response (~1-2KB, heap is fine)
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, payload);
  if(error){
    Serial.printf("[Weather] JSON parse error: %s\n", error.f_str());
    return "天気データの解析に失敗しました。";
  }

  // Extract weather info
  const char* description = doc["weather"][0]["description"];
  float temp = doc["main"]["temp"];
  int humidity = doc["main"]["humidity"];
  float wind_speed = doc["wind"]["speed"];
  const char* city_name = doc["name"];

  // Build concise response
  char buf[128];
  snprintf(buf, sizeof(buf), "%s: %s、%.1f℃、湿度%d%%、風速%.1fm/s",
           city_name ? city_name : city,
           description ? description : "不明",
           temp, humidity, wind_speed);

  response = String(buf);
  Serial.printf("[Weather] Result: %s\n", response.c_str());
  return response;
}


// TODO: send_mail requires EMailSender library and mail config — excluded for now
// See GitHub Issue #13 (Gmail連携) for future implementation
#if 0
String FunctionCall::send_mail(String msg) {
  String response = "";
  EMailSender::EMailMessage message;

  if (authMailAdr != "") {

    EMailSender emailSend(authMailAdr.c_str(), authAppPass.c_str());

    message.subject = "スタックチャンからの通知";
    message.message = msg;
    EMailSender::Response resp = emailSend.send(toMailAdr.c_str(), message);

    if(resp.status == true){
      response = "メール送信成功";
    }
    else{
      response = "メール送信失敗";
    }

  }
  else{
    response = "メールアカウント情報のエラー";
  }


  return response;
}
#endif

String FunctionCall::fn_schedule_add(const char* name, const char* cron, const char* action){
  String response = "";

  if (!name || !cron || !action) {
    response = "スケジュール登録に失敗しました（パラメータ不足）。";
    Serial.printf("[CRON] schedule_add: missing params\n");
    return response;
  }

  // Check if name already exists
  if (find_cron_schedule(name) != nullptr) {
    response = String("スケジュール '") + name + "' は既に存在します。";
    Serial.printf("[CRON] schedule_add: '%s' already exists\n", name);
    return response;
  }

  // Create and add schedule
  add_schedule(new ScheduleCron(name, cron, action, true));

  // Persist to SD
  saveCronSchedules();

  response = String("スケジュール '") + name + "' を登録しました（" + cron + "）。";
  Serial.printf("[CRON] schedule_add: '%s' cron='%s' action='%s'\n", name, cron, action);
  return response;
}


String FunctionCall::fn_schedule_list(){
  String response = "";
  int count = 0;

  for (int i = 0; i < MAX_SCHED_NUM; i++) {
    ScheduleBase* sched = get_schedule(i);
    if (sched != nullptr && sched->get_sched_type() == SCHED_CRON) {
      ScheduleCron* cron = (ScheduleCron*)sched;
      if (count > 0) response += "\n";
      response += cron->getName() + ": " + cron->getCronExpr()
                + " -> " + cron->getAction()
                + (cron->isEnabled() ? "" : " [無効]");
      count++;
    }
  }

  if (count == 0) {
    response = "登録されているスケジュールはありません。";
  } else {
    response = String(count) + "件のスケジュール:\n" + response;
  }

  Serial.printf("[CRON] schedule_list: %d entries\n", count);
  return response;
}


String FunctionCall::fn_schedule_delete(const char* name){
  String response = "";

  if (!name) {
    response = "スケジュール名を指定してください。";
    return response;
  }

  if (remove_schedule_by_name(name)) {
    // Persist to SD
    saveCronSchedules();
    response = String("スケジュール '") + name + "' を削除しました。";
    Serial.printf("[CRON] schedule_delete: '%s' removed\n", name);
  } else {
    response = String("スケジュール '") + name + "' が見つかりません。";
    Serial.printf("[CRON] schedule_delete: '%s' not found\n", name);
  }

  return response;
}


// TODO: read_mail requires mail receiver setup — excluded for now
// See GitHub Issue #13 (Gmail連携) for future implementation
#if 0
String FunctionCall::read_mail(void) {
  String response = "";

  if(recvMessages.size() > 0){
    response = String(recvMessages[0]);
    recvMessages.pop_front();
    prev_nMail = recvMessages.size();
  }
  else{
    response = "受信メールはありません。";
  }

  return response;
}
#endif


#endif  //if defined(USE_EXTENSION_FUNCTIONS)


