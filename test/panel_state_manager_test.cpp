#include "panel_state_manager.hpp"
#include "transport.hpp"
#include "types.hpp"

#include <tuple>

#include <gtest/gtest.h>

using namespace panel::state::manager;
using namespace panel::types;
using namespace std;

// These test cases makes a presumption about the default value into the state
// manager via FunctionalityAttributes initialization in
// panel_state_manager.cpp.
// If values are changed there, modify the test cases accordingly.

// NOTE: Using "up", "down" and "enter" till we implement actual button events.
// Will be modified later.

// std::string lcdDevPath{};
// uint8_t lcdDevAddr = 0;
// getLcdDeviceData(lcdDevPath, lcdDevAddr, lcdObjPath, imValue);

// create transport lcd object
// auto lcdPanel = std::make_shared<panel::Transport>(
//     lcdDevPath, lcdDevAddr, panel::types::PanelType::LCD);
// lcdPanel->setTransportKey(false);
std::shared_ptr<panel::Transport> lcdPanel;

/*TEST(PanelStateManager, default_state)
{
    PanelStateManager stateMgr;
    stateMgr.resetPanelState();
    tuple<size_t, size_t> panelStateInfo = stateMgr.getPanelCurrentStateInfo();

    EXPECT_EQ(1, get<0>(panelStateInfo));
    EXPECT_EQ(0, get<1>(panelStateInfo));
}

TEST(PanelStateManager, increment_decremt)
{
    PanelStateManager stateMgr1, stateMgr, stateMgr2;
    stateMgr1.resetPanelState();
    stateMgr.resetPanelState();
    stateMgr2.resetPanelState();
    stateMgr1.processPanelButtonEvent(ButtonEvent::INCREMENT);
    tuple<size_t, size_t> panelStateInfo = stateMgr1.getPanelCurrentStateInfo();
    EXPECT_EQ(2, get<0>(panelStateInfo));
    EXPECT_EQ(0, get<1>(panelStateInfo));

    stateMgr.processPanelButtonEvent(ButtonEvent::INCREMENT);
    panelStateInfo = stateMgr.getPanelCurrentStateInfo();
    EXPECT_EQ(4, get<0>(panelStateInfo));
    EXPECT_EQ(0, get<1>(panelStateInfo));

    stateMgr2.processPanelButtonEvent(ButtonEvent::DECREMENT);
    panelStateInfo = stateMgr2.getPanelCurrentStateInfo();
    EXPECT_EQ(2, get<0>(panelStateInfo));
    EXPECT_EQ(0, get<1>(panelStateInfo));
}

TEST(PanelStateManager, increment_decrement_loop)
{
    PanelStateManager stateMgr;
    stateMgr.resetPanelState();
    // current state is 2, as per above test case
    stateMgr.processPanelButtonEvent(ButtonEvent::DECREMENT);
    stateMgr.processPanelButtonEvent(ButtonEvent::DECREMENT);
    tuple<size_t, size_t> panelStateInfo = stateMgr.getPanelCurrentStateInfo();
    EXPECT_EQ(64, get<0>(panelStateInfo));
    EXPECT_EQ(0, get<1>(panelStateInfo));

    stateMgr.processPanelButtonEvent(ButtonEvent::INCREMENT);
    panelStateInfo = stateMgr.getPanelCurrentStateInfo();
    EXPECT_EQ(1, get<0>(panelStateInfo));
    EXPECT_EQ(0, get<1>(panelStateInfo));
}*/

TEST(PanelStateManager, enter_exit_sub_range)
{
    PanelStateManager stateMgr(lcdPanel);
    stateMgr.processPanelButtonEvent(ButtonEvent::DECREMENT); // 64
    stateMgr.processPanelButtonEvent(ButtonEvent::EXECUTE);
    tuple<size_t, size_t> panelStateInfo = stateMgr.getPanelCurrentStateInfo();

    // 255 is the star method
    EXPECT_EQ(64, get<0>(panelStateInfo));
    EXPECT_EQ(255, get<1>(panelStateInfo));

    // enter at star will exit the sub range
    stateMgr.processPanelButtonEvent(ButtonEvent::EXECUTE);
    panelStateInfo = stateMgr.getPanelCurrentStateInfo();
    EXPECT_EQ(64, get<0>(panelStateInfo));
    EXPECT_EQ(0, get<1>(panelStateInfo));
}

/*TEST(PanelStateManager, enter_traverse_sub_range)
{
    PanelStateManager stateMgr;
    stateMgr.resetPanelState();
    // current state 64 as per above test case
    stateMgr.processPanelButtonEvent(ButtonEvent::EXECUTE);
    tuple<size_t, size_t> panelStateInfo = stateMgr.getPanelCurrentStateInfo();

    // 255 is the star method
    EXPECT_EQ(64, get<0>(panelStateInfo));
    EXPECT_EQ(255, get<1>(panelStateInfo));

    // enter at star will exit the sub range
    stateMgr.processPanelButtonEvent(ButtonEvent::INCREMENT);
    stateMgr.processPanelButtonEvent(ButtonEvent::INCREMENT);
    panelStateInfo = stateMgr.getPanelCurrentStateInfo();
    EXPECT_EQ(64, get<0>(panelStateInfo));
    EXPECT_EQ(1, get<1>(panelStateInfo));

    stateMgr.processPanelButtonEvent(ButtonEvent::DECREMENT); // 0
    stateMgr.processPanelButtonEvent(ButtonEvent::DECREMENT); // star
    stateMgr.processPanelButtonEvent(ButtonEvent::DECREMENT); // 24
    panelStateInfo = stateMgr.getPanelCurrentStateInfo();
    EXPECT_EQ(64, get<0>(panelStateInfo));
    EXPECT_EQ(24, get<1>(panelStateInfo));
}*/
