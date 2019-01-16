
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
#pragma once

#include <list>
#include <vector>

#include "processornodebase.h"

#include "processor/data.h"

namespace VDD
{
    typedef std::vector<Data> Views;

    /** @brief A processor that runs in its own thread, maybe receives
        input data and produces output data.

        A processor typically runs in its own thread.
    */
    class Processor : public ProcessorNodeBase
    {
    public:
        Processor();
        virtual ~Processor();

        /** @bried What is the preferred input storage format? Input not
            in this storage may be rejected by Processor
        */
        virtual StorageType getPreferredStorageType() const = 0;

        /** @brief Provides data; may return one or multiple results. In
            particular an encoder might first return 0 results, and after
            a while (or after stream end) multiple results.

            Each element is for one frame and each element in the frame is
            for different streams (ie. a tiler might produce one frame with
            multiple streams).

            @throws MismatchingStorageTypeException if the data isn't in
            suitable storage for this processor

            @returns a list of vectors of Data. When the stream ends, for
            each output stream an empty Data() (default-constructed Data)
            is returned.
        */
        virtual std::vector<Views> process(const Views& data) = 0;
    };

#ifdef FUTURE
    class MessagesQueueClosedException;

    class MessageQueueBase;

    /** @brief Asynchronous buffering message queue */
    template <typename T>
    class MessageQueue : public MessageQueueBase
    {
    public:
        MessageQueue();
        virtual ~MessageQueue();

        /** @brief Adds a message to the queue.

            @throws MessagesQueueClosedException if there are no messages and
           the message queue is closed
        */
        void send(const T& message);

        /** @brief Retrives a message from the queue

            @throws MessagesQueueClosedException if the message queue is closed
        */
        T receive();

        /** @brief Closes the message queue. All remaining messages will
            need to be read, but no new messages may be added. After
            reading the last message reading another will cause the
            exception MessagesQueueClosedException to be thrown on the
            sender. */
        void close();

    private:
    };

    /** @brief Synchronously listen for a list of queues for the first
        available message */
    MessageQueueBase select(std::list<MessageQueueBase*> queues);

    class Node
    {
    public:
        /** @brief a processing node with given instructions for its input
           frame.
            The first entry in the graph will not receive instructions. */

        Node(Processor* processor, Instructions instructions);

        ~Node();

        /** @brief Get the next nodes to go after processing this node */
        std::list<const Node*> getNext() const;

        /** @brief Get current node's processor */
        Processor* getProcessor() const;

        /** @brief Get current node's processing instructions */
        Instructions* getInstructions() const;
    };

    class Graph
    {
    public:
        Graph();
        ~Graph();

        /** @brief Add an edge to the graph going from one node to another */
        void add(Node src, Node dst);
    };

    void process(Data& data, Graph& path);
#endif  // FUTURE

}  // namespace VDD

