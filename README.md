# Argum

Fully-featured, powerful and simple to use C++ command line argument parser.

[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization)
[![License](https://img.shields.io/badge/license-BSD-brightgreen.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![Tests](https://github.com/gershnik/argum/actions/workflows/test.yml/badge.svg)](https://github.com/gershnik/argum/actions/workflows/test.yml)

<!-- TOC depthfrom:2 -->

- [Features and goals](#features-and-goals)
- [Examples](#examples)
    - [Using exceptions](#using-exceptions)
    - [Using expected values](#using-expected-values)
- [Integration](#integration)
    - [Single header](#single-header)
    - [Module](#module)
    - [CMake via FetchContent](#cmake-via-fetchcontent)
    - [CMake from download](#cmake-from-download)
    - [Building and installing on your system](#building-and-installing-on-your-system)
        - [Basic use](#basic-use)
        - [CMake package](#cmake-package)
        - [Via pkg-config](#via-pkg-config)
- [Configuration](#configuration)
    - [Error reporting mode](#error-reporting-mode)
    - [Customizing termination function](#customizing-termination-function)
- [FAQ](#faq)
    - [Why another command line library?](#why-another-command-line-library)
    - [Why options cannot have more than 1 argument? ArgParse allows that](#why-options-cannot-have-more-than-1-argument-argparse-allows-that)
    - [Why isn't it using [C++20 feature X]?](#why-isnt-it-using-c20-feature-x)

<!-- /TOC -->

[releases]: https://github.com/gershnik/argum/releases
[syntax]: https://github.com/gershnik/argum/wiki/Syntax-Description
[getsubopt]: https://www.gnu.org/software/libc/manual/html_node/Suboptions.html

## Features and goals

* Supports the commonly used Unix and Windows command line conventions (Posix, GNU extensions to it, Python's Argparse, Microsoft syntax for its better designed utilities etc.). It should be possible to process complicated command lines like the ones of `git` or `clang` using this library. See [Syntax Description][syntax] on wiki for details on accepted syntax and configurability.
* Sequential processing. The arguments are handled in the order they are specified so you can handle things differently based on order if you want to. 
* Adaptive processing. You can also modify parser definitions while processing - this way you can _adapt_ the argument handling on the fly rather than have to pre-configure everything at once. 
* Simple to use. 
  * This library does not attempt to shove all the arguments into a map like data structure and neither does it attempt to define increasingly convoluted ways to "bind" arguments to variables in your code and convert them to internal types. Instead you provide callback lambdas (or any other function objects, of course) that put the data where you need it and convert how you want it. Beyond toy examples this usually results in simpler code and less mental effort to write.
  * Simple and extensible way to do rule based syntax validation. There are no rigid "groups" with predefined mutual exclusion only.
* Configurability beyond just "globally replace `-` with `/`". You can specify what prefixes to use on short options, long options, which prefixes are equivalent, disable short or long options behaviors altogether, what separator to use for arguments instead of `=` and more.
  * The default syntax is the common Unix/GNU one 
  * Additional, pre-built configurations are available for GNU "long options only" and a couple of common Windows syntaxes.
  * You can define your own configurations from scratch or modify one of the above (e.g. add `+` as short option prefix in addition to `-`)
* Support color output
  * The default color scheme is shamelessly borrowed from Python 3.14 Argparse but you can define your own colors.
* Can handle response files. 
* Can operate using exceptions or _expected values_ (similar to `boost::outcome` or proposed `std::expected`)
  * Can be used with exceptions and RTTI completely disabled
* No dependencies beyond C++ standard library. Can be used via a single file header.
  * Requires C++20 or above. 
  * Does not use `iostreams` in any way.
* Equivalent support for `char` and `wchar_t`. 
* Allows for localization with user-supplied message translations.
* Modularity. Everything is not shoved into a one giant "parser" class. You can combine different parts of the library in different ways if you really need to build something unusual. Similarly, things like printing help messages expose functions that print various parts of the whole to allow you to build your own help while reusing the tedious bits.


## Examples

Two equivalent examples that demonstrate many of the most important features of the library can be found at:
* Using exceptions - [samples/demo.cpp](samples/demo.cpp)
* Using expected values - [samples/demo-expected.cpp](samples/demo-expected.cpp)

A very minimalistic example to show "what does it look like" is below:

```cpp
#include <argum.h>

#include <iostream>

using namespace Argum;
using namespace std;

int main(int argc, char * argv[]) {

    const char * progname = (argc ? argv[0] : "prog");

    int optionValue = 0;
    std::string positionalArg;

    Parser parser;
    parser.add(
        Option("--help", "-h"). 
        help("show this help message and exit"). 
        handler([&]() {  
            cout << parser.formatHelp(progname);
            exit(EXIT_SUCCESS);
        }));
    parser.add(
        Option("--option", "-o").
        argName("VALUE").
        help("an option with a value"). 
        handler([&](const string_view & value) {
            optionValue = parseIntegral<int>(value);
        }));
    parser.add(
        Positional("positional").
        help("positional argument"). 
        handler([&](const string_view & value) { 
            positionalArg = value;
        }));

    try {
        parser.parse(argc, argv);
    } catch (ParsingException & ex) {
        cerr << ex.message() << '\n';
        cerr << parser.formatUsage(progname) << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "option value: " << optionValue << '\n';
    std::cout << "positional argument: " << positionalArg << '\n';
}
```

More sample code can be found on the [Wiki](https://github.com/gershnik/argum/wiki).

## Integration

You can integrate Argum into your code in the following ways

### Single header 

Download [single-file/argum.h](single-file/argum.h) from this repo or [Releases][releases] page, 
drop it into your project and `#include` it. This is the simplest way.

### Module
  
If you have a compiler and build system that support modules you can try to use **experimental** 
module file. Download [single-file/argum-module.ixx](single-file/argum-module.ixx) from this repo 
or [Releases][releases] page and integrate it into you project. 

Note, that of the compilers I have access to, only MSVC 2026 currently supports modules to any usable extent 
and is capable of using that module file.

### CMake via FetchContent
  
With modern CMake you can easily integrate Argum as follows:
```cmake
include(FetchContent)
FetchContent_Declare(argum
        GIT_REPOSITORY git@github.com:gershnik/argum.git
        GIT_TAG <desired tag like v2.5>
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(argum)
...
target_link_libraries(mytarget
PRIVATE
  argum::argum
)
``` 

> ℹ&#xFE0F; _[What is FetchContent?](https://cmake.org/cmake/help/latest/module/FetchContent.html)_


### CMake from download

Alternatively you can clone this repository somewhere and do this:

```cmake
add_subdirectory(PATH_WHERE_YOU_DOWNALODED_IT_TO, argum)
...
target_link_libraries(mytarget
PRIVATE
  argum::argum
)
```

### Building and installing on your system

You can also build and install this library on your system using CMake.

1. Download or clone this repository into SOME_PATH
2. On command line:
```bash
cd SOME_PATH
cmake -S . -B build 
cmake --build build

#Optional
#cmake --build build --target run-test

#install to /usr/local
sudo cmake --install build
#or for a different prefix
#cmake --install build --prefix /usr
```

Once the library has been installed it can be used int the following ways:

#### Basic use 

Set the include directory to `<prefix>/include` where `<prefix>` is the install prefix from above.

#### CMake package

```cmake
find_package(argum)

target_link_libraries(mytarget
PRIVATE
  argum::argum
)
```

#### Via `pkg-config`

Add the output of `pkg-config --cflags argum` to your compiler flags.

Note that the default installation prefix `/usr/local` might not be in the list of places your
`pkg-config` looks into. If so you might need to do:
```bash
export PKG_CONFIG_PATH=/usr/local/share/pkgconfig
```
before running `pkg-config`


## Configuration

Whichever method you use in order to use Argum your compiler needs to be set to C++20 mode or 
above. Argum should compile cleanly even on a highest warnings level. 

If you don't use CMake, on MSVC you need to:
- have `_CRT_SECURE_NO_WARNINGS` macro defined to avoid its bogus "deprecation" warnings.
- compile with `/Zc:preprocessor` option to enable standard conformant preprocessor

### Error reporting mode

Argum can operate in 3 modes selected at compile time:

* Using exceptions (this is the default). In this mode parsing errors produce exceptions.
* Using expected values, with exceptions enabled. In this mode parsing errors are reported via `Expected<T>` return values. This class similar to proposed `std::expected` or `boost::outcome`. Trying to access a `result.value()` when it contains an error throws an equivalent exceptions. This mode is enabled via `ARGUM_USE_EXPECTED` macro.
* With exceptions disabled. In this mode expected values used as above but trying to access a `result.value()` when it contains an error calls `std::terminate`. This mode can be manually enabled via `ARGUM_NO_THROW` macro. On Clang, GCC and MSVC Argum automatically detects if exceptions are disabled during compilation and switches to this mode. 

Note that these modes only affect handling of _parsing_ errors. Logic errors such as passing incorrect parameters to configure parser always `assert` in debug and `std::terminate` in non-debug builds.

### Customizing termination function

By default, when being passed invalid arguments or when attempting to "throw" with exceptions disabled Argum calls `assert` in debug mode and 
`std::terminate` in release. You can customize this behavior. To do so define `ARGUM_CUSTOM_TERMINATE` for the compilation. If you do this,
you will need to provide your own implementation of `[[noreturn]] inline void terminateApplication(const char * message)`. 

For reference this is the default implementation
<details>
<summary>Code</summary>

```cpp
[[noreturn]] inline void Argum::terminateApplication(const char * message) { 
    fprintf(stderr, "%s\n", message); 
    fflush(stderr); 
    #ifndef NDEBUG
        assert(false);
    #endif
    std::terminate(); 
}
```
</details>

## FAQ

### Why another command line library?

There are quite a few command line parsing libraries for C++ out there including [Boost.Program_options](https://www.boost.org/doc/libs/1_78_0/doc/html/program_options.html), [Lyra](https://github.com/bfgroup/Lyra), [TCLAP](http://tclap.sourceforge.net) and many others. 
Unfortunately, beyond toy applications I found none of them simultaneously easy to integrate, easy to use and easy to tweak. Specifically:
* Such a library needs to be header-only (and ideally a single file) one that can be quickly used in any little command line project without setting up and building a library dependency. 
* Simple examples work well with all libraries, but trying to implement something in real life soon requires you to fight the library defaults, add ad-hoc validation code, figure out workarounds etc. etc. 
* Handle sequential parsing. It is not uncommon to have meaningful order of options and positional arguments and parsers that don't allow making decisions based on order are just standing in the way.
* Configurability. Most libraries do let you change prefixes but usually in a clumsy way that doesn't really work well on Windows or other situations where you need custom syntax.
* Modularity. Most often this is an issue if you want a different way to display help while not redoing everything from scratch. Most libraries hide everything inside and only expose high level "print help" method with some custom header and footer. Also nobody exposes internal lexer/tokenizer which would allow different parsing semantics.

Some libraries do better on some of these but none I could find do everything well. Hence this project.

### Why options cannot have more than 1 argument? ArgParse allows that

Having multiple options arguments is a very bad idea. Consider this. Normally with Posix/GNU approach when an option argument itself looks like an option you can always use some workaround syntax to disambiguate. For example if you have option `--foo` and `-f` and their **argument** `-x` you can say: `--foo=-x` and `-f-x` to avoid treating `-x` as an unknown option. With multiple arguments this becomes impossible. People using ArgParse occasionally hit this issue and are surprised. Argum follows standard Unix approach of having at most one argument per option.

If you really, really need more than one argument to an option consider requiring to pass them as comma or semicolon separated list. This is also a de-facto standard Unix approach. See for example [getsubopt].

### Why isn't it using [C++20/23/26 feature X]?

This is simply due to the fact that, currently, not all compilers and standard libraries have support for all standard C++ features. 
Notably Ranges and `std::format` support is lacking on many. Once standard features support improves this library will attempt 
to adjust when appropriate.

