#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <unordered_set>

class ServiceRegistry;
class JobHistory;
class Job;

class JobBuilder
{
public:
    JobBuilder(ServiceRegistry &registry, JobHistory &history) : registry(registry), history(history) {}
    std::optional<Job> build(std::string text);

private:
    ServiceRegistry &registry;
    JobHistory &history;
    std::unordered_map<std::string, std::unordered_set<std::string>> seen;
};