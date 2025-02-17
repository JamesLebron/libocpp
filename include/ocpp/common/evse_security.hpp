// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_COMMON_EVSE_SECURITY
#define OCPP_COMMON_EVSE_SECURITY

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include <ocpp/common/types.hpp>
#include <ocpp/v201/ocpp_types.hpp>

namespace ocpp {

/// \brief Handler for security related operations of the charging station
class EvseSecurity {

public:
    /// \brief Installs the CA \p certificate for the given \p certificate_type . This function respects the
    /// requirements of OCPP specified for the CSMS initiated message InstallCertificate.req .
    /// \param certificate PEM formatted CA certificate
    /// \param certificate_type specifies the CA certificate type
    /// \return result of the operation
    virtual InstallCertificateResult install_ca_certificate(const std::string& certificate,
                                                            const CaCertificateType& certificate_type) = 0;

    /// \brief Deletes the certificate specified by \p certificate_hash_data . This function respects the
    /// requirements of OCPP specified for the CSMS initiated message DeleteCertificate.req
    /// \param certificate_hash_data specifies the certificate to be deleted
    /// \return result of the operation
    virtual DeleteCertificateResult delete_certificate(const CertificateHashDataType& certificate_hash_data) = 0;

    /// \brief Verifies the given \p certificate_chain for the given \p certificate_type using the respective CA
    /// certificates for the leaf and if valid installs the certificate. Before installing the certificate, this
    /// function checks if a private key is present for the given certificate. This function respects the
    /// requirements of OCPP specified for the CSMS initiated message CertificateSigned.req .
    /// \param certificate_chain PEM formatted certificate or certificate chain
    /// \param certificate_type type of the leaf certificate
    /// \return result of the operation
    virtual InstallCertificateResult update_leaf_certificate(const std::string& certificate_chain,
                                                             const CertificateSigningUseEnum& certificate_type) = 0;

    /// \brief Verifies the given \p certificate_chain for the given \p certificate_type against the respective CA
    /// certificates for the leaf according to the requirements specified in OCPP.
    /// \param certificate_chain PEM formatted certificate or certificate chain
    /// \param certificate_type type of the leaf certificate
    /// \return result of the operation
    virtual InstallCertificateResult verify_certificate(const std::string& certificate_chain,
                                                        const CertificateSigningUseEnum& certificate_type) = 0;

    /// \brief Retrieves all certificates installed on the filesystem applying the \p certificate_types filter. This
    /// function respects the requirements of OCPP specified for the CSMS initiated message
    /// GetInstalledCertificateIds.req .
    /// \param certificate_types
    /// \return contains the certificate hash data chains of the requested \p certificate_types
    virtual std::vector<CertificateHashDataChain>
    get_installed_certificates(const std::vector<CertificateType>& certificate_types) = 0;

    /// \brief Retrieves the OCSP request data of the V2G certificates. This function respects the requirements of OCPP
    /// specified for the CSMS initiated message GetCertificateStatus.req .
    /// \return contains OCSP request data
    virtual std::vector<OCSPRequestData> get_ocsp_request_data() = 0;

    /// \brief Updates the OCSP cache for the given \p certificate_hash_data with the given \p ocsp_response
    /// \param certificate_hash_data identifies the certificate for which the \p ocsp_response is specified
    /// \param ocsp_response the actual OCSP data
    virtual void update_ocsp_cache(const CertificateHashDataType& certificate_hash_data,
                                   const std::string& ocsp_response) = 0;

    /// \brief Indicates if a CA certificate for the given \p certificate_type is installed on the filesystem
    /// \param certificate_type
    /// \return true if CA certificate is present, else false
    virtual bool is_ca_certificate_installed(const CaCertificateType& certificate_type) = 0;

    /// \brief Generates a certificate signing request for the given \p certificate_type , \p country , \p organization
    /// and \p common . This function respects the requirements of OCPP specified for the CSMS initiated message
    /// SignCertificate.req .
    /// \param certificate_type
    /// \param country
    /// \param organization
    /// \param common
    /// \return the PEM formatted certificate signing request
    virtual std::string generate_certificate_signing_request(const CertificateSigningUseEnum& certificate_type,
                                                             const std::string& country,
                                                             const std::string& organization,
                                                             const std::string& common) = 0;

    /// \brief Searches the leaf certificate for the given \p certificate_type and retrieves the most recent certificate
    /// that is already valid and the respective key . If no certificate is present or no key is matching the
    /// certificate, this function returns std::nullopt
    /// \param certificate_type type of the leaf certificate
    /// \param encoding specifies PEM or DER format
    /// \return key pair of certificate and key if present, else std::nullopt
    virtual std::optional<KeyPair> get_key_pair(const CertificateSigningUseEnum& certificate_type) = 0;

    /// \brief Retrieves the PEM formatted CA bundle file for the given \p certificate_type
    /// \param certificate_type
    /// \return CA certificate file
    virtual std::string get_verify_file(const CaCertificateType& certificate_type) = 0;

    /// \brief Gets the expiry day count for the leaf certificate of the given \p certificate_type
    /// \param certificate_type
    /// \return day count until the leaf certificate expires
    virtual int get_leaf_expiry_days_count(const CertificateSigningUseEnum& certificate_type) = 0;
};

namespace evse_security_conversions {

/** Conversions for Plug&Charge Data Transfer **/

ocpp::v201::GetCertificateIdUseEnum to_ocpp_v201(ocpp::CertificateType other);
ocpp::v201::InstallCertificateUseEnum to_ocpp_v201(ocpp::CaCertificateType other);
ocpp::v201::CertificateSigningUseEnum to_ocpp_v201(ocpp::CertificateSigningUseEnum other);
ocpp::v201::HashAlgorithmEnum to_ocpp_v201(ocpp::HashAlgorithmEnumType other);
ocpp::v201::InstallCertificateStatusEnum to_ocpp_v201(ocpp::InstallCertificateResult other);
ocpp::v201::DeleteCertificateStatusEnum to_ocpp_v201(ocpp::DeleteCertificateResult other);

ocpp::v201::CertificateHashDataType to_ocpp_v201(ocpp::CertificateHashDataType other);
ocpp::v201::CertificateHashDataChain to_ocpp_v201(ocpp::CertificateHashDataChain other);
ocpp::v201::OCSPRequestData to_ocpp_v201(ocpp::OCSPRequestData other);

ocpp::CertificateType from_ocpp_v201(ocpp::v201::GetCertificateIdUseEnum other);
ocpp::CaCertificateType from_ocpp_v201(ocpp::v201::InstallCertificateUseEnum other);
ocpp::CertificateSigningUseEnum from_ocpp_v201(ocpp::v201::CertificateSigningUseEnum other);
ocpp::HashAlgorithmEnumType from_ocpp_v201(ocpp::v201::HashAlgorithmEnum other);
ocpp::InstallCertificateResult from_ocpp_v201(ocpp::v201::InstallCertificateStatusEnum other);
ocpp::DeleteCertificateResult from_ocpp_v201(ocpp::v201::DeleteCertificateStatusEnum other);

ocpp::CertificateHashDataType from_ocpp_v201(ocpp::v201::CertificateHashDataType other);
ocpp::CertificateHashDataChain from_ocpp_v201(ocpp::v201::CertificateHashDataChain other);
ocpp::OCSPRequestData from_ocpp_v201(ocpp::v201::OCSPRequestData other);

} // namespace evse_security_conversions

} // namespace ocpp

#endif // OCPP_COMMON_EVSE_SECURITY
