//
// Copyright 2015 (C). Alex Robenko. All rights reserved.
//

// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#pragma once

#include <memory>

#include <QtWidgets/QAction>

#include "Protocol.h"
#include "Socket.h"

namespace comms_champion
{

class PluginControlInterface
{
public:
    typedef std::shared_ptr<QAction> ActionPtr;

    static unsigned version();
    static void setProtocol(ProtocolPtr protocol);
    static void clearProtocol();
    static void addSocket(SocketPtr socket);
    static void removeSocket(SocketPtr socket);
    static void addMainToolbarAction(ActionPtr action);
    static void removeMainToolbarAction(ActionPtr action);
};

}  // namespace comms_champion


