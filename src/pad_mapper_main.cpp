/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#include "gamepad_mapping.h"
#include "input_nodes.h"
#include "pad_map.h"
#include "pad_translator.h"

#include <poll.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <memory>
#include <string>
#include <vector>

namespace
{
    const char * const ActiveDbPath = "/etc/sdl2/gamecontrollerdb.txt";
    constexpr int PollTimeoutMs = 200;
    constexpr auto RescanInterval = std::chrono::milliseconds(1000);

    volatile std::sig_atomic_t running = 1;

    void Stop(int)
    {
        running = 0;
    }

    bool Exists(const std::string & path)
    {
        struct stat info;
        return stat(path.c_str(), &info) == 0;
    }

    long ModifiedTime(const std::string & path)
    {
        struct stat info;
        return stat(path.c_str(), &info) == 0 ? static_cast<long>(info.st_mtime) : 0;
    }

    struct Plan
    {
        std::vector<InputNode> pads;
        std::vector<InputNode> targets;
        std::string signature;
    };

    Plan BuildPlan()
    {
        Plan plan;
        if(Exists(PadMapperPauseFlag))
            return plan;
        const std::vector<InputNode> nodes = ListInputNodes();
        for(const InputNode & pad : UsbPads(nodes))
        {
            const std::string mapPath = ActivePadMapPath(pad);
            const InputNode * target = CloverconFor(pad, nodes);
            if(!target || mapPath.empty())
                continue;
            plan.pads.push_back(pad);
            plan.targets.push_back(*target);
            plan.signature += pad.node + ">" + target->node + "@" + mapPath + ":" + std::to_string(ModifiedTime(mapPath)) + ";";
        }
        if(!plan.pads.empty())
            plan.signature += "db@" + std::to_string(ModifiedTime(ActiveDbPath));
        return plan;
    }
}

int main()
{
    std::signal(SIGTERM, Stop);
    std::signal(SIGINT, Stop);

    std::vector<std::unique_ptr<PadTranslator>> translators;
    std::string signature;
    bool rebuild = true;
    auto nextScan = std::chrono::steady_clock::now();

    while(running)
    {
        if(std::chrono::steady_clock::now() >= nextScan)
        {
            nextScan = std::chrono::steady_clock::now() + RescanInterval;
            const Plan plan = BuildPlan();
            if(rebuild || plan.signature != signature)
            {
                rebuild = false;
                signature = plan.signature;
                translators.clear();
                const GamepadMapping frontMap = LoadActiveMapping(ActiveDbPath);
                for(size_t i = 0; i < plan.pads.size(); ++i)
                {
                    GamepadMapping padMap;
                    if(!LoadPadMap(ActivePadMapPath(plan.pads[i]), padMap))
                        continue;
                    std::unique_ptr<PadTranslator> translator(new PadTranslator(plan.pads[i], plan.targets[i], padMap, frontMap));
                    if(translator->Ok())
                        translators.push_back(std::move(translator));
                }
            }
        }

        if(translators.empty())
        {
            usleep(PollTimeoutMs * 1000);
            continue;
        }
        std::vector<pollfd> fds;
        for(const auto & translator : translators)
            fds.push_back(pollfd{ translator->Fd(), POLLIN, 0 });
        if(poll(fds.data(), fds.size(), PollTimeoutMs) <= 0)
            continue;
        for(size_t i = 0; i < fds.size(); ++i)
            if(fds[i].revents & POLLIN)
                translators[i]->Pump();
            else if(fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
                rebuild = true;
    }
    return 0;
}
