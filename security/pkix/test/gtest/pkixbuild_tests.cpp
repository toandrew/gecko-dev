/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This code is made available to you under your choice of the following sets
 * of licensing terms:
 */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/* Copyright 2013 Mozilla Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cert.h"
#include "nssgtest.h"
#include "pkix/pkix.h"
#include "pkix/pkixnss.h"
#include "pkixgtest.h"
#include "pkixtestutil.h"
#include "prinit.h"
#include "secerr.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

typedef ScopedPtr<CERTCertificate, CERT_DestroyCertificate>
          ScopedCERTCertificate;
typedef ScopedPtr<CERTCertList, CERT_DestroyCertList> ScopedCERTCertList;

// The result is owned by the arena
static Input
CreateCert(PLArenaPool* arena, const char* issuerStr,
           const char* subjectStr, EndEntityOrCA endEntityOrCA,
           /*optional*/ SECKEYPrivateKey* issuerKey,
           /*out*/ ScopedSECKEYPrivateKey& subjectKey,
           /*out*/ ScopedCERTCertificate* subjectCert = nullptr)
{
  static long serialNumberValue = 0;
  ++serialNumberValue;
  const SECItem* serialNumber(CreateEncodedSerialNumber(arena,
                                                        serialNumberValue));
  EXPECT_TRUE(serialNumber);
  const SECItem* issuerDER(ASCIIToDERName(arena, issuerStr));
  EXPECT_TRUE(issuerDER);
  const SECItem* subjectDER(ASCIIToDERName(arena, subjectStr));
  EXPECT_TRUE(subjectDER);

  const SECItem* extensions[2] = { nullptr, nullptr };
  if (endEntityOrCA == EndEntityOrCA::MustBeCA) {
    extensions[0] =
      CreateEncodedBasicConstraints(arena, true, nullptr,
                                    ExtensionCriticality::Critical);
    EXPECT_TRUE(extensions[0]);
  }

  SECItem* certDER(CreateEncodedCertificate(
                     arena, v3, SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION,
                     serialNumber, issuerDER, oneDayBeforeNow, oneDayAfterNow,
                     subjectDER, extensions, issuerKey, SEC_OID_SHA256,
                     subjectKey));
  EXPECT_TRUE(certDER);
  if (subjectCert) {
    *subjectCert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), certDER,
                                           nullptr, false, true);
    EXPECT_TRUE(*subjectCert);
  }
  Input result;
  EXPECT_EQ(Success, result.Init(certDER->data, certDER->len));
  return result;
}

class TestTrustDomain : public TrustDomain
{
public:
  // The "cert chain tail" is a longish chain of certificates that is used by
  // all of the tests here. We share this chain across all the tests in order
  // to speed up the tests (generating keypairs for the certs is very slow).
  bool SetUpCertChainTail()
  {
    static char const* const names[] = {
        "CN=CA1 (Root)", "CN=CA2", "CN=CA3", "CN=CA4", "CN=CA5", "CN=CA6",
        "CN=CA7"
    };

    static_assert(MOZILLA_PKIX_ARRAY_LENGTH(names) ==
                    MOZILLA_PKIX_ARRAY_LENGTH(certChainTail),
                  "mismatch in sizes of names and certChainTail arrays");

    ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
    if (!arena) {
      return false;
    }

    for (size_t i = 0; i < MOZILLA_PKIX_ARRAY_LENGTH(names); ++i) {
      const char* issuerName = i == 0 ? names[0]
                                      : certChainTail[i - 1]->subjectName;
      (void) CreateCert(arena.get(), issuerName, names[i],
                 EndEntityOrCA::MustBeCA, leafCAKey.get(), leafCAKey,
                 &certChainTail[i]);
    }

    return true;
  }

private:
  virtual Result GetCertTrust(EndEntityOrCA, const CertPolicyId&,
                              Input candidateCert,
                              /*out*/ TrustLevel& trustLevel)
  {
    Input rootDER;
    Result rv = rootDER.Init(certChainTail[0]->derCert.data,
                             certChainTail[0]->derCert.len);
    EXPECT_EQ(Success, rv);
    if (InputsAreEqual(candidateCert, rootDER)) {
      trustLevel = TrustLevel::TrustAnchor;
    } else {
      trustLevel = TrustLevel::InheritsTrust;
    }
    return Success;
  }

  virtual Result FindIssuer(Input encodedIssuerName,
                            IssuerChecker& checker, Time time)
  {
    SECItem encodedIssuerNameSECItem =
      UnsafeMapInputToSECItem(encodedIssuerName);
    ScopedCERTCertList
      candidates(CERT_CreateSubjectCertList(nullptr, CERT_GetDefaultCertDB(),
                                            &encodedIssuerNameSECItem, 0,
                                            false));
    if (candidates) {
      for (CERTCertListNode* n = CERT_LIST_HEAD(candidates);
           !CERT_LIST_END(n, candidates); n = CERT_LIST_NEXT(n)) {
        bool keepGoing;
        Input derCert;
        Result rv = derCert.Init(n->cert->derCert.data, n->cert->derCert.len);
        EXPECT_EQ(Success, rv);
        if (rv != Success) {
          return rv;
        }
        rv = checker.Check(derCert, nullptr/*additionalNameConstraints*/,
                           keepGoing);
        if (rv != Success) {
          return rv;
        }
        if (!keepGoing) {
          break;
        }
      }
    }

    return Success;
  }

