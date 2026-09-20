#include "data_mutex.h"

// Left null at global-construction time -- created in setup() instead.
// (Calling FreeRTOS APIs from C++ global constructors, before the RTOS/heap
// is fully up, is exactly what caused the AudioTools crash earlier in this
// project; not repeating that mistake here.)
SemaphoreHandle_t g_dataMutex = nullptr;
