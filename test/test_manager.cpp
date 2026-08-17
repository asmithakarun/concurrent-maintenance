// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "manager.hpp"

#include <sdbusplus/async.hpp>

#include <gtest/gtest.h>

namespace concurrent_maintenance
{

template <typename F>
void runAsync(F func)
{
    sdbusplus::async::context ctx;
    ctx.spawn(func(ctx) |
              sdbusplus::async::execution::then([&]() { ctx.request_stop(); }));
    ctx.run();
}

class ManagerTest : public ::testing::Test
{
  protected:
    // Wrappers that the friend declaration grants access to, called from
    // test fixture methods — lambdas cannot use friend access directly.
    static void callManageCMObject(Manager& mgr, bool readyToRemove,
                                   const std::string& inventoryPath = "")
    {
        mgr.manageCMObject(readyToRemove, inventoryPath);
    }

    static const std::string& getCurrentCMObjectPath(Manager& mgr)
    {
        return mgr.currentCMObject->getPath();
    }

    static bool isCMObjectNull(Manager& mgr)
    {
        return mgr.currentCMObject == nullptr;
    }
};

TEST_F(ManagerTest, CanBeConstructed)
{
    runAsync([](sdbusplus::async::context& ctx) -> sdbusplus::async::task<> {
        EXPECT_NO_THROW({ Manager manager(ctx); });
        co_return;
    });
}

// While a CM is in progress manager must not create a second CMObject.
TEST_F(ManagerTest, SingleCMGuardRejectsSecondRequest)
{
    runAsync([](sdbusplus::async::context& ctx) -> sdbusplus::async::task<> {
        Manager manager(ctx);

        // First call — should create the CM object
        ManagerTest::callManageCMObject(manager, true);
        EXPECT_FALSE(ManagerTest::isCMObjectNull(manager));

        const std::string firstPath =
            ManagerTest::getCurrentCMObjectPath(manager);

        // Second call while CM is in progress — should be rejected
        ManagerTest::callManageCMObject(manager, false);

        // currentCMObject must still point to the original object
        EXPECT_FALSE(ManagerTest::isCMObjectNull(manager));
        EXPECT_EQ(ManagerTest::getCurrentCMObjectPath(manager), firstPath);

        co_return;
    });
}

} // namespace concurrent_maintenance
