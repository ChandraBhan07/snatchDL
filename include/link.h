#pragma once
#include <string>

struct Link {
    std::string id;
    std::string source;

    Link(std::string id_, std::string source_)
        : id(std::move(id_)),
          source(std::move(source_)) {}
};
