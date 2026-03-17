#include <iostream>
#include <stdexcept>
#include <string>

class CmdOptions
{
public:
    bool show_help() const { return show_help_; }
    bool listen() const { return listen_; }
    const std::string &filename() const { return filename_; }

    static CmdOptions parse(int argc, char *argv[]);

private:
    bool show_help_ = false;
    bool listen_ = true;
    std::string filename_;
    void validate() const;
};