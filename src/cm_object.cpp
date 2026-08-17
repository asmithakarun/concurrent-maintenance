// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "cm_object.hpp"

#include <phosphor-logging/lg2.hpp>

namespace concurrent_maintenance
{

CMObject::CMObject(sdbusplus::async::context& ctx, const std::string& cmPath,
                   const std::string& inventoryPath) :
    sdbusplus::async::server_t<CMObject, AssocDefsAServer>(ctx, cmPath.c_str()),
    objectPath(cmPath)
{
    // Association:
    // "inventory"  — from the CM object, the endpoint is an inventory item
    // "cm_object"  — from the inventory item, what points at it is a cm_object
    this->associations({{"inventory", "cm_object", inventoryPath}});

    this->emit_added();

    lg2::info(
        "CM object created at {PATH} with association to inventory {INV_PATH}",
        "PATH", cmPath, "INV_PATH", inventoryPath);
}

} // namespace concurrent_maintenance
