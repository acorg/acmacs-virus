#include "acmacs-base/argv.hh"
#include "acmacs-base/counter.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-virus/virus-name-normalize.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str> from_file{*this, 'f', "from", desc{"read names from file (one per line)"}};
    option<bool> print_messages{*this, 'm', desc{"print messages (when reading from file)"}};
    option<bool> print_hosts{*this, "hosts", desc{"print all hosts found (when reading from file)"}};
    option<bool> print_bad{*this, 'b', "bad", desc{"print names which were not parsed (when reading from file)"}};

    argument<str_array> names{*this, arg_name{"name"}};
};

static void names_from_file(const Options& opt);

// ----------------------------------------------------------------------

int main(int argc, const char* const* argv)
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        if (opt.from_file) {
            names_from_file(opt);
        }
        else if (!opt.names.empty()) {
            for (const auto& src : opt.names) {
                acmacs::virus::name::parsing_messages_t messages;
                const auto fields = acmacs::virus::name::parse(src, messages);
                if (!messages.empty())
                    AD_WARNING("{}", messages);
                fmt::print("{} -> {}\n", src, fields);
            }
        }
        else
            throw std::runtime_error{fmt::format("Usage: {} [-h] [-f <filename>] [<name> ...]", argv[0])};
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 1;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void names_from_file(const Options& opt)
{
    acmacs::Counter<acmacs::virus::host_t> hosts;

    const std::string lines = acmacs::file::read(opt.from_file);
    size_t lines_read{0}, succeeded{0}, failed{0};
    acmacs::virus::name::parsing_messages_t messages;
    for (const auto& line : acmacs::string::split(lines, "\n", acmacs::string::Split::RemoveEmpty)) {
        ++lines_read;
        const auto messages_before = messages.size();
        const auto fields = acmacs::virus::name::parse(line, messages);
        if (messages_before != messages.size()) {
            ++failed;
            if (opt.print_bad)
                fmt::print("{}\n", line);
        }
        else
            ++succeeded;
        if (!fields.host.empty())
            hosts.count(fields.host);
        // fmt::print("{} -> {}\n", line, fields);
    }
    fmt::print("Lines: {:6d}\nGood:  {:6d}\nBad:   {:6d}\n", lines_read, succeeded, failed);
    if (opt.print_messages)
        acmacs::virus::name::report_by_key(messages);
    if (opt.print_hosts)
        fmt::print("\nHosts ({})\n{}\n", hosts.size(), hosts.report_sorted_max_first("    {first:40s} {second}\n"));

} // names_from_file

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
