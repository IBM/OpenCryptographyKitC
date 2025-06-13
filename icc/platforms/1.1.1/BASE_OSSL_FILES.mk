
#
# Extra code needed to maintain API compatibility with older ICC's
# we used to patch OpenSSL to do this, but now everything resides
# in the one shared lib this is easier maintenance
#
OSSL_XTRA_OBJ = aes_gcm$(OBJSUFX) aes_ccm$(OBJSUFX)

aes_gcm$(OBJSUFX): platforms/$(OPENSSL_LIBVER)/API/aes_gcm.c platforms/$(OPENSSL_LIBVER)/API/aes_gcm.h platforms/$(OPENSSL_LIBVER)/API/aes_ccm.h
	$(CC) $(CFLAGS) -I./ -Iplatforms/$(OPENSSL_LIBVER)/API -I$(OSSLINC_DIR) platforms/$(OPENSSL_LIBVER)/API/aes_gcm.c $(OUT)$@

aes_ccm$(OBJSUFX): platforms/$(OPENSSL_LIBVER)/API/aes_ccm.c platforms/$(OPENSSL_LIBVER)/API/aes_ccm.h platforms/$(OPENSSL_LIBVER)/API/aes_gcm.h
	$(CC) $(CFLAGS) -I./ -Iplatforms/$(OPENSSL_LIBVER)/API -I$(OSSLINC_DIR) platforms/$(OPENSSL_LIBVER)/API/aes_ccm.c $(OUT)$@


#
# Set of objects we link with the static OpenSSL crypto lib
# to produce the openssl comand line executable
#
E_OBJ = $(APP_DIR)/verify$(OBJSUFX)  \
	$(APP_DIR)/asn1pars$(OBJSUFX) \
	$(APP_DIR)/req$(OBJSUFX) \
	$(APP_DIR)/dgst$(OBJSUFX)  \
	$(APP_DIR)/dhparam$(OBJSUFX)  \
	$(APP_DIR)/enc$(OBJSUFX)  \
	$(APP_DIR)/passwd$(OBJSUFX)  \
	$(APP_DIR)/errstr$(OBJSUFX)  \
	$(APP_DIR)/ca$(OBJSUFX)  \
	$(APP_DIR)/pkcs7$(OBJSUFX) \
	$(APP_DIR)/crl2p7$(OBJSUFX)  \
	$(APP_DIR)/crl$(OBJSUFX) \
	$(APP_DIR)/rsa$(OBJSUFX) \
	$(APP_DIR)/rsautl$(OBJSUFX)  \
	$(APP_DIR)/dsa$(OBJSUFX)  \
	$(APP_DIR)/dsaparam$(OBJSUFX)  \
	$(APP_DIR)/ec$(OBJSUFX)  \
	$(APP_DIR)/ecparam$(OBJSUFX) \
	$(APP_DIR)/x509$(OBJSUFX)  \
	$(APP_DIR)/genrsa$(OBJSUFX)  \
	$(APP_DIR)/gendsa$(OBJSUFX)  \
	$(APP_DIR)/s_server$(OBJSUFX)  \
	$(APP_DIR)/s_client$(OBJSUFX)  \
	$(APP_DIR)/speed$(OBJSUFX)  \
	$(APP_DIR)/s_time$(OBJSUFX)  \
	$(A_OBJ)  \
	$(S_OBJ)  \
	$(RAND_OBJ)  \
	$(APP_DIR)/version$(OBJSUFX)  \
	$(APP_DIR)/sess_id$(OBJSUFX) \
	$(APP_DIR)/ciphers$(OBJSUFX)  \
	$(APP_DIR)/nseq$(OBJSUFX)  \
	$(APP_DIR)/pkcs12$(OBJSUFX)  \
	$(APP_DIR)/pkcs8$(OBJSUFX)  \
	$(APP_DIR)/spkac$(OBJSUFX)  \
	$(APP_DIR)/smime$(OBJSUFX)  \
	$(APP_DIR)/storeutl$(OBJSUFX) \
	$(APP_DIR)/rand$(OBJSUFX)  \
	$(APP_DIR)/ocsp$(OBJSUFX)  \
	$(APP_DIR)/prime$(OBJSUFX) \
	$(APP_DIR)/genpkey$(OBJSUFX) \
	$(APP_DIR)/cms$(OBJSUFX) \
	$(APP_DIR)/pkeyparam$(OBJSUFX) \
	$(APP_DIR)/srp$(OBJSUFX) \
	$(APP_DIR)/ts$(OBJSUFX) \
	$(APP_DIR)/pkey$(OBJSUFX) \
	$(APP_DIR)/pkeyutl$(OBJSUFX) \
	$(APP_DIR)/opt$(OBJSUFX) \
	$(APP_DIR)/rehash$(OBJSUFX)


