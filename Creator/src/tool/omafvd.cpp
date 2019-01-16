
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include <iostream>

#include "commandline.h"

#include "buildinfo.h"
#include "common/exceptions.h"
#include "common/optional.h"
#include "common/utils.h"
#include "controller/omafvdcontroller.h"
#include "log/consolelog.h"


namespace
{
    void usage()
    {
        std::cout << "Usage: ./omafvd config.json" << std::endl;
    }
}  // anonymous namespace

int main(int ac, char** av)
{
    std::cout << "omafvd version " << BuildInfo::Version << " built at " << BuildInfo::Time << std::endl
              << "Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved. " << std::endl << std::endl;

    int argIndex = 1;
    VDD::ControllerBase::Config config{};
    config.config = std::shared_ptr<VDD::Config>(new VDD::Config());
    config.log = std::shared_ptr<VDD::Log>(new VDD::ConsoleLog());

    if (ac - argIndex < 1)
    {
        usage();
        return -1;
    }
    std::string args(av[argIndex]);

    try
    {
        config.config->commandline({ av, av + ac });
    }
    catch (VDD::ConfigError& exn)
    {
        std::cerr << "Failed to read configuration: " << exn.message() << std::endl;
        return 1;
    }

#ifdef _DEBUG
    config.log->setLogLevel(VDD::LogLevel::Info);
#else
    config.log->setLogLevel(VDD::LogLevel::Error);
#endif

    int rc = 0;

    enum { Initializing, Running } state = Initializing;
    try
    {
        VDD::OmafVDController controller(config);
        for (auto& warning : config.config->warnings())
        {
            std::cerr << "Info: " << warning << std::endl;
        }
        state = Running;
        controller.run();
    }
    catch (VDD::ConfigError& exn)
    {
        std::cerr << "Failed to process configuration: " << exn.message() << std::endl;
        rc = 1;
    }
    catch (VDD::CannotOpenFile& exn)
    {
        std::cerr << "Failed to open file: " << exn.message() << std::endl;
        rc = 2;
    }
    // Comment this out if this makes debugging difficult..
    catch (VDD::Exception& exn)
    {
        switch (state)
        {
        case Initializing:
        {
            std::cerr << "Failed to initialize controller: " << exn.message() << std::endl;
            rc = 3;
            break;
        }
        case Running:
        {
            std::cerr << "Failed to execute controller: " << exn.message() << std::endl;
            rc = 4;
            break;
        }
        }
    }
    return rc;
}
