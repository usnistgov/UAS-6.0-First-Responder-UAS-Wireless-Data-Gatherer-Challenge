#ifndef RINGBUFFER_H
#define RINGBUFFER_H

// SCOPE: Ringbuffer class def, for storing I2C sensor data 

// Includes
#include <Arduino.h>
#include <ArduinoJson.h>
#include "state.h"
#include "serial.h"
#include "sensor.h"                                                   // for httpDataPayloadDoc


// Classes
class RingBuffer {
public:
  //void init(String var);
  RingBuffer();                                                       // Constructor
  void push(const String& jsonString);                                // Push JSON entry (timestamp is extracted)
  String pop(void);                                                   // Pop JSON entry
  bool isEmpty();                                                     // Check if buffer is empty
  bool isFull();                                                      // Check if buffer is full
  int getCount();                                                     // Get the count of entries in the buffer
  int getTotalSize();                                                 // Get the total size of entries (in bytes)
  time_t getOldestTimestamp();                                        // Get the oldest timestamp in the buffer
  time_t getNewestTimestamp();                                        // Get the newest timestamp in the buffer
  int estimateMaxEntries(int averageStringSize);                      // Estimate max number of entries based on available memory
  DynamicJsonDocument pullEntriesByTime(time_t myUnixTime1, time_t myUnixTime2);   // Pull entries older than a specific time and send as HTTP POST (EndTime, StartTime)
   int dropEntriesUpTo(time_t unixTimeInclusive);                     // Remove (ACK) entries <= unixTimeInclusive

private:
  static const int RING_BUFFER_SIZE = 90;                             // Buffer size
  struct BufferEntry {
      String jsonData;                                                // JSON data
      time_t timestamp;                                               // UNIX timestamp
  };
  
  BufferEntry buffer[RING_BUFFER_SIZE];                               // Array of BufferEntry structures
  int head;                                                           // Index for adding entries
  int tail;                                                           // Index for removing entries
  int size;                                                           // Current number of entries in the buffer
};

// Global Variables
extern RingBuffer ringbuff;

// Function Declarations
void ringBufferInit();

#endif    // RINGBUFFER_H
