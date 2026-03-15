#ifndef _LLM_BASE_H
#define _LLM_BASE_H

#include <Arduino.h>
#include "SpiRamJsonDocument.h"
#include "ChatHistory.h"
#include "StackchanExConfig.h"

extern SpiRamJsonDocument chat_doc;       //LLMに送信するプロンプト（フォーマットは使用するLLMに従う）
extern SpiRamJsonDocument systemPrompt;   //SPIFFSに保存するシステムプロンプト
extern ChatHistory chatHistory;

extern const String defaultRole;
extern const String systemRole_memory;
extern const String systemRole_noMemory;

#define SYSTEM_PROMPT_INDEX_USER_ROLE     (0)
#define SYSTEM_PROMPT_INDEX_SYSTEM_ROLE   (1)
#define SYSTEM_PROMPT_INDEX_USER_INFO     (2)

// SD card memory file paths
#define MEMORY_DIR        "/memory"
#define SOUL_FILE_PATH    "/memory/SOUL.md"
#define USER_FILE_PATH    "/memory/USER.md"
#define MEMORY_FILE_PATH  "/memory/MEMORY.md"
#define MEMORY_FILE_MAX_SIZE  (4096)


struct llm_param_t
{
  String api_key;
  llm_s llm_conf;
};


class LLMBase{
//protected:
public:   //本当はprotectedにしたいところだがコールバック関数にthisポインタを渡して使うためにpublicとした
  llm_param_t param;
  String InitBuffer;
  int promptMaxSize;

  SpiRamJsonDocument chat_doc;
  SpiRamJsonDocument systemPrompt;

protected:
  bool _enableMemory;
  bool save_system_prompt_to_spiffs();
  bool load_system_prompt_from_spiffs();

  // Memory files loaded from SD card
  String _soulContent;
  String _userContent;
  String _memoryContent;

public:
  bool isOfflineService;

  LLMBase(llm_param_t param, int _promptMaxSize);
  virtual void chat(String text, const char *base64_buf = NULL) = 0;
  virtual bool init_chat_doc(const char *data) { return false; };  //TODO: LLMBaseで実装してもよいかも
  virtual void load_role() {};
  bool save_userRole(String role);
  bool save_userInfo(String userInfo);
  String get_userRole();
  String get_userInfo();
  bool clear_userInfo();

  // Memory file operations
  void loadMemoryFiles();
  const String& getSoulContent() { return _soulContent; }
  const String& getUserContent() { return _userContent; }
  const String& getMemoryContent() { return _memoryContent; }
  bool appendMemory(const String& entry);

  String get_InitBuffer() { return InitBuffer; };
  SpiRamJsonDocument& get_chat_doc() { return chat_doc; };

  bool enableMemory() { return _enableMemory; };
  void enableMemory(bool isEnable) { _enableMemory = isEnable; };

  // for async TTS
  //
  std::deque<String> outputTextQueue;
  bool speaking;
  String getOutputText();
  int getOutputTextQueueSize();
  void setSpeaking(bool _speaking){ speaking = _speaking; };
  int search_delimiter(String& text);
};


#endif //_LLM_BASE_H