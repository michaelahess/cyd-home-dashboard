#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Guards every manager's cached data struct. All network fetches run on a
// background task (see main.cpp) so the main loop -- touch polling and
// display drawing -- never blocks on an HTTP call; this is what makes a
// slow fetch briefly unresponsive to touch shared safely between that task
// (writer) and the render loop (reader). One shared mutex is enough since
// the background task only ever touches one manager at a time anyway.
extern SemaphoreHandle_t g_dataMutex;

// RAII lock: take g_dataMutex for the scope of this object.
class DataLock {
public:
    DataLock() { xSemaphoreTake(g_dataMutex, portMAX_DELAY); }
    ~DataLock() { xSemaphoreGive(g_dataMutex); }
    DataLock(const DataLock &) = delete;
    DataLock &operator=(const DataLock &) = delete;
};
