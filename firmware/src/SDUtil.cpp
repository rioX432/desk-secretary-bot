
#include <Arduino.h>
#include <M5Unified.h>
#include <stdio.h>
#include <SD.h>
#include <SPIFFS.h>

int read_sd_file(const char* filename, char* buf, int buf_size)
{
  int ret;
  auto fs = SD.open(filename, FILE_READ);
  
  if(fs) {
    size_t sz = fs.size();
    if(sz >= buf_size){
      Serial.printf("SD file size (%d) exceeds buf size (%d).\n", sz, buf_size);
      ret = 0;
    }
    else{
      fs.read((uint8_t*)buf, sz);
      buf[sz] = 0;
      fs.close();
      ret = sz;
    }
  }
  else{
    Serial.println("Failed to open SD.");
    ret = 0;
  }
  return ret;
}


int read_line_from_buf(char* buf, char* out)
{
  static int n_total = 0;
  static char* buf_ = nullptr;
  int n, ret;

  if(buf != nullptr){
    buf_ = buf;
  }

  if(buf_ == nullptr){
    Serial.println("read_line_from_buf: buffer is nullptr.");
    return 0;
  }

  ret = sscanf(buf_, "%s%n", out, &n);
  buf_ += n;

  return ret;
}


// SDカードのファイルをSPIFFSにコピー
bool copySDFileToSPIFFS(const char *path, bool forced) {
  
  Serial.println("Copy SD File to SPIFFS.");

  if(!SPIFFS.begin(true)){
    Serial.println("Failed to mount SPIFFS.");
    return false;
  }

  if (SPIFFS.exists(path) && !forced) {
	  Serial.println("File already exists in SPIFFS.");
	  return true;
  }

  if (!SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    Serial.println("Failed to mount SD.");
    return false;
  }

  File fsrc = SD.open(path, FILE_READ);
  File fdst = SPIFFS.open(path, FILE_WRITE);
  if(!fsrc || !fdst) {
    Serial.println("Failed to open file.");
    return false;
  }

  uint8_t *buf = new uint8_t[4096];
  if (!buf) {
	  Serial.println("Failed to allocate buffer.");
	  return false;
  }

  size_t len, size, ret;
  size = len = fsrc.size();
  while (len) {
	  size_t s = len;
	  if (s > 4096){
		  s = 4096;
    }  

	  fsrc.read(buf, s);
	  if ((ret = fdst.write(buf, s)) < s) {
		  Serial.printf("write failed: %d - %d\n", ret, s);
		  return false;
	  }
	  len -= s;
	  Serial.printf("%d / %d\n", size - len, size);
  }
 
  delete[] buf;
  fsrc.close();
  fdst.close();

  if (!SPIFFS.exists(path)) {
	  Serial.println("no file in SPIFFS.");
	  return false;
  }
  fdst = SPIFFS.open(path);
  len = fdst.size();
  fdst.close();
  if (len != size) {
	 Serial.println("size not match.");
	 return false;
  }
  Serial.println("*** Done. ***\r\n");
  return true;
}


// Read a text file from SD card into a String.
// Max read size is capped at maxSize to prevent OOM.
// Returns true on success, false if file not found or read error.
bool readSDTextFile(const char* path, String& content, size_t maxSize) {
  content = "";

  if (!SD.exists(path)) {
    Serial.printf("[MEM] SD file not found: %s\n", path);
    return false;
  }

  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.printf("[MEM] Failed to open: %s\n", path);
    return false;
  }

  size_t sz = f.size();
  if (sz == 0) {
    f.close();
    Serial.printf("[MEM] File is empty: %s\n", path);
    return true;  // success but empty
  }

  if (sz > maxSize) {
    Serial.printf("[MEM] File too large (%d > %d), truncating: %s\n", sz, maxSize, path);
    sz = maxSize;
  }

  // Use PSRAM for read buffer
  char* buf = (char*)ps_malloc(sz + 1);
  if (!buf) {
    // Fallback to heap
    buf = (char*)malloc(sz + 1);
    if (!buf) {
      Serial.printf("[MEM] Failed to allocate buffer for: %s\n", path);
      f.close();
      return false;
    }
  }

  size_t bytesRead = f.read((uint8_t*)buf, sz);
  f.close();
  buf[bytesRead] = '\0';
  content = String(buf);
  free(buf);

  Serial.printf("[MEM] Read %d bytes from: %s\n", bytesRead, path);
  return true;
}


// Write (overwrite) a text file on SD card.
bool writeSDTextFile(const char* path, const String& content) {
  File f = SD.open(path, FILE_WRITE);
  if (!f) {
    Serial.printf("[MEM] Failed to open for write: %s\n", path);
    return false;
  }

  size_t written = f.print(content);
  f.close();

  Serial.printf("[MEM] Wrote %d bytes to: %s\n", written, path);
  return true;
}


// Append text to a file on SD card. Creates the file if it doesn't exist.
bool appendSDTextFile(const char* path, const String& content) {
  File f = SD.open(path, FILE_APPEND);
  if (!f) {
    // FILE_APPEND may fail if file doesn't exist on some platforms; try write
    f = SD.open(path, FILE_WRITE);
    if (!f) {
      Serial.printf("[MEM] Failed to open for append: %s\n", path);
      return false;
    }
  }

  size_t written = f.print(content);
  f.close();

  Serial.printf("[MEM] Appended %d bytes to: %s\n", written, path);
  return true;
}

