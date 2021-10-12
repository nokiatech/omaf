
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

#include "buildinfo.h"
#include "commandline.h"
#include "common/exceptions.h"
#include "common/optional.h"
#include "common/utils.h"
#include "controller/omafviconfig.h"
#include "controller/omafvicontroller.h"
#include "controller/videoinput.h"
#include "log/consolelog.h"


int main(int ac, char** av)
{
    std::cout << "omafvi version " << BuildInfo::Version << " built at " << BuildInfo::Time
              << std::endl
              << "Convert legacy 360 mp4 video files to OMAF mp4 video files." << std::endl
              << "Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All "
                 "rights reserved. "
              << std::endl
              << std::endl;

    bool help{};
    std::string videoFilename;
    std::string outputFilename;
    VDD::ConverterConfigTemplate configTemplate{};
    bool verbose{};
    {
        using namespace VDD::CommandLine;
        using namespace VDD::Utils;
        VDD::CommandLine::Parser parser("omafvi");
        bool isH265 = false;
        VDD::Optional<int> gopLength;
        parser.add(Arg({'h'}, {"help"}, new Flag(help), "Provide usage information"))
            .add(Arg({}, {"h265"}, new Flag(isH265),
                     "The input is H265. Provide also --gop-length in this case."))
            .add(Arg({}, {"gop-length"}, new Integer("gop length", gopLength),
                     "Set the Group Of Pictures length for H265 input"))
#ifdef _DEBUG
            .add(Arg({}, {"dump-config"}, new Flag(configTemplate.dumpConfig),
                     "DEVELOPMENT ONLY: Dump the JSON config used"))
            .add(Arg({}, {"dump-graph"}, new String("graph-filename", configTemplate.dumpGraph),
                     "DEVELOPMENT ONLY: Dump the processing graph to this file"))
#endif
            .add(Arg({'v'}, {"verbose"}, new Flag(verbose), "Be more verbose"))
            .add(Arg({'c'}, {"config"}, new AppendString("config-filename", configTemplate.jsonConfig),
                     "Json configuration file"))
            .add(Arg({'i'}, {"video"}, new InputFilename("input-filename", videoFilename),
                     "Set the input MP4 file name for video and audio"))
            .add(Arg({'A'}, {"no-audio"}, new Flag(configTemplate.disableAudio),
                     "Disable audio (required if input file does not have audio)"))
            .add(Arg({'T'}, {"timed-demo"}, new Flag(configTemplate.enableDummyMetadata),
                     "Generate dummy timed metadata"))
            .add(Arg({'o'}, {"output"}, new String("filename", outputFilename),
                     "Set the output MP4/MPD file name for the result"));

        const auto& result = parser.parse({av + 1, av + ac});

        if (help)
        {
            parser.usage(std::cout);
            return 0;
        }

        if (isH265 && !gopLength && outputFilename.find(".mpd") != std::string::npos)
        {
            std::cerr << "You must provide --gop-length for H265 input when creating "
                         "DASH output"
                      << std::endl;
            return 1;
        }

        if (isH265)
        {
            VDD::H265InputConfig h265Config{};
            if (gopLength)
            {
                h265Config.gopLength = *gopLength;
            }
            else
            {
                h265Config.gopLength = 0;
            }
            configTemplate.h265InputConfig = h265Config;
        }

        if (!result.isOK())
        {
            parser.usage(std::cout);
            std::cerr << "Error while parsing arguments: " << result.getError() << std::endl;
            return 1;
        }
    }

    configTemplate.inputFile = videoFilename;
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

    VDD::ControllerBase::Config config{};
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

    enum
    {
        Initializing,
        Running
    } state = Initializing;
    try
    {
        VDD::OmafVIController controller(config);
        for (auto& warning : config.config->warnings())
        {
            std::cerr << "Notice: " << warning << std::endl;
        }
        state = Running;
        controller.run();
        VDD::GraphErrors errors = controller.moveErrors();
        for (auto error : errors)
        {
            if (!error.exception)
            {
                std::cerr << "Error: " << error.message << std::endl;
            }
        }
        // actually only the first error is thrown
        for (auto error : errors)
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
