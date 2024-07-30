
#include "pldm_fw.hpp"

#include "exception.hpp"
#include "utils.hpp"

#include <libpldm/entity.h>
#include <libpldm/platform.h>
#include <libpldm/pldm.h>
#include <libpldm/state_set.h>
#include <poll.h>

#include <iostream>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

namespace panel
{
pldm_instance_id_t PldmFramework::getInstanceID()
{
    pldm_instance_id_t instanceID = 0;

    auto rc = pldm_instance_id_alloc(pldmInstanceIdDb, tid, &instanceID);
    if (rc == -EAGAIN)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        rc = pldm_instance_id_alloc(pldmInstanceIdDb, tid, &instanceID);
    }

    if (rc)
    {
        std::cerr << "Return code = " << rc << std::endl;
        throw FunctionFailure("pldm: call to GetInstanceId failed.");
    }
    std::cout << "Got instanceId: " << static_cast<int>(instanceID)
              << " from PLDM eid: " << static_cast<int>(tid) << std::endl;
    return instanceID;
}

void PldmFramework::freeInstanceID(pldm_instance_id_t instanceID)
{
    auto rc = pldm_instance_id_free(pldmInstanceIdDb, tid, instanceID);
    if (rc)
    {
        std::cerr << "pldm_instance_id_free failed to free id = "
                  << static_cast<int>(instanceID)
                  << " of tid = " << static_cast<int>(tid) << " rc = " << rc
                  << std::endl;
    }
}

int PldmFramework::openPldm()
{
    auto fd = -1;
    if (pldmTransport)
    {
        std::cerr << "open: pldmTransport already setup!" << std::endl;
        throw FunctionFailure("Required host dump action via PLDM is not "
                              "allowed due to openPldm failed");
        return fd;
    }

#if defined(PLDM_TRANSPORT_WITH_MCTP_DEMUX)
    fd = openMctpDemuxTransport();
#elif defined(PLDM_TRANSPORT_WITH_AF_MCTP)
    fd = openAfMctpTransport();
#else
    std::cerr << "open: No valid transport defined!" << std::endl;
    throw FunctionFailure("Required host dump action via PLDM is not allowed: "
                          "No valid transport defined!");
#endif

    if (fd < 0)
    {
        auto e = errno;
        std::cerr << "openPldm failed, errno: " << e << ", FD: " << fd
                  << std::endl;
        throw FunctionFailure("Required host dump action via PLDM is not "
                              "allowed due to openPldm failed");
    }
    return fd;
}

[[maybe_unused]] int PldmFramework::openMctpDemuxTransport()
{
    impl.mctpDemux = nullptr;
    int rc = pldm_transport_mctp_demux_init(&impl.mctpDemux);
    if (rc)
    {
        std::cerr << "openMctpDemuxTransport: Failed to init MCTP demux "
                     "transport. rc = "
                  << rc << std::endl;
        throw FunctionFailure("Failed to init MCTP demux transport");
    }

    rc = pldm_transport_mctp_demux_map_tid(impl.mctpDemux, tid, tid);
    if (rc)
    {
        std::cerr << "openMctpDemuxTransport: Failed to setup tid to eid "
                     "mapping. rc = "
                  << rc << std::endl;
        pldmClose();
        throw FunctionFailure("Failed to setup tid to eid mapping");
    }
    pldmTransport = pldm_transport_mctp_demux_core(impl.mctpDemux);

    struct pollfd pollfd;
    rc = pldm_transport_mctp_demux_init_pollfd(pldmTransport, &pollfd);
    if (rc)
    {
        std::cerr << "openMctpDemuxTransport: Failed to get pollfd. rc = " << rc
                  << std::endl;
        pldmClose();
        throw FunctionFailure("Failed to get pollfd");
    }
    return pollfd.fd;
}

[[maybe_unused]] int PldmFramework::openAfMctpTransport()
{
    impl.afMctp = nullptr;
    int rc = pldm_transport_af_mctp_init(&impl.afMctp);
    if (rc)
    {
        std::cerr
            << "openAfMctpTransport: Failed to init AF MCTP transport. rc = "
            << rc << std::endl;
        throw FunctionFailure("Failed to init AF MCTP transport");
    }

    rc = pldm_transport_af_mctp_map_tid(impl.afMctp, tid, tid);
    if (rc)
    {
        std::cerr
            << "openAfMctpTransport: Failed to setup tid to eid mapping. rc = "
            << rc << std::endl;
        pldmClose();
        throw FunctionFailure("Failed to setup tid to eid mapping");
    }
    pldmTransport = pldm_transport_af_mctp_core(impl.afMctp);

    struct pollfd pollfd;
    rc = pldm_transport_af_mctp_init_pollfd(pldmTransport, &pollfd);
    if (rc)
    {
        std::cerr << "openAfMctpTransport: Failed to get pollfd. rc = " << rc
                  << std::endl;
        pldmClose();
        throw FunctionFailure("Failed to get pollfd");
    }
    return pollfd.fd;
}

void PldmFramework::pldmClose()
{
#if defined(PLDM_TRANSPORT_WITH_MCTP_DEMUX)
    pldm_transport_mctp_demux_destroy(impl.mctpDemux);
    impl.mctpDemux = nullptr;
#elif defined(PLDM_TRANSPORT_WITH_AF_MCTP)
    pldm_transport_af_mctp_destroy(impl.afMctp);
    impl.afMctp = nullptr;
#endif
    pldmTransport = nullptr;
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

    pldm_instance_id_t instance = getInstanceID();

    types::PldmPacket packet =
        prepareSetEffecterReq(pdrs, instance, funcNumber);

    if (packet.empty())
    {
        std::map<std::string, std::string> additionalData{};
        additionalData.emplace(
            "DESCRIPTION", "pldm:SetStateEffecterStates request message empty");
        utils::createPEL("com.ibm.Panel.Error.HostCommunicationError",
                         "xyz.openbmc_project.Logging.Entry.Level.Warning",
                         additionalData);
        return;
    }

    int fd = openPldm();
    if (fd == -1)
    {
        std::cerr << "openPldm() failed with error = " << strerror(errno)
                  << std::endl;
        std::map<std::string, std::string> additionalData{};
        additionalData.emplace("DESCRIPTION",
                               "pldm: Failed to connect to MCTP socket");
        additionalData.emplace("ERRNO:", strerror(errno));
        utils::createPEL("com.ibm.Panel.Error.HostCommunicationError",
                         "xyz.openbmc_project.Logging.Entry.Level.Warning",
                         additionalData);
        freeInstanceID(instance);
        return;
    }

    std::cout << "packet data sent to pldm: ";
    for (const auto i : packet)
    {
        std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)i
                  << " ";
    }
    std::cout << std::endl;

    pldm_tid_t pldmTID = static_cast<pldm_tid_t>(mctpEid);
    auto rc = pldm_transport_send_msg(pldmTransport, pldmTID, packet.data(),
                                      packet.size());

    freeInstanceID(instance);
    pldmClose();
    if (rc)
    {
        std::map<std::string, std::string> additionalData{};
        additionalData.emplace(
            "DESCRIPTION",
            "pldm: pldm_send failed for panel function trigger.");
        panel::utils::createPEL(
            "com.ibm.Panel.Error.HostCommunicationError",
            "xyz.openbmc_project.Logging.Entry.Level.Warning", additionalData);
    }
}
} // namespace panel
