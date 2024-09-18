#ifndef OPTION_PARSER_H
#define OPTION_PARSER_H

#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <sstream>
#include <iterator>

// Arguments holder
struct Option {
    char short_name;
    std::string long_name;
    std::string description;
    bool required;

    // Constructor for options with required defaulting to true
    Option(char opt, const std::string& arg, const std::string& desc, bool req = true)
        : short_name(opt), long_name(arg), description(desc), required(req) {}
};

// Class to manage command-line options and positional arguments
class OptionParser {
public:
    OptionParser(const std::vector<Option>& options, const std::string& optstring, 
                 const std::string& program_desc)
        : options_(options), optstring_(optstring), program_desc_(program_desc) {

        // add help to optstring
        optstring_ += "h";
        // Identify the positional argument if defined
        for (const auto& opt : options_) {
            if (optstring_.find(opt.short_name) == std::string::npos) {
                if (positional_arg_defined_) {
                    throw std::runtime_error("Only one positional argument is allowed. Check options and optstring.");
                }
                positional_arg_defined_ = true;
                positional_arg_short_name_ = std::string(1, opt.short_name);
                positional_arg_name_ = opt.long_name;
                positional_arg_description_ = opt.description;
            }
        }
    }

    std::map<std::string, std::string> parse(int argc, char* argv[]) {
        std::map<std::string, std::string> parsed_args;

        // Default to showing help if no arguments are provided
        if (argc == 1) {
            print_usage(argv[0]);
            return {}; // Return an empty map, indicating that help was requested
        }

        int opt;
        
        // Parse the options using getopt
        while ((opt = getopt(argc, argv, optstring_.c_str())) != -1) {
            if (opt == 'h') {
                print_usage(argv[0]);
                return {}; // Return an empty map, indicating that help was requested
            }

            // Convert char option to string key
            parsed_args[std::string(1, opt)] = optarg ? optarg : "";
        }

        // Handle positional arguments
        if (positional_arg_defined_) {
            std::string positional_args;
            for (int index = optind; index < argc; ++index) {
                if (!positional_args.empty()) {
                    positional_args += " ";
                }
                positional_args += argv[index];
            }

            if (!positional_args.empty()) {
                parsed_args[positional_arg_short_name_] = positional_args; // Use the short name as the key
            }
        } else if (optind < argc) {
            std::cerr << "Error: Positional arguments are not accepted." << std::endl;
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }

        // Check if all required options are present
        check_required_options(parsed_args);

        return parsed_args;
    }

    static std::vector<std::string> split_filenames(const std::string& str) {
        std::istringstream iss(str);
        return std::vector<std::string>{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
    }

private:
    void print_usage(const std::string& program_name) const {
        std::cerr << "Usage: \n";
        std::cerr << "\t" << program_name << " ";

        if (positional_arg_defined_) {
            std::cerr << positional_arg_name_ << " ";
        }
        
        for (const auto& opt : options_) {
            if (optstring_.find(opt.short_name) != std::string::npos) {
                // std::cerr << "[-" << opt.short_name << " " << opt.long_name << "] "; TODO: add support for optional arguments
                std::cerr << "-" << opt.short_name << " " << opt.long_name << " ";
            }
        }

        std::cerr << "\n\nOptions:\n";
        if (positional_arg_defined_) {
            // std::cerr << "\n\nPositional Argument:\n";
            std::cerr << "\t" << positional_arg_name_ << "\t\t\t" << positional_arg_description_ << "\n\n";
        }

        for (const auto& opt : options_) {
            if (optstring_.find(opt.short_name) != std::string::npos) {
                std::cerr << "\t-" << opt.short_name << " " << opt.long_name << "\t\t" << opt.description << "\n\n";
            }
        }

        std::cerr << "Description:\n\t" << program_desc_ << "\n\n";
        exit(EXIT_FAILURE);
    }

    void check_required_options(const std::map<std::string, std::string>& parsed_args) const {
        for (const auto& opt : options_) {
            if (opt.required) {
                // Check if the required option is present in parsed_args
                if (parsed_args.find(std::string(1, opt.short_name)) == parsed_args.end()) {
                    std::cerr << "Error: Missing required option -" << opt.short_name << " " << opt.long_name << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    std::vector<Option> options_;
    std::string optstring_;
    std::string program_desc_;
    bool positional_arg_defined_ = false;
    std::string positional_arg_short_name_;
    std::string positional_arg_name_;
    std::string positional_arg_description_;
};

#endif // OPTION_PARSER_H
