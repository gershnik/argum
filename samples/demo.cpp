#include "../single-file/argum.h"

#include <iostream>

//For fileno() and isatty()
#include <stdio.h>
#ifndef _WIN32
    #include <unistd.h>
#else
    #include <io.h>
#endif

using namespace Argum;
using namespace std;

//Portable isatty
bool isAtTty(FILE * fp) {
#ifndef _WIN32
    return isatty(fileno(fp));
#else
    return _isatty(_fileno(fp));
#endif
}

Colorizer colorizerForFile(ColorStatus envColorStatus, FILE * fp) {

    if (envColorStatus == ColorStatus::required || 
        (envColorStatus == ColorStatus::allowed && isAtTty(fp))) {
        return defaultColorizer();
    }
    return {};
}

int main(int argc, char * argv[]) {

    ColorStatus envColorStatus = environmentColorStatus();
    auto colorizerForStdout = colorizerForFile(envColorStatus, stdout);
    auto colorizerForStderr = colorizerForFile(envColorStatus, stderr);

    vector<string> sources;
    string destination;
    enum Encoding { defaultEncoding, Base64, Hex };
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
            cout << parser.formatHelp(progname, colorizerForStdout);
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
        cerr << colorize<Color::bold,Color::red>(ex.message()) << "\n\n";
        cerr << parser.formatUsage(progname, colorizerForStderr) << '\n';
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
