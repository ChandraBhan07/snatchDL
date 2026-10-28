#pragma once

#include "service.h"

class ServiceRegistry
{
public:
    ServiceRegistry();
    const Service *findByKey(const std::string &key) const;
    Service *findByHost(const std::string &host) const;

private:
    std::unordered_map<std::string, Service> services;
    std::unordered_map<std::string, Service *> hostToService;
};