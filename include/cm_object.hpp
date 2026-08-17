// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#pragma once

#include <sdbusplus/async/server.hpp>
#include <xyz/openbmc_project/Association/Definitions/aserver.hpp>

#include <string>

namespace concurrent_maintenance
{

// Template alias required by server_t
template <typename Instance, typename Server>
using AssocDefsAServer =
    sdbusplus::aserver::xyz::openbmc_project::association::Definitions<Instance,
                                                                       Server>;

class CMObject : public sdbusplus::async::server_t<CMObject, AssocDefsAServer>
{
  public:
    CMObject(sdbusplus::async::context& ctx, const std::string& cmPath,
             const std::string& inventoryPath);

    CMObject(const CMObject&) = delete;
    CMObject& operator=(const CMObject&) = delete;
    CMObject(CMObject&&) = delete;
    CMObject& operator=(CMObject&&) = delete;

    ~CMObject()
    {
        this->emit_removed();
    }

    // Get the object path
    const std::string& getPath() const
    {
        return objectPath;
    }

  private:
    std::string objectPath;
};

} // namespace concurrent_maintenance
