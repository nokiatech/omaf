
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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
#include "controller/omafvicontroller.h"
#include "controller/videoinput.h"
#include "controller/omafviconfig.h"
#include "log/consolelog.h"


int main(int ac, char** av)
{
    std::cout << "mp4conv version " << BuildInfo::Version << " built at " << BuildInfo::Time << std::endl
              << "Convert legacy 360 mp4 video files to OMAF mp4 video files." << std::endl
              << "Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved. " << std::endl << std::endl;

    bool help {};
    std::string videoFilename;
    std::string outputFilename;
    VDD::ConverterConfigTemplate configTemplate {};
    bool verbose {};
    {
        using namespace VDD::CommandLine;
        using namespace VDD::Utils;
        VDD::CommandLine::Parser parser("omafvi");
        parser
            .add(Arg({ 'h' }, { "help" },
                     new Flag(help),
                     "Provide usage information"))
#ifdef _DEBUG
            .add(Arg({}, { "dump-config" },
                     new Flag(configTemplate.dumpConfig),
                     "DEVELOPMENT ONLY: Dump the JSON config used"))
            .add(Arg({}, { "dump-graph" },
                     new String("filename", configTemplate.dumpGraph),
                     "DEVELOPMENT ONLY: Dump the processing graph to this file"))
#endif
            .add(Arg({'v'}, { "verbose" },
                     new Flag(verbose),
                     "Be more verbose"))
            .add(Arg({'i'}, { "video" },
                     new InputFilename("filename", videoFilename),
                     "Set the input MP4 file name for video"))
            .add(Arg({ 'o' }, { "output" },
                    new String("filename", outputFilename),
                    "Set the output MP4/MPD file name for video"))
            ;

        const auto& result = parser.parse({ av + 1, av + ac });

        if (help)
        {
            parser.usage(std::cout);
            return 0;
        }

        if (!result.isOK())
        {
            parser.usage(std::cout);
            std::cerr << "Error while parsing arguments: " << result.getError() << std::endl;
            return 1;
        }
    }

    configTemplate.inputMP4 = videoFilename;
    if (outputFilename.find(".mpd") != std::string::npos)
    {
        configTemplate.outputDASH = outputFilename;
        configTemplate.outputMP4 = "";
    }
    else
    {
        configTemplate.outputMP4 = outputFilename;
        configTemplate.outputDASH = "";
    }

    VDD::ControllerBase::Config config {};
    config.config = VDD::createConvConfigJson(configTemplate);
    config.log = std::shared_ptr<VDD::Log>(new VDD::ConsoleLog());
    if (verbose)
    {
        config.log->setLogLevel(VDD::LogLevel::Info);
    }
    else
    {
        config.log->setLogLevel(VDD::LogLevel::Error);
    }

    int rc = 0;

    enum { Initializing, Running } state = Initializing;
    try
    {
        VDD::OmafVIController controller(config);
        state = Running;
        controller.run();
        VDD::GraphErrors errors = controller.moveErrors();
        for (auto error: errors)
        {
            if (!error.exception)
            {
                std::cerr << "Error: " << error.message << std::endl;
            }
        }
        // actually only the first error is thrown
        for (auto error: errors)
        {
            if (error.exception)
            {
                std::rethrow_exception(error.exception);
            }
        }
    }
    catch (VDD::ConfigError& exn)
    {
        std::cerr << "Error while processing configuration: " << exn.message() << std::endl;
        rc = 1;
    }
    catch (VDD::CannotOpenFile& exn)
    {
        std::cerr << "Error while opening file: " << exn.message() << std::endl;
        rc = 2;
    }
    catch (VDD::CannotWriteException& exn)
    {
        std::cerr << "Error while writing to file " << exn.getFilename() << std::endl;
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
