# Argum

Fully-featured, powerful and simple to use C++ command line argument parser.

<!-- TOC -->

- [Argum](#argum)
    - [Features and goals](#features-and-goals)
    - [On exceptions](#on-exceptions)
    - [On ranges and coroutines](#on-ranges-and-coroutines)
    - [Examples](#examples)
    - [Why another library?](#why-another-library)

<!-- /TOC -->

## Features and goals

* Supports the commonly used Unix and Windows command line conventions (Posix, GNU extensions to it, Python's Argparse, Microsoft syntax for its better designed utilities etc.). It should be possible to process complicated command lines like the ones of `git` or `clang` using this library.
* Sequential processing. The arguments are handled in the order they are specified so you can handle things differently based on order if you want to. 
* Adaptive processing. You can also modify parser definitions while processing - this way you can _adapt_ the argument handling on the fly rather than have to pre-configure everything at once. 
* Simple to use. 
  * This library does not attempt to shove all the arguments into a map like data structure and neither does it attempt to define increasingly bizarre ways to "bind" arguments to variables in your code and convert them to internal types. 
  * Instead you provide lambdas (or any other function objects of course) that put the data where you need it and convert how you want it. Beyond toy examples this usually results in simpler code and less mental effort to write.
  * Simple and extensible way to do rule based syntax validation 
* Configurability beyond just "globally replace `-` with `/`". You can specify what prefixes to use on short options, long options, which prefixes are equivalent, disable short or long options behaviors altogether, what separator to use for arguments instead of `=` and more.
  * The default syntax is the common Unix/GNU one 
  * Pre-built configurations are available for GNU "long options only) and a couple of common Windows syntaxes.
  * You can define your own configurations from scratch or modify one of the above (e.g. add `+` as short option prefix in addition to `-`)
* Ability to handle response files. 
* No dependencies beyond C++ standard library. Can be used via a single file header.
  * Requires C++20 or above. 
  * Does not use `iostreams` in any way.
* Equivalent support for `char` and `wchar_t`. 
* Allows for localization with user-supplied message translations.
* Modularity
  * Everything is not shoved into a one giant "parser" class. You can reuse different parts of the library if you really need to build something unusual. Similarly, things like printing help messages expose functions that print various parts of the whole to allow you to build your own help while reusing the tedious bits.


## On exceptions

This library currently uses exceptions to signal parsing errors. This is controversial in some circles and for some very good reasons. Unfortunately, there is currently no standard way of writing clear exception free code. Any attempt to do so today will require custom error handling machinery that will eventually become obsolete and maintenance burden once one or more of currently proposed <code>std::expected</code>, deterministic <code>try</code> or something else become part of the standard language. Rather than lock itself into proprietary machinery now, Argum uses exceptions. When the language direction for exception free code becomes clear it will be updated to allow operating in this mode.

Note that exceptions are only used for _parsing_ errors. Logic errors such as passing incorrect parameters to configure parser will `assert` in debug and `std::terminate` in non-debug builds.

## On ranges

Despite targeting C++20 the library currently doesn't use/understand ranges. This is simply due to the fact that, currently, not all compilers have standard libraries that include them. Once ranges become widely available the plan is to switch to using them.

## Examples

## Why another library?

There are quite a few command line parsing libraries for C++ out there including [Boost.Program_options](https://www.boost.org/doc/libs/1_78_0/doc/html/program_options.html), [Lyra](https://github.com/bfgroup/Lyra), [TCLAP](http://tclap.sourceforge.net) and many others. 
Unfortunately, I found none of them particularly easy to use or satisfy all of my requirements. In particular I need:
* Header-only and ideally single file library that can be quickly used in any little command line project without setting up and building a library dependency. 
* Command line processing is usually a small part of what your my actually does. A library that constantly requires me to read its manual to do anything (e.g. "how to bind this to that while making sure it is valid") is a pain to use and wastes my time.
* Handle sequential parsing. It is not uncommon to have meaningful order of options and positional arguments and parsers that don't allow making decisions based on order are just standing in the way.
* Configurability. Most libraries do let you change prefixes but usually in a clumsy way that doesn't really work well on Windows.
* Modularity. Most often this is an issue if you want a different way to display help while not redoing everything from scratch. Most libraries hide everything inside and only expose high level "print help" method with some custom header and footer. Also nobody exposes internal lexer/tokenizer which would allow different parsing semantics.

Some libraries do better on some of these but none I could find do everything well. Hence this project.













