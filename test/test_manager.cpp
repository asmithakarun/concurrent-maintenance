#include "manager.hpp"

#include <sdbusplus/async/context.hpp>

#include <gtest/gtest.h>

namespace concurrent_maintenance
{

TEST(ManagerTest, CanBeConstructed)
{
    sdbusplus::async::context ctx;

    EXPECT_NO_THROW({ Manager manager(ctx); });
}

} // namespace concurrent_maintenance
