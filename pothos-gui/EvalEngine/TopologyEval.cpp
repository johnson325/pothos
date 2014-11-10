// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "TopologyEval.hpp"
#include "BlockEval.hpp"
#include <Pothos/Framework.hpp>
#include <Poco/Logger.h>

TopologyEval::TopologyEval(void):
    _topology(new Pothos::Topology()),
    _failureState(false)
{
    return;
}

TopologyEval::~TopologyEval(void)
{
    delete _topology;
}

void TopologyEval::acceptConnectionInfo(const ConnectionInfos &info)
{
    _newConnectionInfo = info;
}

void TopologyEval::acceptBlockEvals(const std::map<GraphBlock *, std::shared_ptr<BlockEval>> &info)
{
    _newBlockEvals = info;
}

void TopologyEval::update(void)
{
    if (this->isFailureState()) return;

    //remove all connections from the topology
    try
    {
        _topology->disconnectAll();
    }
    catch (const Pothos::Exception &ex)
    {
        poco_error(Poco::Logger::get("PothosGui.TopologyEval.disconnectAll"), ex.displayText());
        _failureState = true;
        return;
    }

    //create new connections
    for (const auto &conn : _newConnectionInfo)
    {
        assert(_newBlockEvals.count(conn.srcBlock) != 0);
        assert(_newBlockEvals.count(conn.dstBlock) != 0);
        auto src = _newBlockEvals.at(conn.srcBlock)->getProxyBlock();
        auto dst = _newBlockEvals.at(conn.dstBlock)->getProxyBlock();
        try
        {
            _topology->connect(src, conn.srcPort, dst, conn.dstPort);
        }
        catch (const Pothos::Exception &ex)
        {
            poco_error(Poco::Logger::get("PothosGui.TopologyEval.connect"), ex.displayText());
            _failureState = true;
            return;
        }
    }

    //commit after changes
    try
    {
        _topology->commit();
        //stash data for the current state
        _lastBlockEvals = _newBlockEvals;
        _lastConnectionInfo = _newConnectionInfo;
    }
    catch (const Pothos::Exception &ex)
    {
        poco_error(Poco::Logger::get("PothosGui.TopologyEval.commit"), ex.displayText());
        _failureState = true;
        return;
    }
}
