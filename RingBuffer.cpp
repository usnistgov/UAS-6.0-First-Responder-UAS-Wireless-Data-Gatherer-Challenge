#include "RingBuffer.h"

RingBuffer ringbuff;

void ringBufferInit(){
  int averageStringSize = httpDataPayload.length();			        // Approximate size of each JSON entry in bytes
  putToSerial("RingBuffer DataPayload Size: ");
  putToSerialWithNewline(String(averageStringSize));						// This info is to be used once (maunally), set and forget
  int maxEntries = ringbuff.estimateMaxEntries(averageStringSize);
  //putToSerialWithNewline("Allocating "+String(RING_BUFFER_SIZE)+" RingBuffer max entries.",2);   // totally number of entries Ringbuffer to be stored
  putToDebugWithNewline("Allocating 90 RingBuffer max entries.",2);   // totally number of entries Ringbuffer to be stored
  putToDebugWithNewline("ringBufferInit() Init completed",3);
}

RingBuffer::RingBuffer(void) {                                  // Ringbuffer Constructor 
  head = 0;
  tail = 0;
  size = 0;
}

void RingBuffer::push(const String& jsonString) {               // Method to add a JSON entry to the buffer and extract its timestamp
  DynamicJsonDocument doc(1024);               						      // Parse the JSON string to extract the timestamp
  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
      Serial.print("Failed to parse JSON in Ringbuffer: ");
      Serial.println(error.f_str());
      return;                                                   // Skip this entry if parsing fails
  }
  time_t unixTime = doc["rts"].as<time_t>();                    // Extract the timestamp from the JSON (assuming the field is named "Time")
  if (size < RING_BUFFER_SIZE) {                                // Store the JSON entry and timestamp in the buffer
      buffer[head].jsonData = jsonString;
      buffer[head].timestamp = unixTime;
      head = (head + 1) % RING_BUFFER_SIZE;
      size++;
  } else {
      buffer[head].jsonData = jsonString;
      buffer[head].timestamp = unixTime;
      head = (head + 1) % RING_BUFFER_SIZE;
      tail = (tail + 1) % RING_BUFFER_SIZE;                     // Move tail forward if buffer is full
  }
}

// Method to get the oldest JSON entry from the buffer
String RingBuffer::pop(void) {
  if (size == 0) {
      return "";
  } else {
      String jsonString = buffer[tail].jsonData;
      tail = (tail + 1) % RING_BUFFER_SIZE;
      size--;
      return jsonString;
  }
}

bool RingBuffer::isEmpty() {                                    // Method to check if the buffer is empty
  return size == 0;
}

bool RingBuffer::isFull() {                                     // Method to check if the buffer is full
  return size == RING_BUFFER_SIZE;
}

int RingBuffer::getCount() {                                    // Method to get the count of entries in the buffer
  return size;
}

int RingBuffer::getTotalSize() {                                // Method to get the total size (in bytes) of all entries in the buffer
  int totalSize = 0;
  for (int i = 0; i < size; i++) {
      int index = (tail + i) % RING_BUFFER_SIZE;
      totalSize += buffer[index].jsonData.length();
  }
  return totalSize;
}

time_t RingBuffer::getOldestTimestamp() {                       // Method to get the oldest timestamp in the buffer
  if (isEmpty()) {
      return 0;  // Return 0 if the buffer is empty
  }
  time_t oldestTimestamp = buffer[tail].timestamp;              // The oldest timestamp is always at the tail of the circular buffer
  return oldestTimestamp;
}

time_t RingBuffer::getNewestTimestamp() {                       // Method to get the newest timestamp in the buffer
  if (isEmpty()) {
      return 0;                                                 // Return 0 if the buffer is empty
  }
  int newestIndex = (head - 1 + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;     // The newest timestamp is just before the head of the circular buffer
  return buffer[newestIndex].timestamp;
}

int RingBuffer::estimateMaxEntries(int averageStringSize) {               // Method to estimate the max number of entries that can fit in the buffer
  int freeMemory = ESP.getFreeHeap();
  int maxEntries = freeMemory / averageStringSize;
  Serial.print("Estimated max number of entries possible in Ringbuffer: ");
  Serial.println(maxEntries);
  return maxEntries;
}

DynamicJsonDocument RingBuffer::pullEntriesByTime(time_t myUnixTime1, time_t myUnixTime2) { // Method to pull data based ont time, format, and send via HTTP-POST

  JsonObject data = httpDataPayloadDoc.createNestedObject("data");
  data["dlen"] = 0;
  data["ftso"] = 0;
  data["fox"] = thisFoxNodeId;
  JsonArray dumpData = data.createNestedArray("dump");

  int entriesFound = 0;
  time_t baseTimestamp = 0;

  for (int i = 0; i < size; i++) {
    int index = (tail + i) % RING_BUFFER_SIZE;
    time_t entryUnixTime = buffer[index].timestamp;

    // Debugging print for each entry and comparison
    // DEBUGGING clutter... // Serial.print("Checking RingBuffer timestamp: ");
    //Serial.println(entryUnixTime);

    if (entryUnixTime >= myUnixTime1 && entryUnixTime <= myUnixTime2) {
      if (baseTimestamp == 0 || entryUnixTime < baseTimestamp) {
        baseTimestamp = entryUnixTime;
      }

      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, buffer[index].jsonData);
      if (error) {
        Serial.print("Error parsing JSON for entry: ");
        Serial.println(error.f_str());
        continue;
      }

      JsonObject reading = doc.as<JsonObject>();
      JsonObject readingObject = dumpData.createNestedObject();
      readingObject["rts"] = reading["rts"];

      if (reading.containsKey("rssi")) readingObject["rssi"] = reading["rssi"];
      if (reading.containsKey("ip")) readingObject["ip"] = reading["ip"];
      if (reading.containsKey("t")) readingObject["t"] = reading["t"];
      if (reading.containsKey("c")) readingObject["c"] = reading["c"];
      if (reading.containsKey("h")) readingObject["h"] = reading["h"];
      if (reading.containsKey("l")) readingObject["l"] = reading["l"];
      if (reading.containsKey("p")) readingObject["p"] = reading["p"];
      if (reading.containsKey("x")) readingObject["x"] = reading["x"];
      if (reading.containsKey("y")) readingObject["y"] = reading["y"];
      if (reading.containsKey("z")) readingObject["z"] = reading["z"];

      entriesFound++;
    }
  }

  data["dlen"] = entriesFound;
  data["ftso"] = baseTimestamp;
  data["fox"] = thisFoxNodeId;

  return httpDataPayloadDoc;
}

