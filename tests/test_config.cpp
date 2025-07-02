#include "config.h"
#include "controllogger.h"
#include "util.h"

version04::ConfigVar<int>::ptr g_int_value_config= version04::Config::Lookup<int>("system.port",(int)8080,"system port");
version04::ConfigVar<float>::ptr g_float_value_config= version04::Config::Lookup<float>("system.value",(float)3.14,"system value");

int main(void)
{
    version04::initLogs();
    ULOG_INFO_SRC("main","port is {}",g_int_value_config->getValue());
    ULOG_INFO_SRC("main","port is {}",g_int_value_config->toString());
    ULOG_INFO_SRC("main","port is {}",g_float_value_config->getValue());
    ULOG_INFO_SRC("main","port is {}",g_float_value_config->toString());


    return 0;
}