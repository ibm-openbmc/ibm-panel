#include "types.hpp"

#include <libpldm/instance-id.h>
#include <libpldm/transport.h>
#include <libpldm/transport/af-mctp.h>
#include <libpldm/transport/mctp-demux.h>
#include <stdint.h>

#include <vector>

namespace panel
{
/**
 * @brief A class to implement Pldm related functionalities.
 */
class PldmFramework
{
  public:
    /* Deleted methods */
    PldmFramework(const PldmFramework&) = delete;
    PldmFramework& operator=(const PldmFramework&) = delete;
    PldmFramework(PldmFramework&&) = delete;
    PldmFramework& operator=(const PldmFramework&&) = delete;

    /**
     * @brief Construtor
     */
    PldmFramework()
    {
        int rc = pldm_instance_db_init_default(&pldmInstanceIdDb);
        if (rc)
        {
            throw std::system_category().default_error_condition(rc);
        }
    }

    /**
     * @brief Destructor
     */
    ~PldmFramework()
    {
        int rc = pldm_instance_db_destroy(pldmInstanceIdDb);
        if (rc)
        {
            std::cout << "pldm_instance_db_destroy failed, rc =" << rc << "\n";
        }
    }

    /**
     * @brief Send Panel Function to PHYP.
     * This api is used to send panel function number to phyp by fetching and
     * setting the corresponding effector.
     *
     * @param[in] funcNumber - Function number that needs to be sent to PHYP.
     */
    void sendPanelFunctionToPhyp(const types::FunctionNumber& funcNumber);

  private:
    // TODO: <https://github.com/ibm-openbmc/ibm-panel/issues/57>
    // use PLDM defined header file to refer following constants.
    /** Host mctp eid */
    static constexpr auto mctpEid = (types::Byte)9;

    /** @brief Hardcoded TID */
    using TerminusID = uint8_t;
    static constexpr TerminusID tid = mctpEid;

    /** @brief PLDM instance ID database object used to get instance IDs
     */
    pldm_instance_db* pldmInstanceIdDb = nullptr;

    /** pldm transport instance  */
    pldm_transport* pldmTransport = nullptr;

    union TransportImpl
    {
        pldm_transport_mctp_demux* mctpDemux;
        pldm_transport_af_mctp* afMctp;
    };

    TransportImpl impl;

    // Constants required for PLDM packet.
    static constexpr auto phypTerminusID = (types::Byte)208;
    static constexpr auto frontPanelBoardEntityId = (uint16_t)32837;
    static constexpr auto stateIdToEnablePanelFunc = (uint16_t)32778;

    /**
     * @brief An api to prepare "set effecter" request packet.
     * This api prepares the message packet that needs to be sent to the PHYP.
     *
     * @param[in] pdrs - Panel pdr data.
     * @param[in] instanceId - instance id which uniquely identifies the
     * requested message packet. This needs to be encoded in the message packet.
     * @param[in] function - function number that needs to be sent to PHYP.
     *
     * @return Returns a Pldm packet.
     */
    types::PldmPacket
        prepareSetEffecterReq(const types::PdrList& pdrs,
                              types::Byte instanceId,
                              const types::FunctionNumber& function);

    /**
     * @brief Fetch the Panel effecter state set from PDR.
     * This api fetches host effecter id, effecter count and effecter position
     * from the panel's PDR.
     * @param[in] pdrs - Panel PDR data.
     * @param[out] effecterId - effecter id retrieved from the PDR.
     * @param[out] effecterCount - effecter count retrieved from the PDR.
     * @param[out] panelEffecterPos - Position of panel effecter.
     */

    void fetchPanelEffecterStateSet(const types::PdrList& pdrs,
                                    uint16_t& effecterId,
                                    types::Byte& effecterCount,
                                    types::Byte& panelEffecterPos);

    /**
     * @brief Get instance ID
     * This api returns the instance id by making a dbus call to libpldm
     * api. Instance id is to uniquely identify a message packet.
     * @return pldm_instance_id_t instance id.
     */
    pldm_instance_id_t getInstanceID();

    /**
     * @brief Free PLDM requester instance id
     * @param[in] pdrs - Panel PDR data.
     */
    void freeInstanceID(pldm_instance_id_t instanceID);

    /**
     * @brief setup PLDM transport for sending and receiving messages
     *
     * @return file descriptor
     */
    int openPldm();

    /** @brief Opens the MCTP socket for sending and receiving messages.
     *
     */
    int openMctpDemuxTransport();

    /** @brief Opens the MCTP AF_MCTP for sending and receiving messages.
     *
     */
    int openAfMctpTransport();

    /** @brief Close the PLDM file */
    void pldmClose();

};
} // namespace panel