  virtual Result CheckRevocation(EndEntityOrCA, const CertID&, Time,
                                 /*optional*/ const Input*,
                                 /*optional*/ const Input*)
  {
    return Success;
  }

  virtual Result IsChainValid(const DERArray&)
  {
    return Success;
  }

  virtual Result VerifySignedData(const SignedDataWithSignature& signedData,
                                  Input subjectPublicKeyInfo)
  {
    return ::mozilla::pkix::VerifySignedData(signedData, subjectPublicKeyInfo,
                                             nullptr);
  }

  virtual Result DigestBuf(Input item, /*out*/ uint8_t *digestBuf,
                           size_t digestBufLen)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result CheckPublicKey(Input subjectPublicKeyInfo)
  {
    return ::mozilla::pkix::CheckPublicKey(subjectPublicKeyInfo);
  }

  // We hold references to CERTCertificates in the cert chain tail so that we
  // CERT_CreateSubjectCertList can find them.
  ScopedCERTCertificate certChainTail[7];

public:
  ScopedSECKEYPrivateKey leafCAKey;
  CERTCertificate* GetLeafCACert() const
  {
    return certChainTail[MOZILLA_PKIX_ARRAY_LENGTH(certChainTail) - 1].get();
  }
};

class pkixbuild : public NSSTest
{
public:
  static void SetUpTestCase()
  {
    NSSTest::SetUpTestCase();
    // Initialize the tail of the cert chains we'll be using once, to make the
    // tests run faster (generating the keys is slow).
    if (!trustDomain.SetUpCertChainTail()) {
      abort();
    }
  }

protected:
  static TestTrustDomain trustDomain;
};

/*static*/ TestTrustDomain pkixbuild::trustDomain;

TEST_F(pkixbuild, MaxAcceptableCertChainLength)
{
  {
    Input certDER;
    ASSERT_EQ(Success, certDER.Init(trustDomain.GetLeafCACert()->derCert.data,
                                    trustDomain.GetLeafCACert()->derCert.len));
    ASSERT_EQ(Success,
              BuildCertChain(trustDomain, certDER, Now(),
                             EndEntityOrCA::MustBeCA,
                             KeyUsage::noParticularKeyUsageRequired,
                             KeyPurposeId::id_kp_serverAuth,
                             CertPolicyId::anyPolicy,
                             nullptr/*stapledOCSPResponse*/));
  }

  {
    ScopedSECKEYPrivateKey privateKey;
    ScopedCERTCertificate cert;
    Input certDER(CreateCert(arena.get(),
                             trustDomain.GetLeafCACert()->subjectName,
                             "CN=Direct End-Entity",
                             EndEntityOrCA::MustBeEndEntity,
                             trustDomain.leafCAKey.get(), privateKey));
    ASSERT_EQ(Success,
              BuildCertChain(trustDomain, certDER, Now(),
                             EndEntityOrCA::MustBeEndEntity,
                             KeyUsage::noParticularKeyUsageRequired,
                             KeyPurposeId::id_kp_serverAuth,
                             CertPolicyId::anyPolicy,
                             nullptr/*stapledOCSPResponse*/));
  }
}

TEST_F(pkixbuild, BeyondMaxAcceptableCertChainLength)
{
  static char const* const caCertName = "CN=CA Too Far";
  ScopedSECKEYPrivateKey caPrivateKey;

  // We need a CERTCertificate for caCert so that the trustdomain's FindIssuer
  // method can find it through the NSS cert DB.
  ScopedCERTCertificate caCert;

  {
    Input cert(CreateCert(arena.get(),
                          trustDomain.GetLeafCACert()->subjectName,
                          caCertName, EndEntityOrCA::MustBeCA,
                          trustDomain.leafCAKey.get(), caPrivateKey,
                          &caCert));
    ASSERT_EQ(Result::ERROR_UNKNOWN_ISSUER,
              BuildCertChain(trustDomain, cert, Now(),
                             EndEntityOrCA::MustBeCA,
                             KeyUsage::noParticularKeyUsageRequired,
                             KeyPurposeId::id_kp_serverAuth,
                             CertPolicyId::anyPolicy,
                             nullptr/*stapledOCSPResponse*/));
  }

  {
    ScopedSECKEYPrivateKey privateKey;
    Input cert(CreateCert(arena.get(), caCertName,
                          "CN=End-Entity Too Far",
                          EndEntityOrCA::MustBeEndEntity,
                          caPrivateKey.get(), privateKey));
    ASSERT_EQ(Result::ERROR_UNKNOWN_ISSUER,
              BuildCertChain(trustDomain, cert, Now(),
                             EndEntityOrCA::MustBeEndEntity,
                             KeyUsage::noParticularKeyUsageRequired,
                             KeyPurposeId::id_kp_serverAuth,
                             CertPolicyId::anyPolicy,
                             nullptr/*stapledOCSPResponse*/));
  }
}

