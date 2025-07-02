#include "config.h"

namespace version04
{
    Config::ConfigVarMap Config::s_datas;
    Config::RWMutexType Config::s_mutex;
}