#include "http/http_parser.h"
#include "controllogger.h"
#include "util.h"
const char test_request_data[] = "POST / HTTP/1.1\r\n"
                                "Host: www.sylar.top\r\n"
                                "Content-Length: 10\r\n\r\n"
                                "1234567890";

void test_request() {
    version04::http::HttpRequestParser parser;
    std::string tmp = test_request_data;
    size_t s = parser.execute(&tmp[0], tmp.size());
    ULOG_INFO_SRC("main","execute rt={} has_error={} is_finished={} total={} content_length={} tmp[s]={}",s,parser.hasError(),parser.isFinished(),tmp.size(),parser.getContentLength(),tmp[s]);
    tmp.resize(tmp.size() - s);
    ULOG_INFO_SRC("main","{}",parser.getData()->toString());
    ULOG_INFO_SRC("main","{}",tmp);
}

const char test_response_data[] = "HTTP/1.1 200 OK\r\n"
        "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
        "Server: Apache\r\n"
        "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
        "ETag: \"51-47cf7e6ee8400\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 81\r\n"
        "Cache-Control: max-age=86400\r\n"
        "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
        "Connection: Close\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html>\r\n"
        "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
        "</html>\r\n";

void test_response() {
    version04::http::HttpResponseParser parser;
    std::string tmp = test_response_data;
    size_t s = parser.execute(&tmp[0], tmp.size(), true);
    ULOG_INFO_SRC("main","execute rt={} has_error={} is_finished={} total={} content_length={} tmp[s]={}",s,parser.hasError(),parser.isFinished(),tmp.size(),parser.getContentLength(),tmp[s]);
    tmp.resize(tmp.size() - s);
    ULOG_INFO_SRC("main","{}",parser.getData()->toString());
    ULOG_INFO_SRC("main","{}",tmp);

}

int main(int argc, char** argv) {
    version04::initLogs();
    test_request();
    ULOG_INFO_SRC("main","-------------------");
    test_response();
    return 0;
}
