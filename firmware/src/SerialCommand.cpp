#include <Arduino.h>
#include <SD.h>
#include "SerialCommand.h"

static const char* TAG = "[SerialCmd]";

// List files in a directory
static void cmdList(const char* path) {
  File dir = SD.open(path);
  if (!dir) {
    Serial.printf("ERROR: Cannot open directory: %s\n", path);
    return;
  }
  if (!dir.isDirectory()) {
    Serial.printf("ERROR: Not a directory: %s\n", path);
    dir.close();
    return;
  }

  Serial.printf("--- DIR: %s ---\n", path);
  File entry = dir.openNextFile();
  while (entry) {
    if (entry.isDirectory()) {
      Serial.printf("  [DIR]  %s\n", entry.name());
    } else {
      Serial.printf("  %7d  %s\n", entry.size(), entry.name());
    }
    entry = dir.openNextFile();
  }
  Serial.println("--- END ---");
  dir.close();
}

// Read and print file contents
static void cmdRead(const char* path) {
  File file = SD.open(path, FILE_READ);
  if (!file) {
    Serial.printf("ERROR: Cannot open file: %s\n", path);
    return;
  }

  Serial.printf("--- FILE: %s (%d bytes) ---\n", path, file.size());
  while (file.available()) {
    Serial.write(file.read());
  }
  Serial.println("\n--- EOF ---");
  file.close();
}

// Write content received line-by-line until "END" marker
static void cmdWrite(const char* path) {
  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.printf("ERROR: Cannot open file for writing: %s\n", path);
    return;
  }

  Serial.println("READY");

  size_t totalBytes = 0;
  while (true) {
    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      line.trim();
      if (line == "END") {
        break;
      }
      if (totalBytes > 0) {
        file.print("\n");
        totalBytes++;
      }
      file.print(line);
      totalBytes += line.length();
    }
    delay(1);
  }

  file.close();
  Serial.printf("OK: %d bytes written to %s\n", totalBytes, path);
}

// Delete a file
static void cmdDelete(const char* path) {
  if (!SD.exists(path)) {
    Serial.printf("ERROR: File not found: %s\n", path);
    return;
  }
  if (SD.remove(path)) {
    Serial.printf("OK: Deleted %s\n", path);
  } else {
    Serial.printf("ERROR: Failed to delete %s\n", path);
  }
}

void handleSerialCommand() {
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() == 0) return;

  Serial.printf("%s Received: %s\n", TAG, input.c_str());

  // Parse command and argument
  int spaceIdx = input.indexOf(' ');
  String cmd, arg;
  if (spaceIdx > 0) {
    cmd = input.substring(0, spaceIdx);
    arg = input.substring(spaceIdx + 1);
    arg.trim();
  } else {
    cmd = input;
  }
  cmd.toUpperCase();

  if (cmd == "SD_LIST") {
    cmdList(arg.length() > 0 ? arg.c_str() : "/");
  } else if (cmd == "SD_READ") {
    if (arg.length() == 0) {
      Serial.println("ERROR: Usage: SD_READ <path>");
      return;
    }
    cmdRead(arg.c_str());
  } else if (cmd == "SD_WRITE") {
    if (arg.length() == 0) {
      Serial.println("ERROR: Usage: SD_WRITE <path>");
      return;
    }
    cmdWrite(arg.c_str());
  } else if (cmd == "SD_DELETE") {
    if (arg.length() == 0) {
      Serial.println("ERROR: Usage: SD_DELETE <path>");
      return;
    }
    cmdDelete(arg.c_str());
  } else if (cmd == "SD_HELP") {
    Serial.println("--- Serial SD Commands ---");
    Serial.println("  SD_LIST [path]    List directory contents (default: /)");
    Serial.println("  SD_READ <path>    Read file contents");
    Serial.println("  SD_WRITE <path>   Write file (send lines, then END)");
    Serial.println("  SD_DELETE <path>  Delete a file");
    Serial.println("  SD_HELP           Show this help");
    Serial.println("--------------------------");
  }
}