#	$(APP_DIR)/dh$(OBJSUFX)  \
#	$(APP_DIR)/gendh$(OBJSUFX)  \


#SHA3_OBJ1 = \
#   m_sha3$(OBJSUFX) KeccakF-1600-reference$(OBJSUFX) KeccakNISTInterface$(OBJSUFX) \
#   KeccakSponge$(OBJSUFX) sha3$(OBJSUFX)


#
# Set of common objects for the OpenSSL crypto library
# We strip out objects that are never called.
#
# What's missing here is anything that's platform specific assembler
#

BASE_OBJS = \
   a_bitstr$(OBJSUFX) a_d2i_fp$(OBJSUFX) a_digest$(OBJSUFX) a_dup$(OBJSUFX) a_gentm$(OBJSUFX) \
   a_i2d_fp$(OBJSUFX) a_int$(OBJSUFX) a_mbstr$(OBJSUFX) a_object$(OBJSUFX) a_octet$(OBJSUFX) a_print$(OBJSUFX) a_sign$(OBJSUFX) \
   a_strex$(OBJSUFX) a_strnid$(OBJSUFX) a_time$(OBJSUFX) a_type$(OBJSUFX) a_utctm$(OBJSUFX) a_utf8$(OBJSUFX) a_verify$(OBJSUFX) \
   aes_misc$(OBJSUFX) aes_wrap$(OBJSUFX) ameth_lib$(OBJSUFX) asn1_err$(OBJSUFX) asn1_gen$(OBJSUFX) asn1_lib$(OBJSUFX) \
   asn1_par$(OBJSUFX) asn_mime$(OBJSUFX) asn_pack$(OBJSUFX) b_dump$(OBJSUFX) b_print$(OBJSUFX) bf_buff$(OBJSUFX) bf_cfb64$(OBJSUFX) bf_ecb$(OBJSUFX) \
   bf_ofb64$(OBJSUFX) bf_skey$(OBJSUFX) bio_asn1$(OBJSUFX) bio_b64$(OBJSUFX) bio_enc$(OBJSUFX) bio_err$(OBJSUFX) bio_lib$(OBJSUFX) \
   bio_md$(OBJSUFX) bio_ndef$(OBJSUFX) bn_add$(OBJSUFX) bn_blind$(OBJSUFX) bn_ctx$(OBJSUFX) bn_div$(OBJSUFX) bn_err$(OBJSUFX) bn_exp$(OBJSUFX) \
   bn_exp2$(OBJSUFX) bn_gcd$(OBJSUFX) bn_gf2m$(OBJSUFX) bn_kron$(OBJSUFX) bn_lib$(OBJSUFX) bn_mod$(OBJSUFX) bn_mont$(OBJSUFX) bn_mul$(OBJSUFX) \
   bn_prime$(OBJSUFX) bn_x931p$(OBJSUFX) \
	bn_print$(OBJSUFX) bn_rand$(OBJSUFX) bn_recp$(OBJSUFX) bn_shift$(OBJSUFX) bn_sqr$(OBJSUFX) bn_sqrt$(OBJSUFX) bn_word$(OBJSUFX) \
   bss_file$(OBJSUFX) bss_mem$(OBJSUFX) bss_null$(OBJSUFX) buf_err$(OBJSUFX) buffer$(OBJSUFX) \
   c_allc$(OBJSUFX) c_alld$(OBJSUFX) c_cfb64$(OBJSUFX) c_ecb$(OBJSUFX) c_ofb64$(OBJSUFX) c_skey$(OBJSUFX)  \
   cfb64ede$(OBJSUFX) cfb64enc$(OBJSUFX) cfb_enc$(OBJSUFX) cm_ameth$(OBJSUFX) cm_pmeth$(OBJSUFX) cmac$(OBJSUFX) \
   cmll_cfb$(OBJSUFX) cmll_ecb$(OBJSUFX) cmll_ofb$(OBJSUFX) cms_asn1$(OBJSUFX) cms_att$(OBJSUFX) \
   cms_dd$(OBJSUFX) cms_enc$(OBJSUFX) cms_env$(OBJSUFX) cms_err$(OBJSUFX) cms_io$(OBJSUFX) cms_lib$(OBJSUFX) cms_pwri$(OBJSUFX) cms_sd$(OBJSUFX) \
   comp_err$(OBJSUFX) conf_api$(OBJSUFX) conf_def$(OBJSUFX) conf_err$(OBJSUFX) conf_lib$(OBJSUFX) conf_mod$(OBJSUFX) cpt_err$(OBJSUFX) cryptlib$(OBJSUFX) \
   d2i_pr$(OBJSUFX) d2i_pu$(OBJSUFX)  dh_meth$(OBJSUFX) dh_ameth$(OBJSUFX) dh_asn1$(OBJSUFX) dh_check$(OBJSUFX) dh_depr$(OBJSUFX) \
   dh_err$(OBJSUFX) dh_gen$(OBJSUFX) dh_key$(OBJSUFX) dh_lib$(OBJSUFX) dh_pmeth$(OBJSUFX) digest$(OBJSUFX) dsa_ameth$(OBJSUFX) dsa_asn1$(OBJSUFX) \
   dsa_depr$(OBJSUFX) dsa_err$(OBJSUFX) dsa_gen$(OBJSUFX) dsa_key$(OBJSUFX) dsa_lib$(OBJSUFX) dsa_ossl$(OBJSUFX) dsa_pmeth$(OBJSUFX) dsa_sign$(OBJSUFX) \
   dsa_vrf$(OBJSUFX) dso_dlfcn$(OBJSUFX) dso_err$(OBJSUFX) dso_lib$(OBJSUFX) dso_openssl$(OBJSUFX) e_aes$(OBJSUFX) e_aes_cbc_hmac_sha1$(OBJSUFX) e_bf$(OBJSUFX) \
   e_camellia$(OBJSUFX) e_cast$(OBJSUFX) e_des$(OBJSUFX) e_des3$(OBJSUFX) e_rc2$(OBJSUFX) e_rc4$(OBJSUFX) e_rc4_hmac_md5$(OBJSUFX) \
   e_xcbc_d$(OBJSUFX)  ec2_oct$(OBJSUFX) ec2_smpl$(OBJSUFX) ec_ameth$(OBJSUFX) ec_asn1$(OBJSUFX) ec_curve$(OBJSUFX) ec_cvt$(OBJSUFX) \
   ec_err$(OBJSUFX) ec_key$(OBJSUFX) ec_lib$(OBJSUFX) ec_mult$(OBJSUFX) ec_oct$(OBJSUFX) ec_pmeth$(OBJSUFX) ec_print$(OBJSUFX) ecb3_enc$(OBJSUFX) \
   ecb_enc$(OBJSUFX)  eck_prn$(OBJSUFX) ecp_mont$(OBJSUFX) ecp_oct$(OBJSUFX) \
   ecp_smpl$(OBJSUFX) encode$(OBJSUFX) \
   err$(OBJSUFX) err_all$(OBJSUFX) err_prn$(OBJSUFX) evp_asn1$(OBJSUFX) evp_enc$(OBJSUFX) evp_err$(OBJSUFX) evp_key$(OBJSUFX) evp_lib$(OBJSUFX) \
   evp_pbe$(OBJSUFX) evp_pkey$(OBJSUFX) ex_data$(OBJSUFX) f_int$(OBJSUFX) f_string$(OBJSUFX) hm_ameth$(OBJSUFX) \
   hm_pmeth$(OBJSUFX) hmac$(OBJSUFX) i2d_pr$(OBJSUFX) lhash$(OBJSUFX)  m_md2$(OBJSUFX) m_md4$(OBJSUFX) \
   m_md5$(OBJSUFX) m_mdc2$(OBJSUFX) m_ripemd$(OBJSUFX) m_sha1$(OBJSUFX) m_sigver$(OBJSUFX) m_wp$(OBJSUFX) md4_dgst$(OBJSUFX) \
    md2_dgst$(OBJSUFX) md2_one$(OBJSUFX) \
   md5_dgst$(OBJSUFX) mdc2dgst$(OBJSUFX) mem$(OBJSUFX) mem_dbg$(OBJSUFX) names$(OBJSUFX) o_init$(OBJSUFX) \
   stack$(OBJSUFX) \
   o_names$(OBJSUFX) o_time$(OBJSUFX) obj_dat$(OBJSUFX) obj_err$(OBJSUFX) obj_lib$(OBJSUFX) obj_xref$(OBJSUFX) ocsp_asn$(OBJSUFX) ocsp_err$(OBJSUFX) \
   ofb64ede$(OBJSUFX) ofb64enc$(OBJSUFX) p12_add$(OBJSUFX) p12_asn$(OBJSUFX) p12_crpt$(OBJSUFX) p12_decr$(OBJSUFX) p12_key$(OBJSUFX) \
   p12_p8d$(OBJSUFX) p12_p8e$(OBJSUFX) p12_utl$(OBJSUFX) p5_crpt$(OBJSUFX) p5_crpt2$(OBJSUFX) p5_pbe$(OBJSUFX) p5_pbev2$(OBJSUFX) p8_pkey$(OBJSUFX) \
   p_dec$(OBJSUFX) p_enc$(OBJSUFX) p_lib$(OBJSUFX) p_open$(OBJSUFX) p_seal$(OBJSUFX) p_sign$(OBJSUFX) p_verify$(OBJSUFX) pcy_cache$(OBJSUFX) \
   pcy_data$(OBJSUFX) pcy_lib$(OBJSUFX) pcy_map$(OBJSUFX) pcy_node$(OBJSUFX) pcy_tree$(OBJSUFX) pem_err$(OBJSUFX) pem_lib$(OBJSUFX) pem_oth$(OBJSUFX) \
   pk12err$(OBJSUFX) pk7_asn1$(OBJSUFX) pk7_attr$(OBJSUFX) pk7_doit$(OBJSUFX) pk7_lib$(OBJSUFX) pkcs7err$(OBJSUFX) pmeth_fn$(OBJSUFX) pmeth_gn$(OBJSUFX) \
   pmeth_lib$(OBJSUFX) rand_egd$(OBJSUFX) rand_err$(OBJSUFX) rand_key$(OBJSUFX) rand_lib$(OBJSUFX) rand_unix$(OBJSUFX) rc2_cbc$(OBJSUFX) rc2_ecb$(OBJSUFX) \
   rc2_skey$(OBJSUFX) rc2cfb64$(OBJSUFX) rc2ofb64$(OBJSUFX) rmd_dgst$(OBJSUFX) rsa_ameth$(OBJSUFX) \
   rsa_asn1$(OBJSUFX) rsa_chk$(OBJSUFX) rsa_crpt$(OBJSUFX) rsa_depr$(OBJSUFX) rsa_err$(OBJSUFX) rsa_gen$(OBJSUFX) rsa_lib$(OBJSUFX) \
   rsa_none$(OBJSUFX) rsa_oaep$(OBJSUFX) rsa_pk1$(OBJSUFX) rsa_pmeth$(OBJSUFX) rsa_pss$(OBJSUFX) rsa_saos$(OBJSUFX) rsa_sign$(OBJSUFX) rsa_ssl$(OBJSUFX) \
   rsa_x931$(OBJSUFX) rsa_x931g$(OBJSUFX) \
   set_key$(OBJSUFX)  \
   sha1dgst$(OBJSUFX)  sha256$(OBJSUFX) sha512$(OBJSUFX) t_pkey$(OBJSUFX) \
   t_x509$(OBJSUFX) tasn_dec$(OBJSUFX) tasn_enc$(OBJSUFX) tasn_fre$(OBJSUFX) tasn_new$(OBJSUFX) tasn_prn$(OBJSUFX) tasn_typ$(OBJSUFX) \
   tasn_utl$(OBJSUFX) ts_err$(OBJSUFX) ui_err$(OBJSUFX) ui_lib$(OBJSUFX) ui_openssl$(OBJSUFX)\
   v3_akeya$(OBJSUFX) v3_alt$(OBJSUFX) \
   v3_bcons$(OBJSUFX) v3_bitst$(OBJSUFX) v3_conf$(OBJSUFX) \
   v3_cpols$(OBJSUFX) v3_crld$(OBJSUFX) v3_enum$(OBJSUFX) \
   v3_extku$(OBJSUFX) v3_genn$(OBJSUFX) \
   v3_ia5$(OBJSUFX) v3_info$(OBJSUFX) v3_int$(OBJSUFX) \
   v3_lib$(OBJSUFX) v3_ncons$(OBJSUFX) v3_ocsp$(OBJSUFX) \
   v3_pci$(OBJSUFX) v3_pcia$(OBJSUFX) \
   v3_pcons$(OBJSUFX) v3_pku$(OBJSUFX) v3_pmaps$(OBJSUFX) \
   v3_prn$(OBJSUFX) v3_purp$(OBJSUFX) v3_skey$(OBJSUFX) \
   v3_sxnet$(OBJSUFX) v3_utl$(OBJSUFX) \
   x509_att$(OBJSUFX) x509_cmp$(OBJSUFX) x509_def$(OBJSUFX) \
   x509_err$(OBJSUFX) \
   x509_ext$(OBJSUFX) x509_lu$(OBJSUFX) x509_obj$(OBJSUFX) \
   x509_req$(OBJSUFX) x509_trs$(OBJSUFX) x509_v3$(OBJSUFX) \
   x509_vfy$(OBJSUFX) x509_vpm$(OBJSUFX) \
   x509name$(OBJSUFX) x509rset$(OBJSUFX)  x_algor$(OBJSUFX) \
   x_all$(OBJSUFX) x_attrib$(OBJSUFX) x_bignum$(OBJSUFX) x_crl$(OBJSUFX) x_exten$(OBJSUFX) x_long$(OBJSUFX) x_name$(OBJSUFX) x_pubkey$(OBJSUFX) \
   x_req$(OBJSUFX) x_sig$(OBJSUFX) x_spki$(OBJSUFX) x_val$(OBJSUFX) x_x509$(OBJSUFX) x_x509a$(OBJSUFX) xcbc_enc$(OBJSUFX) \
   cbc128$(OBJSUFX) cfb128$(OBJSUFX) cts128$(OBJSUFX) ofb128$(OBJSUFX) \
   ccm128$(OBJSUFX) ctr128$(OBJSUFX) gcm128$(OBJSUFX) xts128$(OBJSUFX) \
   cms_kari$(OBJSUFX) dh_kdf$(OBJSUFX) sha1_one$(OBJSUFX) \
   x509cset$(OBJSUFX) ocsp_ht$(OBJSUFX) \
   e_aes_cbc_hmac_sha256$(OBJSUFX)  wrap128$(OBJSUFX) \
   ecdsa_sign$(OBJSUFX) ecdsa_vrf$(OBJSUFX) ec_kmeth$(OBJSUFX) \
   o_str$(OBJSUFX) mem_sec$(OBJSUFX) e_chacha20_poly1305$(OBJSUFX) \
   m_md5_sha1$(OBJSUFX) m_blake2s$(OBJSUFX) m_blake2b$(OBJSUFX) x509_set$(OBJSUFX) \
   v3_asid$(OBJSUFX) ct_x509v3$(OBJSUFX) ct_err$(OBJSUFX) rsa_ossl$(OBJSUFX) \
   bn_intern$(OBJSUFX) tls1_prf$(OBJSUFX) hkdf$(OBJSUFX) \
   init$(OBJSUFX)  ct_oct$(OBJSUFX) ct_b64$(OBJSUFX) ct_sct$(OBJSUFX) \
   ct_sct_ctx$(OBJSUFX) ct_log$(OBJSUFX)  \
   conf_mall$(OBJSUFX) ct_vfy$(OBJSUFX) ct_prn$(OBJSUFX) \
   dh_rfc5114$(OBJSUFX) ocb128$(OBJSUFX) poly1305$(OBJSUFX) \
   ecdh_ossl$(OBJSUFX)  ecx_meth$(OBJSUFX) ecdsa_ossl$(OBJSUFX) \
   async_err$(OBJSUFX) kdf_err$(OBJSUFX) p5_scrypt$(OBJSUFX) \
   conf_sap$(OBJSUFX) async$(OBJSUFX) c_zlib$(OBJSUFX) \
   blake2b$(OBJSUFX) blake2s$(OBJSUFX) v3_addr$(OBJSUFX) \
   ecdh_kdf$(OBJSUFX) scrypt$(OBJSUFX) bn_dh$(OBJSUFX) \
   curve25519$(OBJSUFX) async_wait$(OBJSUFX) asn_moid$(OBJSUFX) \
   asn_mstbl$(OBJSUFX) evp_cnf$(OBJSUFX) v3err$(OBJSUFX) \
   v3_akey$(OBJSUFX) v3_tlsf$(OBJSUFX) rsa_meth$(OBJSUFX) \
   i2d_pu$(OBJSUFX) o_fopen$(OBJSUFX) ctype$(OBJSUFX) \
   poly1305_ameth$(OBJSUFX) poly1305_pmeth$(OBJSUFX) \
   siphash$(OBJSUFX) siphash_ameth$(OBJSUFX) siphash_pmeth$(OBJSUFX) \
   bio_meth$(OBJSUFX)  \
   sm2_crypt$(OBJSUFX) sm2_err$(OBJSUFX) sm2_sign$(OBJSUFX) sm2_pmeth$(OBJSUFX)  \
   sm3$(OBJSUFX) m_sm3$(OBJSUFX) \
   sm4$(OBJSUFX) e_sm4$(OBJSUFX) \
   aria$(OBJSUFX) e_aria$(OBJSUFX) \
   x_int64$(OBJSUFX) \
   o_dir$(OBJSUFX) \
   dh_rfc7919$(OBJSUFX) \
   ec_check$(OBJSUFX) \
   eddsa$(OBJSUFX) \
   drbg_lib$(OBJSUFX) drbg_ctr$(OBJSUFX) \
   loader_file$(OBJSUFX) store_err$(OBJSUFX) store_init$(OBJSUFX) \
   store_lib$(OBJSUFX) store_register$(OBJSUFX) store_strings$(OBJSUFX) \
   m_sha3$(OBJSUFX) \
   rsa_mp$(OBJSUFX) \
   pbe_scrypt$(OBJSUFX) \
   p12_mutl$(OBJSUFX) p12_kiss$(OBJSUFX) p12_sbag$(OBJSUFX) \
   p12_attr$(OBJSUFX) \
   curve448$(OBJSUFX) curve448_tables$(OBJSUFX) scalar$(OBJSUFX) \
   f_impl$(OBJSUFX) f_generic$(OBJSUFX) \
   uid$(OBJSUFX) \
   ui_null$(OBJSUFX) m_null$(OBJSUFX) \
   v3_admis$(OBJSUFX) \
   getenv$(OBJSUFX) \
   aes_core$(OBJSUFX) aes_cbc$(OBJSUFX) \
   o_fips$(OBJSUFX)


#   wp_dgst$(OBJSUFX)

#	seed$(OBJSUFX) seed_cbc$(OBJSUFX) seed_cfb$(OBJSUFX) seed_ecb$(OBJSUFX) seed_ofb$(OBJSUFX) \


# 407 objects



