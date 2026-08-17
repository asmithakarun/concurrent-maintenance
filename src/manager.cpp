// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/match.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <map>
#include <string>
#include <variant>

namespace concurrent_maintenance
{

constexpr auto readyToRemoveProperty = "ReadyToRemove";
constexpr auto cmRemoveObjectPath = "/com/ibm/ConcurrentMaintenance/remove";
constexpr auto cmAddObjectPath = "/com/ibm/ConcurrentMaintenance/add";

Manager::Manager(sdbusplus::async::context& ctx) : ctx(ctx)
{
    lg2::info("Concurrent Maintenance manager initialized");
}

// NOLINTBEGIN(clang-analyzer-core.uninitialized.Branch)
void Manager::start()
{
    // Spawn coroutine to watch for ReadyToRemove property changes
    ctx.spawn(watchReadyToRemove());
}

sdbusplus::async::task<> Manager::watchReadyToRemove()
{
    using PropertiesVariant = std::variant<bool>;
    using ChangedProperties = std::map<std::string, PropertiesVariant>;

    /* Watch for property changes on all child objects under
     * /xyz/openbmc_project/inventory
     */

    sdbusplus::async::match matcher(
        ctx,
        sdbusplus::bus::match::rules::type::signal()
            .append(sdbusplus::bus::match::rules::path_namespace(
                "/xyz/openbmc_project/inventory"))
            .append(sdbusplus::bus::match::rules::member("PropertiesChanged"))
            .append(sdbusplus::bus::match::rules::interface(
                "org.freedesktop.DBus.Properties"))
            .append(sdbusplus::bus::match::rules::argN(
                0, "xyz.openbmc_project.State.ReadyToRemove")));

    lg2::info(
        "ReadyToRemove property watcher registered for all inventory objects");

    while (true)
    {
        auto msg = co_await matcher.next();

        try
        {
            auto [interface, changedProperties] =
                msg.unpack<std::string, ChangedProperties>();

            const auto it = changedProperties.find(readyToRemoveProperty);
            if (it == changedProperties.end())
            {
                continue;
            }

            bool readyToRemove = std::get<bool>(it->second);
            const std::string inventoryPath = msg.get_path();
            lg2::info("ReadyToRemove property changed on {PATH}: {VALUE}",
                      "PATH", inventoryPath, "VALUE", readyToRemove);

            manageCMObject(readyToRemove, inventoryPath);
        }
        catch (const std::exception& e)
        {
            lg2::error("Error handling ReadyToRemove property change: {ERROR}",
                       "ERROR", e);
        }
    }
}
// NOLINTEND(clang-analyzer-core.uninitialized.Branch)

void Manager::manageCMObject(bool readyToRemove,
                             const std::string& inventoryPath)
{
    // Single-CM guard: reject if a CM is already in progress
    if (currentCMObject)
    {
        lg2::error(
            "CM is already in progress at {PATH}. Ignoring new request for {INV_PATH}.",
            "PATH", currentCMObject->getPath(), "INV_PATH", inventoryPath);
        return;
    }

    const std::string path = readyToRemove ? cmRemoveObjectPath
                                           : cmAddObjectPath;

    currentCMObject = std::make_unique<CMObject>(ctx, path, inventoryPath);
    lg2::info("CM object created at {PATH}", "PATH",
              currentCMObject->getPath());
}

} // namespace concurrent_maintenance
