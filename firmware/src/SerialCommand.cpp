#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "SerialCommand.h"

static const char* TAG = "[SerialCmd]";

// Ensure SD card is mounted before operations
static bool ensureSD() {
  if (SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    return true;
  }
  Serial.printf("%s ERROR: Failed to mount SD card\n", TAG);
  return false;
}

// List files in a directory
static void cmdList(const char* path) {
  if (!ensureSD()) return;
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
  if (!ensureSD()) return;
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
  if (!ensureSD()) return;
  // Auto-create parent directory if needed
  String pathStr(path);
  int lastSlash = pathStr.lastIndexOf('/');
  if (lastSlash > 0) {
    String dir = pathStr.substring(0, lastSlash);
    if (!SD.exists(dir.c_str())) {
      SD.mkdir(dir.c_str());
    }
  }
  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.printf("ERROR: Cannot open file for writing: %s\n", path);
    return;
  }

  Serial.println("READY");
  Serial.setTimeout(5000);  // Increase timeout for long lines

  size_t totalBytes = 0;
  while (true) {
    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      // Remove only trailing \r, preserve leading spaces (YAML indentation)
      if (line.endsWith("\r")) {
        line.remove(line.length() - 1);
      }
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

  Serial.setTimeout(1000);  // Restore default timeout

  file.close();
  Serial.printf("OK: %d bytes written to %s\n", totalBytes, path);
}

// Delete a file
static void cmdDelete(const char* path) {
  if (!ensureSD()) return;
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
