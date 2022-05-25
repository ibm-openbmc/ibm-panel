#include "pldm_fw.hpp"

#include "exception.hpp"
#include "utils.hpp"

#include <libpldm/entity.h>
#include <libpldm/platform.h>
#include <libpldm/pldm.h>
#include <libpldm/state_set.h>

#include <iostream>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

namespace panel
{
types::Byte PldmFramework::getInstanceID()
{
    types::Byte instanceId = 0;
    auto bus = sdbusplus::bus::new_default();

    try
    {
        auto method = bus.new_method_call(
            "xyz.openbmc_project.PLDM", "/xyz/openbmc_project/pldm",
            "xyz.openbmc_project.PLDM.Requester", "GetInstanceId");
        method.append(mctpEid);
        auto reply = bus.call(method);
        reply.read(instanceId);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << e.what() << std::endl;
        throw FunctionFailure("pldm: call to GetInstanceId failed.");
    }
    return instanceId;
}

void PldmFramework::fetchPanelEffecterStateSet(const types::PdrList& pdrs,
                                               uint16_t& effecterId,
                                               types::Byte& effecterCount,
                                               types::Byte& panelEffecterPos)
{
    auto pdr =
        reinterpret_cast<const pldm_state_effecter_pdr*>(pdrs.front().data());

    auto possibleStates =
        reinterpret_cast<const state_effecter_possible_states*>(
            pdr->possible_states);

    for (auto offset = 0; offset < pdr->composite_effecter_count; offset++)
    {
        if (possibleStates->state_set_id == stateIdToEnablePanelFunc)
        {
            effecterId = pdr->effecter_id;
            effecterCount = pdr->composite_effecter_count;
            panelEffecterPos = offset;
            return;
        }
    }

    throw FunctionFailure(
        "State set ID to enable panel function could not be found in PDR.");
}

types::PldmPacket
    PldmFramework::prepareSetEffecterReq(const types::PdrList& pdrs,
                                         types::Byte instanceId,
                                         const types::FunctionNumber& function)
{
    types::Byte effecterCount = 0;
    types::Byte panelEffecterPos = 0;
    uint16_t effecterId = 0;

    fetchPanelEffecterStateSet(pdrs, effecterId, effecterCount,
                               panelEffecterPos);

    types::PldmPacket request(
        sizeof(pldm_msg_hdr) + sizeof(effecterId) + sizeof(effecterCount) +
        (effecterCount * sizeof(set_effecter_state_field)));

    auto requestMsg = reinterpret_cast<pldm_msg*>(request.data());

    std::vector<set_effecter_state_field> stateField;

    for (types::Byte pos = 0; panelEffecterPos <= effecterCount; pos++)
    {
        if (pos == panelEffecterPos)
        {
            stateField.emplace_back(
                set_effecter_state_field{PLDM_REQUEST_SET, function});
            break;
        }
        else
        {
            stateField.emplace_back(
                set_effecter_state_field{PLDM_NO_CHANGE, 0});
        }
    }

    int rc = encode_set_state_effecter_states_req(
        instanceId, effecterId, effecterCount, stateField.data(), requestMsg);

    if (rc != PLDM_SUCCESS)
    {
        std::cerr << "Return code = " << rc << std::endl;
        throw FunctionFailure(
            "pldm: encode set effecter states request returned error.");
    }
    return request;
}

void PldmFramework::sendPanelFunctionToPhyp(
    const types::FunctionNumber& funcNumber)
{
    types::PdrList pdrs =
        utils::getPDR(phypTerminusID, frontPanelBoardEntityId,
                      stateIdToEnablePanelFunc, "FindStateEffecterPDR");

    if (pdrs.empty())
    {
        std::map<std::string, std::string> additionalData{};
        additionalData.emplace("DESCRIPTION",
            "Empty PDR returned for panel entity id.");
        utils::createPEL("com.ibm.Panel.Error.HostCommunicationError",
                         "xyz.openbmc_project.Logging.Entry.Level.Warning",
                         additionalData);
        return;
    }

    types::Byte instance = getInstanceID();

    types::PldmPacket packet =
        prepareSetEffecterReq(pdrs, instance, funcNumber);

    if (packet.empty())
    {
        std::map<std::string, std::string> additionalData{};
        additionalData.emplace("DESCRIPTION",
            "pldm:SetStateEffecterStates request message empty");
        utils::createPEL("com.ibm.Panel.Error.HostCommunicationError",
                         "xyz.openbmc_project.Logging.Entry.Level.Warning",
                         additionalData);
        return;
    }

    int fd = pldm_open();
    if (fd == -1)
    {
        std::cerr << "pldm_open() failed with error = " << strerror(errno)
                  << std::endl;
        std::map<std::string, std::string> additionalData{};
        additionalData.emplace("DESCRIPTION",
            "pldm: Failed to connect to MCTP socket");
        additionalData.emplace("ERRNO:", strerror(errno));
        utils::createPEL("com.ibm.Panel.Error.HostCommunicationError",
                         "xyz.openbmc_project.Logging.Entry.Level.Warning",
                         additionalData);
        return;
    }

    std::cout << "packet data sent to pldm: ";
    for (const auto i : packet)
    {
        std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)i
                  << " ";
    }
    std::cout << std::endl;

    auto rc = pldm_send(mctpEid, fd, packet.data(), packet.size());

    if (close(fd) == -1)
    {
        std::cerr << "Close on File descriptor failed with error = "
                  << strerror(errno) << std::endl;
    }

    if (rc)
    {
        std::map<std::string, std::string> additionalData{};
        additionalData.emplace("DESCRIPTION",
            "pldm: pldm_send failed for panel function trigger.");
        panel::utils::createPEL("com.ibm.Panel.Error.HostCommunicationError",
                          "xyz.openbmc_project.Logging.Entry.Level.Warning",
                          additionalData);
    }
}
} // namespace panel
