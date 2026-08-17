// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#pragma once

#include "cm_object.hpp"

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/task.hpp>

#include <memory>
#include <string>

namespace concurrent_maintenance
{

class Manager
{
  public:
    explicit Manager(sdbusplus::async::context& ctx);
    void start();

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    ~Manager() = default;

  private:
    sdbusplus::async::context& ctx;

    // Current CM object (nullptr when idle)
    std::unique_ptr<CMObject> currentCMObject;

    // Coroutine that watches for ReadyToRemove property changes
    sdbusplus::async::task<> watchReadyToRemove();

    // Create/remove CM object based on ReadyToRemove value
    void manageCMObject(bool readyToRemove, const std::string& inventoryPath);

    // Allow unit tests to access private members
    friend class ManagerTest;
};

} // namespace concurrent_maintenance
