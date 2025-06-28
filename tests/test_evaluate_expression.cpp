
#include "nlohmann/json.hpp"
#include <vector>
#include <string>
#include "util.h"
#include "controllogger.h"
#include "read_config.h"

int main(void)
{
    version04::initLogs();
    nlohmann::json config_json_base = version04::get_config();

    // auto fiber_json=config_json_base.value("fiber","abc");
    // ULOG_INFO_SRC("main","test evaluate expreesion fiber_json: {}",fiber_json);
    std::string expr = config_json_base["fiber"][0]["stack_size"];
    ULOG_INFO_SRC("main","test evaluate expreesion expr: {}",expr);

    int result=version04::evaluate_expression(expr);
    ULOG_INFO_SRC("main","test evaluate expreesion result: {}",result);
    return 0;
}

