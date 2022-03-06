# Argum Tutorial

<!-- TOC depthfrom:2 -->

- [The basics](#the-basics)
- [Adding help](#adding-help)
- [Positional arguments](#positional-arguments)

<!-- /TOC -->

## The basics

Let us start with a very simple example which does (almost) nothing:

```cpp
#include <argum/parser.h>
//or #include <argum-all.h> if you use a single header distribution
//or import Argum; if you use module distribution

#include <iostream>

//everything in Argum library is in this namespace. 
using namespace Argum;

int main(int argc, char * argv[]) {

    Parser parser;

    try {
        //parse the arguments starting from argv[1] (if exists)
        parser.parse(argc, argv);
    } catch (ParsingException & ex) {
        //all parsing issues are reported via classes derived from ParsingException
        std::cerr << ex.message() << '\n';
        //print the formatted usage string. The argument to formatUsage is the name of our program 
        //we want printed in the usage string
        std::cerr << parser.formatUsage(argc ? argv[0] : "prog") << '\n';
        return EXIT_FAILURE;
    }
}
```

Assuming this code has been compiled into an application named `prog` following are the results of running it:

```console
$ ./prog
$ ./prog --help
unrecognized option: --help
Usage: ./prog
$ ./prog foo
unexpected argument: foo
Usage: ./prog
```

As you can see, the program accepts no command line arguments at all. Note, that unlike some other parsers, Argum does not automatically add `--help` option. We will see how to add it in the next section.

This example demonstrates the basics of handling errors and displaying usage information to the user. All errors produced during parsing are reported via classes derived from `ParsingException` (itself a subclass of `std::exception`). We handle the exception by printing the error message and the program's short usage string. 
Note that we have to pass the name of our program to `formatUsage`. It does not make any assumptions of what the name you want printed 
is or whether it should be taken from `argv[0]`. It is common to use `argv[0]` but it is not the only choice - some people prefer to hardcode the name. **IMPORTANT**: if you do use `argv[0]` never assume that `argc > 0` and it is valid. See [this vulnerability][1] for 
gory details of what can happen if you do.

In addition to standard `what()` method that can be used to display error information `ParsingException` also has the `message()` method, used above. The difference between the two is that `message()` returns a `string_view` and has a `wchar_t` counterpart. Here is the same code for Windows `wmain()` that uses `wchar_t` throughout.
In what follows we will also omit the `#include` and `using namespace` parts - these are assumed throughout this tutorial.

```cpp
int wmain(int argc, wchar_t * argv[]) {

    WParser parser;

    try {
        parser.parse(argc, argv);
    } catch (WParsingException & ex) {
        std::wcerr << ex.message() << L'\n';
        std::wcerr << parser.formatUsage(argc ? argv[0] : L"prog") << L'\n';
        return EXIT_FAILURE;
    }
}
```

Note the `W` prefix on `Parser` and `ParsingException`. Argum follows the same pattern as `std::string`/`std::wstring` in this respect. 
The rest of the examples in this tutorial will be using narrow `char`s for simplicity. You can always convert them to `wchar_t` by simple substitution of `W` prefixes and adding `L` to strings.

## Adding help

As mentioned above, Argum does not, on its own, add any kind of automatic help support to the parser. Adding one is easy, though. Let's do it.

```cpp
#include <cstdlib>

int main(int argc, char * argv[]) {

    const char * progname = (argc ? argv[0] : "prog");

    Parser parser;
    parser.add(
        Option("--help", "-h"). //different names of the option. The first one is the "main" name
        help("show this help message and exit"). //help message for the option
        handler([&]() {  //handler has no arguments - this means the option has no arguments

            std::cout << parser.formatHelp(progname);
            std::exit(EXIT_SUCCESS);
    }));

    try {
        parser.parse(argc, argv);
    } catch (ParsingException & ex) {
        std::cerr << ex.message() << '\n';
        std::cerr << parser.formatUsage(progname) << '\n';
        return EXIT_FAILURE;
    }
}
```
The code should be self-explanatory: we added an option named `--help` or `-h` with a
help message "show this help message and exit" and a handler that prints out the program help and exits.

Running this code produces
```console
$ ./prog
$ ./prog --help
Usage: ./prog [--help]

options:
  --help, -h  show this help message and exit

$ ./prog  -h
Usage: ./prog [--help]

options:
  --help, -h  show this help message and exit

$ ./prog foo
unexpected argument: foo
Usage: ./prog [--help]
$ ./prog --help=foo
extraneous argument for option: --help
Usage: ./prog [--help]

```

Note the last error. Option `--help` we added does not accept argument - this is because its handler doesn't have one. An attempt to pass what unambiguously looks like an argument results in error. However, let's try this

```console
$ ./prog --help foo bar
Usage: ./prog [--help]

options:
  --help, -h  show this help message and exit

```

The parser seem to have ignored the invalid arguments following `--help`. Why?
What is very important to understand is that argument handlers are **invoked sequentially, as they are parsed** - not after the whole 
command line was validated like most other parsers do. Thus when the handler for "--help" is invoked invalid arguments `foo` and `bar` haven't been handled yet. The `--help` handler we specified prints the help message and exits the program so no error is ever recorded.
This sequential behavior is worth noticing because it will turn up very handy later.

## Positional arguments

Let's add a positional argument to our parser.

```cpp
int main(int argc, char * argv[]) {

    std::string stringToEcho;

    const char * progname = (argc ? argv[0] : "prog");
    Parser parser;
    parser.add(
        Positional("echo"). //name of the argument - this is used in help and error messages
        help("echo the string you use here"). //description to show in help
        handler([&](const std::string_view & value) { 
            stringToEcho = value;
    }));
    parser.add(
        Option("--help", "-h").help("show this help message and exit"). 
        handler([&]() {
            std::cout << parser.formatHelp(progname);
            std::exit(EXIT_SUCCESS);
    }));

    try {
        parser.parse(argc, argv);
    } catch (ParsingException & ex) {
        std::cerr << ex.message() << '\n';
        std::cerr << parser.formatUsage(progname) << '\n';
        return EXIT_FAILURE;
    }

    //if we reach this point we are guarranteed that an argument was passed in
    //and its content is in stringToEcho
    std::cout << stringToEcho << '\n';
}
```

Running this code produces the following
```console
$ ./prog       
invalid arguments: positional argument echo must be present
Usage: ./prog [--help] echo
$ ./prog --help                            
Usage: ./prog [--help] echo

positional arguments:
  echo        echo the string you use here

options:
  --help, -h  show this help message and exit

$ ./prog foo
foo
```

As you can see running the program now requires specifying the argument. Now let's do something more complicated

```cpp

#include <charconv>

int main(int argc, char * argv[]) {

    int numberToSquare;

    const char * progname = (argc ? argv[0] : "prog");
    Parser parser;
    parser.add(
        Positional("square").
        help("display a square of a given number"). 
        handler([&](const std::string_view & value) { 
            //parse the value into an integer. 
            //See https://en.cppreference.com/w/cpp/utility/from_chars for details about std::from_chars
            auto [ptr, ec]  = std::from_chars(value.data(), value.data() + value.size(), numberToSquare);
            if (ec == std::errc::invalid_argument || ptr != value.data() + value.size())
                throw Parser::ValidationError(std::string(value) + " is not a number");
            else if (ec == std::errc::result_out_of_range)
                throw Parser::ValidationError(std::string(value) + " is out of range");
            //make sure the number can be squared without overflow
            if (abs(numberToSquare) > std::numeric_limits<int>::max() / abs(numberToSquare))
                throw Parser::ValidationError(std::string(value) + " is out of range");
    }));
    parser.add(
        Option("--help", "-h").help("show this help message and exit"). 
        handler([&]() {
            std::cout << parser.formatHelp(progname);
            std::exit(EXIT_SUCCESS);
    }));

    try {
        parser.parse(argc, argv);
    } catch (ParsingException & ex) {
        std::cerr << ex.message() << '\n';
        std::cerr << parser.formatUsage(progname) << '\n';
        return EXIT_FAILURE;
    }

    std::cout << numberToSquare * numberToSquare << '\n';
}
```
This example demonstrates how to deal with argument validation errors in your handlers. All argument values are passed to your handlers
as `std::string_view`s - Argum does not convert them to any other data types. If you want your argument to become something else you
need to do the conversion. Such conversion can fail due to malformed input and the way to integrate such failures into existing machinery
is to throw an exception derived from `ParsingException`. Usually, what you want to use is `Parser::ValidationError` which is the exception
Argum itself uses when command line validation (described below) fails.

Running this code produces

```console
$ ./prog --help    
Usage: ./prog [--help] square

positional arguments:
  square      display a square of a given number

options:
  --help, -h  show this help message and exit

$ ./prog 4       
16
$ ./prog foo
invalid arguments: foo is not a number
Usage: ./prog [--help] square
$ ./prog 1234567890
invalid arguments: 1234567890 is out of range
$ ./prog "45 abc"
invalid arguments: 45 abc is not a number
Usage: ./prog [--help] square
```

What if we wanted to make the positional argument optional? This is also easy. Starting from the next example let's move lengthy lambdas into separate functions to make code clearer.

```cpp

#include <charconv>

[[noreturn]] void printUsageAndExit(const Parser & parser, const char * progname) {
    std::cout << parser.formatHelp(progname);
    std::exit(EXIT_SUCCESS);
}

int parseSquarableInt(std::string_view value) {
    int ret;
    auto [ptr, ec]  = std::from_chars(value.data(), value.data() + value.size(), ret);
    if (ec == std::errc::invalid_argument || ptr != value.data() + value.size())
        throw Parser::ValidationError(std::string(value) + " is not a number");
    else if (ec == std::errc::result_out_of_range)
        throw Parser::ValidationError(std::string(value) + " is out of range");
    if (abs(ret) > std::numeric_limits<int>::max() / abs(ret))
        throw Parser::ValidationError(std::string(value) + " is out of range");
    return ret;
}

int main(int argc, char * argv[]) {

    std::optional<int> numberToSquare; //the value is now optional

    const char * progname = (argc ? argv[0] : "prog");
    Parser parser;
    parser.add(
        Positional("square").
        help("display a square of a given number"). 
        occurs(neverOrOnce).  //you can also say zeroOrOneTime or Quantifier(0,1)
        handler([&](const std::string_view & value) { 
            numberToSquare = parseSquarableInt(value);
    }));
    parser.add(
        Option("--help", "-h").help("show this help message and exit"). 
        handler([&]() {
            printUsageAndExit(parser, progname);
    }));

    try {
        parser.parse(argc, argv);
    } catch (ParsingException & ex) {
        std::cerr << ex.message() << '\n';
        std::cerr << parser.formatUsage(progname) << '\n';
        return EXIT_FAILURE;
    }

    if (numberToSquare)
        std::cout << *numberToSquare * *numberToSquare << '\n';
}
```

Note the `occurs()` call. It takes a `Quantifier` object that specifies minimum and maximum number of times a positional can occur. There are some predefined quantifiers such as: `zeroOrOneTime` or `neverOrOnce` (both mean the same), `oneTime` or `once` (this is the default), `zeroOrMoreTimes` and `oneOrMoreTimes` or `onceOrMore` which cover most common scenarios. For anything else you can pass a 
`Quantifer(min, max)`. To specify "unlimited" for max pass `Quantifer::infinity`.

Running the changed code now produces

```console
$ ./prog --help                                                   
Usage: ./prog [--help] [square]

positional arguments:
  square      display a square of a given number

options:
  --help, -h  show this help message and exit

$ ./prog
$ ./prog 5
25
```

What happens if you have multiple positionals defined with different quantifiers? How does the parser decide which argument belongs where?
The algorithm it follows is very simple:
1. First it makes sure that there is enough arguments to satisfy the minima of all of them. If not an error is reported.
2. Once the minima are satisfied the parser **greedily** tries to match up to each positional maximum from left to right.
If you are familiar with regular expressions and quantifiers there this is the exact same approach.

Let's look at a concrete example. Consider a utility that takes zero or more source files and one destination file as input. It would
be coded like this

```cpp
int main(int argc, char * argv[]) {

    std::vector<std::string> sources;
    std::string destination;

    const char * progname = (argc ? argv[0] : "prog");
    Parser parser;
    parser.add(
        Positional("source").
        help("source file"). 
        occurs(zeroOrMoreTimes).
        handler([&](const std::string_view & value) { 
            sources.emplace_back(value);
    }));
    parser.add(
        Positional("destination").
        help("destination file"). 
        occurs(once). //this could be omitted as it is the default
        handler([&](const std::string_view & value) { 
            destination = value;
    }));
    parser.add(
        Option("--help", "-h").help("show this help message and exit"). 
        handler([&]() {
            printUsageAndExit(parser, progname);
    }));

    try {
        parser.parse(argc, argv);
    } catch (ParsingException & ex) {
        std::cerr << ex.message() << '\n';
        std::cerr << parser.formatUsage(progname) << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "sources: " << join(sources.begin(), sources.end(), ", ") << '\n'; //join is a useful helper in Argum
    std::cout << "destination: " << destination << '\n';
}
```

Running this produces

```console
$ ./prog --help                        
Usage: ./prog [--help] [source [source ...]] destination

positional arguments:
  source       source file
  destination  destination file

options:
  --help, -h   show this help message and exit

$ ./prog
invalid arguments: positional argument destination must be present
Usage: ./prog [--help] [source [source ...]] destination
$ ./prog a
sources: 
destination: a
$ ./prog a b
sources: a
destination: b
$ ./prog a b c
sources: a, b
destination: c
```

<!-- References -->

[1]: <https://blog.qualys.com/vulnerabilities-threat-research/2022/01/25/pwnkit-local-privilege-escalation-vulnerability-discovered-in-polkits-pkexec-cve-2021-4034> 


