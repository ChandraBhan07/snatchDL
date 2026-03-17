#include "cmdOptions.h"

CmdOptions CmdOptions::parse(int argc, char *argv[])
{
    CmdOptions options;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h")
        {
            options.show_help_ = true;
        }
        else if (arg == "--nolisten")
        {
            options.listen_ = false;
        }
        else if (arg[0] == '-')
        {
            throw std::runtime_error("Unknown option: " + arg);
        }
        else
        {
            if (options.filename_.empty())
            {
                options.filename_ = arg;
            }
            else
            {
                throw std::runtime_error("Multiple filenames not supported.");
            }
        }
    }

    options.validate();
    return options;
}

void CmdOptions::validate() const
{
    if (!show_help_ && filename_.empty())
        throw std::runtime_error("Filename is required, use --help to know more.");
}
