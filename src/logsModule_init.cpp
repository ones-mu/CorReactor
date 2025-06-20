#include "util.h"
#include "controllogger.h"

namespace version04
{
    void initLogs() {
        ControlLogger::instance().initializeFromFile("config/config.json");
    }
    
} // namespace version04