// A TrustDomain that explicitly fails if CheckRevocation is called.
// It is initialized with the DER encoding of a root certificate that
// is treated as a trust anchor and is assumed to have issued all certificates
// (i.e. FindIssuer always attempts to build the next step in the chain with
// it).
class ExpiredCertTrustDomain : public TrustDomain
{
public:
  ExpiredCertTrustDomain(CERTCertificate* rootCert)
    : rootCert(rootCert)
  {
  }

  // The CertPolicyId argument is unused because we don't care about EV.
  virtual Result GetCertTrust(EndEntityOrCA, const CertPolicyId&,
                              Input candidateCert,
                              /*out*/ TrustLevel& trustLevel)
  {
    Input rootDER;
    Result rv = rootDER.Init(rootCert->derCert.data,
                             rootCert->derCert.len);
    EXPECT_EQ(Success, rv);
    if (InputsAreEqual(candidateCert, rootDER)) {
      trustLevel = TrustLevel::TrustAnchor;
    } else {
      trustLevel = TrustLevel::InheritsTrust;
    }
    return Success;
  }

  virtual Result FindIssuer(Input encodedIssuerName,
                            IssuerChecker& checker, Time time)
  {
    Input derCert;
    Result rv = derCert.Init(rootCert->derCert.data, rootCert->derCert.len);
    EXPECT_EQ(Success, rv);
    if (rv != Success) {
      return rv;
    }
    // keepGoing is an out parameter from IssuerChecker.Check. It would tell us
    // whether or not to continue attempting other potential issuers. We only
    // know of one potential issuer, however, so we ignore it.
    bool keepGoing;
    return checker.Check(derCert, nullptr, keepGoing);
  }

  virtual Result CheckRevocation(EndEntityOrCA, const CertID&, Time,
                                 /*optional*/ const Input*,
                                 /*optional*/ const Input*)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result IsChainValid(const DERArray&)
  {
    return Success;
  }

  virtual Result VerifySignedData(const SignedDataWithSignature& signedData,
                                  Input subjectPublicKeyInfo)
  {
    return ::mozilla::pkix::VerifySignedData(signedData, subjectPublicKeyInfo,
                                             nullptr);
  }

  virtual Result DigestBuf(Input, uint8_t*, size_t)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result CheckPublicKey(Input subjectPublicKeyInfo)
  {
    return ::mozilla::pkix::CheckPublicKey(subjectPublicKeyInfo);
  }

private:
  ScopedCERTCertificate rootCert;
};

TEST_F(pkixbuild, NoRevocationCheckingForExpiredCert)
{
  ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
  ASSERT_TRUE(arena.get());

  const char* rootCN = "CN=Root CA";
  ScopedSECKEYPrivateKey rootKey;
  ScopedCERTCertificate rootCert;
  (void) CreateCert(arena.get(), rootCN, rootCN, EndEntityOrCA::MustBeCA,
                    nullptr, rootKey, &rootCert);
  ExpiredCertTrustDomain expiredCertTrustDomain(rootCert.release());

  const SECItem* serialNumber(CreateEncodedSerialNumber(arena.get(), 100));
  EXPECT_TRUE(serialNumber);
  const SECItem* issuerDER(ASCIIToDERName(arena.get(), rootCN));
  EXPECT_TRUE(issuerDER);
  const SECItem* subjectDER(ASCIIToDERName(arena.get(), "CN=Expired End-Entity Cert"));
  EXPECT_TRUE(subjectDER);
  ScopedSECKEYPrivateKey unusedSubjectKey;
  SECItem* certDER(CreateEncodedCertificate(
                     arena.get(), v3,
                     SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION,
                     serialNumber, issuerDER,
                     oneDayBeforeNow - Time::ONE_DAY_IN_SECONDS,
                     oneDayBeforeNow,
                     subjectDER, nullptr, rootKey.get(),
                     SEC_OID_SHA256,
                     unusedSubjectKey));
  EXPECT_TRUE(certDER);
  Input certInput;
  EXPECT_EQ(Success, certInput.Init(certDER->data, certDER->len));
  ASSERT_EQ(Result::ERROR_EXPIRED_CERTIFICATE,
            BuildCertChain(expiredCertTrustDomain, certInput,
                           Now(), EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::id_kp_serverAuth,
                           CertPolicyId::anyPolicy,
                           nullptr));
}
