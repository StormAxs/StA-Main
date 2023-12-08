#ifndef ENGINE_SHARED_ASSERTION_LOGGER_H
#define ENGINE_SHARED_ASSERTION_LOGGER_H

#include <memory>

class ILogger;
class IStorageTW;

std::unique_ptr<ILogger> CreateAssertionLogger(IStorageTW *pStorage, const char *pGameName);

#endif
