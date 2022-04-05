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
    - [CMake](#cmake)
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

Two equivalent examples - one using exceptions and one error codes - of file processing utility of some kind are given below. 
They demonstrates many of the most important features of the library. More examples can be found on the [Wiki](https://github.com/gershnik/argum/wiki).

### Using exceptions
<details>
<summary>Code</summary>

```cpp
#include "argum.h"
#include <iostream>

using namespace Argum;
using namespace std;

enum Encoding { defaultEncoding, Base64, Hex };

int main(int argc, char * argv[]) {

    vector<string> sources;
    string destination;
    optional<Encoding> encoding = defaultEncoding;
    optional<string> compression;
    int compressionLevel = 9;

    const char * progname = (argc ? argv[0] : "my_utility");

    Parser parser;
    parser.add(
        Positional("source").
        help("source file"). 
        occurs(zeroOrMoreTimes).
        handler([&](const string_view & value) { 
            sources.emplace_back(value);
    }));
    parser.add(
        Positional("destination").
        help("destination file"). 
        occurs(once). 
        handler([&](string_view value) { 
            destination = value;
    }));
    parser.add(
        Option("--help", "-h").
        help("show this help message and exit"). 
        handler([&]() {
            cout << parser.formatHelp(progname);
            exit(EXIT_SUCCESS);
    }));
    ChoiceParser encodingChoices;
    encodingChoices.addChoice("default");
    encodingChoices.addChoice("base64");
    encodingChoices.addChoice("hex");
    parser.add(
        Option("--format", "-f", "--encoding", "-e").
        help("output file format"). 
        argName(encodingChoices.description()).
        handler([&](string_view value) {
            encoding = Encoding(encodingChoices.parse(value));
    }));
    parser.add(
        Option("--compress", "-c").
        argName("ALGORITHM").
        requireAttachedArgument(true). //require -cALGORITHM or --compress=ALGORITHM syntax
        help("compress output with a given algorithm (default gzip)"). 
        handler([&](const optional<string_view> & value) {
            encoding = nullopt;
            compression = value.value_or("gzip");
    }));
    parser.add(
        Option("--level", "-l").
        argName("LEVEL").
        help("compression level, requires --compress"). 
        handler([&](const string_view & value) {
            compressionLevel = parseIntegral<int>(value);
    }));
    parser.addValidator(
        oneOrNoneOf(
            optionPresent("--format"),
            anyOf(optionPresent("--compress"), optionPresent("--level"))
        ), 
        "options --format and --compress/--level are mutually exclusive"
    );
    parser.addValidator(
        !optionPresent("--level") || optionPresent("--compress"), 
        "if --level is specified then --compress must be specified also"
    );

    try {
        parser.parse(argc, argv);
    } catch (ParsingException & ex) {
        cerr << ex.message() << '\n';
        cerr << parser.formatUsage(progname) << '\n';
        return EXIT_FAILURE;
    }

    if (encoding)
        cout << "need to encode with encoding: " << *encoding <<'\n';
    else 
        cout << "need to compress with algorithm: " << *compression 
             << " at level: " << compressionLevel <<'\n';
    cout << "sources: {" << join(sources.begin(), sources.end(), ", ") << "}\n";
    cout << "into: " << destination <<'\n';
}
```
</details>

### Using expected values
<details>
<summary>Code</summary>

```cpp
#include "argum.h"
#include <iostream>

using namespace Argum;
using namespace std;

enum Encoding { defaultEncoding, Base64, Hex };

int main(int argc, char * argv[]) {

    vector<string> sources;
    string destination;
    optional<Encoding> encoding = defaultEncoding;
    optional<string> compression;
    int compressionLevel = 9;

    const char * progname = (argc ? argv[0] : "my_utility");

    Parser parser;
    parser.add(
        Positional("source").
        help("source file"). 
        occurs(zeroOrMoreTimes).
        handler([&](const string_view & value) { 
            sources.emplace_back(value);
    }));
    parser.add(
        Positional("destination").
        help("destination file"). 
        occurs(once). 
        handler([&](string_view value) { 
            destination = value;
    }));
    parser.add(
        Option("--help", "-h").
        help("show this help message and exit"). 
        handler([&]() {
            cout << parser.formatHelp(progname);
            exit(EXIT_SUCCESS);
    }));
    ChoiceParser encodingChoices;
    encodingChoices.addChoice("default");
    encodingChoices.addChoice("base64");
    encodingChoices.addChoice("hex");
    parser.add(
        Option("--format", "-f", "--encoding", "-e").
        help("output file format"). 
        argName(encodingChoices.description()).
        handler([&](string_view value) -> Expected<void> {
            auto result = encodingChoices.parse(value);
            if (auto error = result.error()) return error;
            encoding = Encoding(*result);
            return {};
    }));
    parser.add(
        Option("--compress", "-c").
        argName("ALGORITHM").
        requireAttachedArgument(true). //require -cALGORITHM or --compress=ALGORITHM syntax
        help("compress output with a given algorithm (default gzip)"). 
        handler([&](const optional<string_view> & value) {
            encoding = nullopt;
            compression = value.value_or("gzip");
    }));
    parser.add(
        Option("--level", "-l").
        argName("LEVEL").
        help("compression level, requires --compress"). 
        handler([&](const string_view & value) -> Expected<void> {
            auto result = parseIntegral<int>(value);
            if (auto error = result.error()) return error;
            compressionLevel = *result;
            return {};
    }));
    parser.addValidator(
        oneOrNoneOf(
            optionPresent("--format"),
            anyOf(optionPresent("--compress"), optionPresent("--level"))
        ), 
        "options --format and --compress/--level are mutually exclusive"
    );
    parser.addValidator(
        !optionPresent("--level") || optionPresent("--compress"), 
        "if --level is specified then --compress must be specified also"
    );

    auto result = parser.parse(argc, argv);
    if (auto error = result.error()) {
        cerr << error->message() << '\n';
        cerr << parser.formatUsage(progname) << '\n';
        return EXIT_FAILURE;
    }

    if (encoding)
        cout << "need to encode with encoding: " << *encoding <<'\n';
    else 
        cout << "need to compress with algorithm: " << *compression 
             << " at level: " << compressionLevel <<'\n';
    cout << "sources: {" << join(sources.begin(), sources.end(), ", ") << "}\n";
    cout << "into: " << destination <<'\n';
}
```
</details>

## Integration

You can integrate Argum into your code in the following ways

### Single header 

Download `argum.h` from the [Releases][releases] page, drop it into your project and `#include` it. This is the simplest way.

### Module
  
If you are lucky (or unlucky?) to have a compiler and build system that support modules you can try to use **experimental** 
module file. Download `argum-module.ixx` from [Releases][releases] page and integrate it into you project. Note, that of the compilers I
have access to, only MSVC and GCC currently supports modules to any usable extent and, even there, many things appear to be broken. 
If you encounter internal compiler errors please complain to your compiler vendor. Use at your own risk.

### CMake
  
With modern CMake you can easily integrate Argum as follows:
```cmake
include(FetchContent)
FetchContent_Declare(argum
        GIT_REPOSITORY git@github.com:gershnik/argum.git
        GIT_TAG <desired tag like v1.2>
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(argum)
``` 
  
Alternatively you can clone this repository somewhere and do this:

```cmake
add_subdirectory(PATH_WHERE_YOU_DOWNALODED_IT_TO, argum)
```

In either case you can then use `argum` as a library in your project. 

Whichever method you use in order to use Argum your compiler needs to be set in C++20 mode. 
Argum should compile cleanly even on a highest warnings level. 

On MSVC you need to have `_CRT_SECURE_NO_WARNINGS` defined to avoid its bogus "deprecation" warnings.

## Configuration

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
    #else
        std::terminate(); 
    #endif
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

### Why isn't it using [C++20 feature X]?

This is simply due to the fact that, currently, not all compilers and standard libraries have support for all C++20 features. Notably Ranges support is lacking on many. Once C++20 support improves this library will attempt to adjust when appropriate.

