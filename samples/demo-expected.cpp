#define ARGUM_USE_EXPECTED
#include <argum.h>

#include <iostream>

using namespace Argum;
using namespace std;


int main(int argc, char * argv[]) {

    vector<string> sources;
    string destination;
    enum Encoding { defaultEncoding, Base64, Hex };
    optional<Encoding> encoding = defaultEncoding;
    optional<string> compression;
    int compressionLevel = 9;

    const char * progname = (argc ? argv[0] : "my_utility");
    const ColorStatus envColorStatus = environmentColorStatus();

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
            auto colorizer = colorizerForFile(envColorStatus, stdout);
            cout << parser.formatHelp(progname, terminalWidth(stdout), colorizer);
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
        auto colorizer = colorizerForFile(envColorStatus, stderr);
        cerr << colorizer.error(error->message()) << "\n\n";
        cerr << parser.formatUsage(progname, terminalWidth(stderr), colorizer) << '\n';
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
