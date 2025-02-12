#include "test-common.h"

#include <argum/command-line.h>

#include <doctest/doctest.h>

using namespace Argum;
using namespace std;

using namespace std::literals;

#ifndef ANDROID
    auto myDir = []() {
        filesystem::path myPath = __FILE__;
        if (myPath.is_relative()) {
            myPath = filesystem::current_path() / myPath;
        }
        return filesystem::canonical(myPath).parent_path();
    }();
#else
    auto myDir = filesystem::current_path() / "data";
#endif

TEST_SUITE("command-line") {

TEST_CASE( "Narrow response file" ) {

    auto respFile = myDir / "response.txt";
    auto respArg = "@" + respFile.string();

    const char * argv[] = {"prog", "first", respArg.c_str(), "last"};
    const auto args = makeArgSpan(int(std::size(argv)), (char**)argv);
    #ifndef ARGUM_NO_THROW
        CHECK_THROWS_WITH(ARGUM_EXPECTED_VALUE(ResponseFileReader({"@"}).expand(args)), 
                        ("error reading response file \"response1.txt\": "s + std::make_error_code(errc::no_such_file_or_directory).message()).c_str());
    #else
        auto err = ResponseFileReader({"@"}).expand(args).error();
        CHECK(err);
        if (!err) abort();
        CHECK(err->message() == 
              "error reading response file \"response1.txt\": "s + std::make_error_code(errc::no_such_file_or_directory).message());
    #endif
    
    auto expanded = ARGUM_EXPECTED_VALUE(ResponseFileReader('@').expand(args, [&](string && line, auto out) {
        trimInPlace(line);
        if (line.empty())
            return;
        if (line[0] == '@') {
            *out = "@" + filesystem::canonical(myDir / line.substr(1)).string();
        } else {
            *out = std::move(line);
        }
    }));
    CHECK(expanded == vector<string>{"first", "foo", "bar", "hello", "world", "baz", "last"});
}

TEST_CASE( "Wide response file" ) {



    auto respFile = myDir / "response.txt";
    auto respArg = L"@" + respFile.wstring();

    const wchar_t * argv[] = {L"prog", L"first", respArg.c_str(), L"last"};
    const auto args = makeArgSpan(int(std::size(argv)), (wchar_t**)argv);

    #ifndef ARGUM_NO_THROW
        CHECK_THROWS_WITH(ARGUM_EXPECTED_VALUE(WResponseFileReader({L"@"}).expand(args)), 
                        ("error reading response file \"response1.txt\": "s +  std::make_error_code(errc::no_such_file_or_directory).message()).c_str());
    #else
        auto err = WResponseFileReader({L"@"}).expand(args).error();
        CHECK(err);
        if (!err) abort();
        CHECK(toString<char>(err->message()) == 
              "error reading response file \"response1.txt\": "s +  std::make_error_code(errc::no_such_file_or_directory).message());
    #endif
    
    auto expanded = ARGUM_EXPECTED_VALUE(WResponseFileReader(L'@').expand(args, [&](wstring && line, auto out) {
        trimInPlace(line);
        if (line.empty())
            return;
        if (line[0] == L'@') {
            *out = L"@" + filesystem::canonical(myDir / line.substr(1)).wstring();
        } else {
            *out = std::move(line);
        }
    }));
    CHECK(expanded == vector<wstring>{L"first", L"foo", L"bar", L"hello", L"world", L"baz", L"last"});
}

TEST_CASE( "Trim in place" ) {

    std::string str = "help help";
    CHECK(trimInPlace(str) == "help help");
}

}
