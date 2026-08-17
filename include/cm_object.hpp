// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#pragma once

#include "fru_identifier.hpp"

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/server.hpp>
#include <sdbusplus/async/task.hpp>
#include <xyz/openbmc_project/Association/Definitions/aserver.hpp>
#include <xyz/openbmc_project/Common/Progress/aserver.hpp>

#include <string>

namespace concurrent_maintenance
{

// Template aliases required by server_t — both take (Instance, Server).
template <typename Instance, typename Server>
using AssocDefsAServer =
    sdbusplus::aserver::xyz::openbmc_project::association::Definitions<Instance,
                                                                       Server>;

template <typename Instance, typename Server>
using ProgressAServer =
    sdbusplus::aserver::xyz::openbmc_project::common::details::Progress<
        Instance, Server>;

using OperationStatus =
    sdbusplus::common::xyz::openbmc_project::common::Progress::OperationStatus;

/**
 * @brief Tracks one in-flight CM operation and drives its execution.
 *
 * Responsibilities:
 *   1. Own the D-Bus tracking object path for the operation.
 *   2. Publish both Association.Definitions and Common.Progress on a single
 *      D-Bus object at that path (one InterfacesAdded signal).
 *   3. Drive the remove or add sequence via execute().
 *   4. Update Progress state via updateStatus().
 *
 * Lifecycle:
 *   Construction: registers Progress at NotStarted (start_time stamped).
 *   execute():    sets InProgress, drives handler,
 *                 sets Completed or Failed (completed_time stamped).
 *
 * Handler functions are stateless inline free functions declared in
 * cm_handlers.hpp. CMObject invokes them — no circular ownership.
 *
 * Manager owns CMObject for the full operation lifetime.
 */
class CMObject :
    public sdbusplus::async::server_t<CMObject, AssocDefsAServer,
                                      ProgressAServer>
{
  public:
    CMObject(sdbusplus::async::context& ctx, const std::string& objectPath,
             const std::string& fruPath);

    CMObject(const CMObject&) = delete;
    CMObject& operator=(const CMObject&) = delete;

    ~CMObject()
    {
        AssocDefsAServer<CMObject, Self>::emit_removed();
        ProgressAServer<CMObject, Self>::emit_removed();
    }

    /** @brief D-Bus object path for this operation. */
    const std::string& getPath() const
    {
        return objectPath;
    }

    /** @brief Inventory path of the FRU under maintenance. */
    const std::string& getFruPath() const
    {
        return fruPath;
    }

    /**
     * @brief Drive the full remove or add sequence asynchronously.
     *
     * Sets Progress to InProgress before invoking the handler, and
     * to Completed on success or Failed on exception.
     *
     * @param isRemove  true -> removal; false -> addition.
     * @param ops       Handler pair resolved by Manager via the mapper.
     *                  Never null — Manager validates before constructing
     *                  CMObject.
     */
    sdbusplus::async::task<> execute(bool isRemove, const FRUOperations& ops);

    /**
     * @brief Update the Progress interface status on D-Bus.
     */
    void updateStatus(OperationStatus status);

    /** @brief Return the current Progress status (for testing). */
    OperationStatus getStatus() const
    {
        return this->status();
    }

  private:
    sdbusplus::async::context& ctx;
    const std::string objectPath;
    const std::string fruPath;
};

} // namespace concurrent_maintenance
