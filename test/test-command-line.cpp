#include <modern-arg-parser/command-line.h>

#include "catch.hpp"

using namespace MArgP;
using namespace std;

using namespace std::literals;

#ifndef ANDROID
    filesystem::path myPath = __FILE__;
    if (myPath.is_relative()) {
        myPath = filesystem::current_path() / myPath;
    }
    myPath = filesystem::canonical(myPath);
    auto myDir = myPath.parent_path();
#else
    auto myDir = filesystem::current_path() / "data";
#endif

TEST_CASE( "Narrow response file" , "[command-line]") {

    auto respFile = myDir / "response.txt";
    auto respArg = "@" + respFile.string();

    const char * argv[] = {"prog", "first", respArg.c_str(), "last"};
    const auto args = makeArgSpan(int(std::size(argv)), (char**)argv);
    CHECK_THROWS_WITH(ResponseFileReader({"@"}).expand(args), "error reading response file \"response1.txt\": "s + std::make_error_code(errc::no_such_file_or_directory).message());
    
    auto expanded = ResponseFileReader('@').expand(args, [&](string && line, auto out) {
        trimInPlace(line);
        if (line.empty())
            return;
        if (line[0] == '@') {
            *out = "@" + filesystem::canonical(myDir / line.substr(1)).string();
        } else {
            *out = std::move(line);
        }
    });
    CHECK(expanded == vector<string>{"first", "foo", "bar", "hello", "world", "baz", "last"});
}

TEST_CASE( "Wide response file" , "[command-line]") {



    auto respFile = myDir / "response.txt";
    auto respArg = L"@" + respFile.wstring();

    const wchar_t * argv[] = {L"prog", L"first", respArg.c_str(), L"last"};
    const auto args = makeArgSpan(int(std::size(argv)), (wchar_t**)argv);
    CHECK_THROWS_WITH(WResponseFileReader({L"@"}).expand(args), "error reading response file \"response1.txt\": "s +  std::make_error_code(errc::no_such_file_or_directory).message());
    
    auto expanded = WResponseFileReader(L'@').expand(args, [&](wstring && line, auto out) {
        trimInPlace(line);
        if (line.empty())
            return;
        if (line[0] == L'@') {
            *out = L"@" + filesystem::canonical(myDir / line.substr(1)).wstring();
        } else {
            *out = std::move(line);
        }
    });
    CHECK(expanded == vector<wstring>{L"first", L"foo", L"bar", L"hello", L"world", L"baz", L"last"});
}