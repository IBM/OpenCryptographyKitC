
ASM_OBJS = \
	x86_64cpuid.obj \
	x86_64-mont.obj \
	x86_64-mont5.obj \
	x86_64-gf2m.obj \
	md5-x86_64.obj \
	vpaes-x86_64.obj \
	aesni-x86_64.obj \
	aesni-gcm-x86_64.obj \
	aesni-mb-x86_64.obj \
	aesni-sha1-x86_64.obj \
	aesni-sha256-x86_64.obj \
	ghash-x86_64.obj \
	rc4-md5-x86_64.obj \
	rc4-x86_64.obj \
	cmll-x86_64.obj \
	sha1-x86_64.obj \
	sha1-mb-x86_64.obj \
	sha256-x86_64.obj \
	sha256-mb-x86_64.obj \
	sha512-x86_64.obj  \
	bf_enc.obj \
	des_enc.obj \
	c_enc.obj \
	cmll_misc.obj \
	bn_asm.obj \
	dso_win32.obj \
	rand_win.obj \
	rsaz-x86_64.obj \
	rsaz-avx2.obj \
	rsaz_exp.obj \
	ecp_nistz256.obj \
	ecp_nistz256-x86_64.obj \
	chacha-x86_64.obj \
	poly1305-x86_64.obj \
	threads_win.obj async_win.obj \
	x25519-x86_64.obj	\
	keccak1600-x86_64.obj \
	uplink.obj \
	uplink-x86_64.obj \


# For the .bat file to work we need to be running a cmd processor, not cygmin.
# alternative is to convert b64_VS2022.bat to .sh which will run on either.
# OPENSSL build wants to run on CMD

WIN64_VS2022_OSSLINC_DIR    = $(OSSL_DIR)/include
WIN64_VS2022_BUILD_OSSL     = platforms\$(OPENSSL_LIBVER)\b64_VS2022.bat $(OPENSSL_VER) $(OPENSSL_$(CONFIG)_FLAG)
WIN64_VS2022_CLEAN_OSSL     = rm $(OSSL_DIR)/*.dll; rm $(OSSL_DIR)/*.ilk ; rm $(OSSL_DIR)/*/*.obj ; rm $(OSSL_DIR)/*/*/*.obj ; find . -name \*.obj -type f -delete ; rm openssl.c


BUILD_OBJS = $(BASE_OBJS) $(ASM_OBJS) icc.res


include platforms/${OPENSSL_LIBVER}/WIN_like.mk

OBJ_D = $(OSSL_DIR)/tmp32dll
OBJ_MS = $(OSSL_DIR)/ms/

#
# Objects needed to build OpenSSL (from ntdll.mak)
#
#
# Cut paste & edit from ms/ntdll.mak, we need the list of objects for Windows
# 

SSLOBJ= \
	$(OBJ_D)/s3_lib.obj \
	$(OBJ_D)/s3_enc.obj  \
	$(OBJ_D)/s3_cbc.obj   \
	$(OBJ_D)/s3_msg.obj \
	$(OBJ_D)/t1_trce.obj  \
	$(OBJ_D)/t1_lib.obj $(OBJ_D)/t1_enc.obj  \
	$(OBJ_D)/d1_lib.obj $(OBJ_D)/d1_msg.obj \
	$(OBJ_D)/d1_srtp.obj \
	$(OBJ_D)/bio_ssl.obj  \
	$(OBJ_D)/ssl_asn1.obj \
	$(OBJ_D)/ssl_rsa.obj \
	$(OBJ_D)/ssl_lib.obj \
	$(OBJ_D)/ssl_sess.obj \
	$(OBJ_D)/ssl_conf.obj \
	$(OBJ_D)/ssl_init.obj \
	$(OBJ_D)/ssl_ciph.obj \
	$(OBJ_D)/ssl_cert.obj \
	$(OBJ_D)/ssl_txt.obj \
	$(OBJ_D)/ssl_stat.obj \
	$(OBJ_D)/ssl_mcnf.obj \
	$(OBJ_D)/ssl_utst.obj \
	$(OBJ_D)/ssl_err.obj \
	$(OBJ_D)/ssl3_record.obj \
	$(OBJ_D)/ssl3_buffer.obj \
	$(OBJ_D)/dtls1_bitmap.obj \
	$(OBJ_D)/statem_lib.obj \
	$(OBJ_D)/statem_dtls.obj \
	$(OBJ_D)/statem.obj \
	$(OBJ_D)/statem_clnt.obj \
	$(OBJ_D)/statem_srvr.obj \
	$(OBJ_D)/rec_layer_d1.obj \
	$(OBJ_D)/rec_layer_s3.obj \
	$(OBJ_D)/methods.obj \
	$(OBJ_D)/extensions.obj \
	$(OBJ_D)/extensions_cust.obj \
	$(OBJ_D)/extensions_clnt.obj \
	$(OBJ_D)/extensions_srvr.obj \
	$(OBJ_D)/packet.obj \
	$(OBJ_D)/tls13_enc.obj \
	$(OBJ_D)/ssl3_record_tls13.obj

WIN64_AMD_CRYPTOOBJ= \
	$(OBJ_D)/init.obj \
	$(OBJ_D)/t_x509.obj $(OBJ_D)/b_sock2.obj $(OBJ_D)/b_addr.obj \
	$(OBJ_D)/bn_srp.obj $(OBJ_D)/p12_sbag.obj \
	$(OBJ_D)/cryptlib.obj $(OBJ_D)/ct_x509v3.obj \
	$(OBJ_D)/ecx_meth.obj $(OBJ_D)/hkdf.obj \
	$(OBJ_D)/blake2b.obj $(OBJ_D)/blake2s.obj \
	$(OBJ_D)/m_blake2s.obj $(OBJ_D)/m_blake2b.obj \
	$(OBJ_D)/ocb128.obj $(OBJ_D)/tls1_prf.obj \
	$(OBJ_D)/m_sha3.obj \
	$(OBJ_D)/asn_mstbl.obj $(OBJ_D)/v3_tlsf.obj \
	$(OBJ_D)/p5_scrypt.obj $(OBJ_D)/scrypt.obj  \
	$(OBJ_D)/chacha-x86_64.obj $(OBJ_D)/e_chacha20_poly1305.obj \
	$(OBJ_D)/ct_err.obj $(OBJ_D)/ct_prn.obj $(OBJ_D)/ct_log.obj \
	$(OBJ_D)/ct_sct.obj $(OBJ_D)/ct_sct_ctx.obj $(OBJ_D)/ct_vfy.obj  \
	$(OBJ_D)/ct_b64.obj $(OBJ_D)/ct_policy.obj  $(OBJ_D)/ct_oct.obj \
	$(OBJ_D)/async_err.obj  $(OBJ_D)/kdf_err.obj \
	$(OBJ_D)/async.obj $(OBJ_D)/async_wait.obj $(OBJ_D)/threads_win.obj \
	$(OBJ_D)/async_win.obj $(OBJ_D)/o_fopen.obj\
	$(OBJ_D)/bn_intern.obj \
	$(OBJ_D)/ecdh_kdf.obj $(OBJ_D)/ecdh_ossl.obj $(OBJ_D)/ec_kmeth.obj \
	$(OBJ_D)/bn_dh.obj \
	$(OBJ_D)/poly1305.obj  \
	$(OBJ_D)/ecdsa_ossl.obj \
	$(OBJ_D)/curve25519.obj \
	$(OBJ_D)/ecdsa_vrf.obj $(OBJ_D)/ecdsa_sign.obj \
	$(OBJ_D)/tls_srp.obj \
	$(OBJ_D)/m_md5_sha1.obj $(OBJ_D)/rsa_ossl.obj \
	$(OBJ_D)/win32_init.obj \
	$(OBJ_D)/mem_sec.obj \
	$(OBJ_D)/mem.obj $(OBJ_D)/mem_dbg.obj $(OBJ_D)/cversion.obj \
	$(OBJ_D)/ex_data.obj $(OBJ_D)/cpt_err.obj $(OBJ_D)/ebcdic.obj \
	$(OBJ_D)/uid.obj $(OBJ_D)/o_time.obj $(OBJ_D)/o_str.obj \
	$(OBJ_D)/o_dir.obj $(OBJ_D)/o_fips.obj $(OBJ_D)/o_init.obj \
	$(OBJ_D)/x86_64cpuid.obj $(OBJ_D)/md2_dgst.obj \
	$(OBJ_D)/md2_one.obj $(OBJ_D)/md4_dgst.obj $(OBJ_D)/md4_one.obj \
	$(OBJ_D)/md5_dgst.obj $(OBJ_D)/md5_one.obj $(OBJ_D)/md5-x86_64.obj \
	$(OBJ_D)/sha1dgst.obj  \
	$(OBJ_D)/sha1_one.obj $(OBJ_D)/sha256.obj $(OBJ_D)/sha512.obj \
	$(OBJ_D)/mdc2dgst.obj \
	$(OBJ_D)/mdc2_one.obj $(OBJ_D)/hmac.obj $(OBJ_D)/hm_ameth.obj \
	$(OBJ_D)/hm_pmeth.obj $(OBJ_D)/cmac.obj $(OBJ_D)/cm_ameth.obj \
	$(OBJ_D)/cm_pmeth.obj $(OBJ_D)/rmd_dgst.obj $(OBJ_D)/rmd_one.obj \
	$(OBJ_D)/set_key.obj $(OBJ_D)/ecb_enc.obj $(OBJ_D)/cbc_enc.obj \
	$(OBJ_D)/ecb3_enc.obj $(OBJ_D)/cfb64enc.obj $(OBJ_D)/cfb64ede.obj \
	$(OBJ_D)/cfb_enc.obj $(OBJ_D)/ofb64ede.obj \
	$(OBJ_D)/ofb64enc.obj $(OBJ_D)/ofb_enc.obj \
	$(OBJ_D)/str2key.obj $(OBJ_D)/pcbc_enc.obj $(OBJ_D)/qud_cksm.obj \
	$(OBJ_D)/rand_key.obj $(OBJ_D)/des_enc.obj $(OBJ_D)/fcrypt_b.obj \
	$(OBJ_D)/fcrypt.obj $(OBJ_D)/xcbc_enc.obj \
	$(OBJ_D)/cbc_cksm.obj \
	$(OBJ_D)/rc2_ecb.obj \
	$(OBJ_D)/rc2_skey.obj $(OBJ_D)/rc2_cbc.obj $(OBJ_D)/rc2cfb64.obj \
	$(OBJ_D)/rc2ofb64.obj $(OBJ_D)/rc4-x86_64.obj $(OBJ_D)/rc4-md5-x86_64.obj \
	$(OBJ_D)/bf_skey.obj $(OBJ_D)/bf_ecb.obj \
	$(OBJ_D)/bf_enc.obj $(OBJ_D)/bf_cfb64.obj $(OBJ_D)/bf_ofb64.obj \
	$(OBJ_D)/c_skey.obj $(OBJ_D)/c_ecb.obj $(OBJ_D)/c_enc.obj \
	$(OBJ_D)/c_cfb64.obj $(OBJ_D)/c_ofb64.obj $(OBJ_D)/aes_misc.obj \
	$(OBJ_D)/aes_ecb.obj $(OBJ_D)/aes_cfb.obj $(OBJ_D)/aes_ofb.obj \
	$(OBJ_D)/aes_ige.obj $(OBJ_D)/aes_wrap.obj \
	$(OBJ_D)/vpaes-x86_64.obj $(OBJ_D)/aesni-x86_64.obj \
	$(OBJ_D)/aesni-sha1-x86_64.obj $(OBJ_D)/aesni-sha256-x86_64.obj $(OBJ_D)/aesni-mb-x86_64.obj \
	$(OBJ_D)/cmll_ecb.obj $(OBJ_D)/cmll_ofb.obj $(OBJ_D)/cmll_cfb.obj \
	$(OBJ_D)/cmll_ctr.obj $(OBJ_D)/cmll-x86_64.obj \
	$(OBJ_D)/cmll_misc.obj $(OBJ_D)/cbc128.obj $(OBJ_D)/ctr128.obj \
	$(OBJ_D)/cts128.obj $(OBJ_D)/cfb128.obj $(OBJ_D)/ofb128.obj \
	$(OBJ_D)/gcm128.obj $(OBJ_D)/ccm128.obj $(OBJ_D)/xts128.obj \
	$(OBJ_D)/wrap128.obj $(OBJ_D)/ghash-x86_64.obj $(OBJ_D)/aesni-gcm-x86_64.obj \
	$(OBJ_D)/bn_add.obj $(OBJ_D)/bn_div.obj $(OBJ_D)/bn_exp.obj \
	$(OBJ_D)/bn_lib.obj $(OBJ_D)/bn_ctx.obj $(OBJ_D)/bn_mul.obj \
	$(OBJ_D)/bn_mod.obj $(OBJ_D)/bn_print.obj $(OBJ_D)/bn_rand.obj \
	$(OBJ_D)/bn_shift.obj $(OBJ_D)/bn_word.obj $(OBJ_D)/bn_blind.obj \
	$(OBJ_D)/bn_kron.obj $(OBJ_D)/bn_sqrt.obj $(OBJ_D)/bn_gcd.obj \
	$(OBJ_D)/bn_prime.obj $(OBJ_D)/bn_err.obj $(OBJ_D)/bn_sqr.obj \
	$(OBJ_D)/bn_rsa_fips186_4$(OBJSUFX) \
	$(OBJ_D)/bn_asm.obj $(OBJ_D)/x86_64-mont.obj $(OBJ_D)/x86_64-mont5.obj \
	$(OBJ_D)/x86_64-gf2m.obj $(OBJ_D)/rsaz_exp.obj $(OBJ_D)/rsaz-x86_64.obj \
	$(OBJ_D)/rsaz-avx2.obj $(OBJ_D)/bn_recp.obj $(OBJ_D)/bn_mont.obj \
	$(OBJ_D)/bn_mpi.obj $(OBJ_D)/bn_exp2.obj $(OBJ_D)/bn_gf2m.obj \
	$(OBJ_D)/bn_nist.obj $(OBJ_D)/bn_depr.obj $(OBJ_D)/bn_const.obj \
	$(OBJ_D)/bn_x931p.obj $(OBJ_D)/rsa_gen.obj \
	$(OBJ_D)/rsa_lib.obj $(OBJ_D)/rsa_sign.obj $(OBJ_D)/rsa_saos.obj \
	$(OBJ_D)/rsa_err.obj $(OBJ_D)/rsa_pk1.obj $(OBJ_D)/rsa_ssl.obj \
	$(OBJ_D)/rsa_none.obj $(OBJ_D)/rsa_oaep.obj $(OBJ_D)/rsa_chk.obj \
	$(OBJ_D)/rsa_pss.obj $(OBJ_D)/rsa_x931.obj \
	$(OBJ_D)/rsa_asn1.obj $(OBJ_D)/rsa_depr.obj $(OBJ_D)/rsa_ameth.obj \
	$(OBJ_D)/rsa_prn.obj $(OBJ_D)/rsa_pmeth.obj $(OBJ_D)/rsa_crpt.obj \
	$(OBJ_D)/rsa_x931g.obj $(OBJ_D)/dsa_gen.obj $(OBJ_D)/dsa_key.obj \
	$(OBJ_D)/rsa_sp800_56b_check$(OBJSUFX) $(OBJ_D)/rsa_sp800_56b_gen$(OBJSUFX) \
	$(OBJ_D)/dsa_lib.obj $(OBJ_D)/dsa_asn1.obj $(OBJ_D)/dsa_vrf.obj \
	$(OBJ_D)/dsa_sign.obj $(OBJ_D)/dsa_err.obj $(OBJ_D)/dsa_ossl.obj \
	$(OBJ_D)/dsa_depr.obj $(OBJ_D)/dsa_ameth.obj $(OBJ_D)/dsa_pmeth.obj \
	$(OBJ_D)/dsa_prn.obj $(OBJ_D)/dso_dl.obj $(OBJ_D)/dso_dlfcn.obj \
	$(OBJ_D)/dso_err.obj $(OBJ_D)/dso_lib.obj \
	$(OBJ_D)/dso_openssl.obj $(OBJ_D)/dso_win32.obj $(OBJ_D)/dso_vms.obj \
	$(OBJ_D)/dh_asn1.obj $(OBJ_D)/dh_gen.obj \
	$(OBJ_D)/dh_key.obj $(OBJ_D)/dh_lib.obj $(OBJ_D)/dh_check.obj \
	$(OBJ_D)/dh_err.obj $(OBJ_D)/dh_depr.obj $(OBJ_D)/dh_ameth.obj \
	$(OBJ_D)/dh_pmeth.obj $(OBJ_D)/dh_prn.obj $(OBJ_D)/dh_rfc5114.obj \
	$(OBJ_D)/dh_kdf.obj $(OBJ_D)/ec_lib.obj $(OBJ_D)/ecp_smpl.obj \
	$(OBJ_D)/ecp_mont.obj $(OBJ_D)/ecp_nist.obj $(OBJ_D)/ec_cvt.obj \
	$(OBJ_D)/ec_mult.obj $(OBJ_D)/ec_err.obj $(OBJ_D)/ec_curve.obj \
	$(OBJ_D)/ec_check.obj $(OBJ_D)/ec_print.obj $(OBJ_D)/ec_asn1.obj \
	$(OBJ_D)/ec_key.obj $(OBJ_D)/ec2_smpl.obj  \
	$(OBJ_D)/ec_ameth.obj $(OBJ_D)/ec_pmeth.obj $(OBJ_D)/eck_prn.obj \
	$(OBJ_D)/ecp_nistp224.obj $(OBJ_D)/ecp_nistp256.obj $(OBJ_D)/ecp_nistp521.obj \
	$(OBJ_D)/ecp_nistputil.obj $(OBJ_D)/ecp_oct.obj $(OBJ_D)/ec2_oct.obj \
	$(OBJ_D)/ec_oct.obj $(OBJ_D)/ecp_nistz256.obj $(OBJ_D)/ecp_nistz256-x86_64.obj \
	$(OBJ_D)/buffer.obj \
	$(OBJ_D)/buf_err.obj $(OBJ_D)/bio_lib.obj \
	$(OBJ_D)/bio_cb.obj $(OBJ_D)/bio_err.obj $(OBJ_D)/bss_mem.obj \
	$(OBJ_D)/bss_null.obj $(OBJ_D)/bss_fd.obj $(OBJ_D)/bss_file.obj \
	$(OBJ_D)/bss_sock.obj $(OBJ_D)/bss_conn.obj $(OBJ_D)/bf_null.obj \
	$(OBJ_D)/bf_buff.obj $(OBJ_D)/b_print.obj $(OBJ_D)/b_dump.obj \
	$(OBJ_D)/b_sock.obj $(OBJ_D)/bss_acpt.obj $(OBJ_D)/bf_nbio.obj \
	$(OBJ_D)/bss_log.obj $(OBJ_D)/bss_bio.obj $(OBJ_D)/bss_dgram.obj \
	$(OBJ_D)/stack.obj $(OBJ_D)/lhash.obj $(OBJ_D)/lh_stats.obj \
	$(OBJ_D)/randfile.obj $(OBJ_D)/rand_lib.obj \
	$(OBJ_D)/rand_err.obj $(OBJ_D)/rand_egd.obj $(OBJ_D)/rand_win.obj \
	$(OBJ_D)/rand_unix.obj \
	$(OBJ_D)/err.obj $(OBJ_D)/err_all.obj $(OBJ_D)/err_prn.obj \
	$(OBJ_D)/o_names.obj $(OBJ_D)/obj_dat.obj $(OBJ_D)/obj_lib.obj \
	$(OBJ_D)/obj_err.obj $(OBJ_D)/obj_xref.obj $(OBJ_D)/encode.obj \
	$(OBJ_D)/digest.obj $(OBJ_D)/evp_enc.obj $(OBJ_D)/evp_key.obj \
	$(OBJ_D)/evp_cnf.obj $(OBJ_D)/e_des.obj \
	$(OBJ_D)/e_bf.obj $(OBJ_D)/e_idea.obj $(OBJ_D)/e_des3.obj \
	$(OBJ_D)/e_camellia.obj $(OBJ_D)/e_rc4.obj $(OBJ_D)/e_aes.obj \
	$(OBJ_D)/names.obj $(OBJ_D)/e_xcbc_d.obj $(OBJ_D)/e_rc2.obj \
	$(OBJ_D)/e_cast.obj $(OBJ_D)/e_rc5.obj $(OBJ_D)/m_null.obj \
	$(OBJ_D)/m_md2.obj $(OBJ_D)/m_md4.obj $(OBJ_D)/m_md5.obj \
	$(OBJ_D)/m_sha1.obj $(OBJ_D)/m_wp.obj \
	$(OBJ_D)/m_mdc2.obj \
	$(OBJ_D)/m_ripemd.obj $(OBJ_D)/p_open.obj \
	$(OBJ_D)/p_seal.obj $(OBJ_D)/p_sign.obj $(OBJ_D)/p_verify.obj \
	$(OBJ_D)/p_lib.obj $(OBJ_D)/p_enc.obj $(OBJ_D)/p_dec.obj \
	$(OBJ_D)/bio_md.obj $(OBJ_D)/bio_b64.obj $(OBJ_D)/bio_enc.obj \
	$(OBJ_D)/evp_err.obj $(OBJ_D)/e_null.obj \
	$(OBJ_D)/c_allc.obj $(OBJ_D)/c_alld.obj $(OBJ_D)/evp_lib.obj \
	$(OBJ_D)/bio_ok.obj $(OBJ_D)/evp_pkey.obj $(OBJ_D)/evp_pbe.obj \
	$(OBJ_D)/p5_crpt.obj $(OBJ_D)/p5_crpt2.obj $(OBJ_D)/e_old.obj \
	$(OBJ_D)/pmeth_lib.obj $(OBJ_D)/pmeth_fn.obj $(OBJ_D)/pmeth_gn.obj \
	$(OBJ_D)/m_sigver.obj $(OBJ_D)/e_aes_cbc_hmac_sha1.obj $(OBJ_D)/e_aes_cbc_hmac_sha256.obj \
	$(OBJ_D)/e_rc4_hmac_md5.obj $(OBJ_D)/a_object.obj $(OBJ_D)/a_bitstr.obj \
	$(OBJ_D)/a_utctm.obj $(OBJ_D)/a_gentm.obj $(OBJ_D)/a_time.obj \
	$(OBJ_D)/a_int.obj $(OBJ_D)/a_octet.obj $(OBJ_D)/a_print.obj \
	$(OBJ_D)/a_type.obj  $(OBJ_D)/a_dup.obj \
	$(OBJ_D)/a_d2i_fp.obj $(OBJ_D)/a_i2d_fp.obj  \
	$(OBJ_D)/a_utf8.obj $(OBJ_D)/a_sign.obj $(OBJ_D)/a_digest.obj \
	$(OBJ_D)/a_verify.obj $(OBJ_D)/a_mbstr.obj $(OBJ_D)/a_strex.obj \
	$(OBJ_D)/x_algor.obj $(OBJ_D)/x_val.obj $(OBJ_D)/x_pubkey.obj \
	$(OBJ_D)/x_sig.obj $(OBJ_D)/x_req.obj $(OBJ_D)/x_attrib.obj \
	$(OBJ_D)/x_bignum.obj $(OBJ_D)/x_long.obj $(OBJ_D)/x_name.obj \
	$(OBJ_D)/x_x509.obj $(OBJ_D)/x_x509a.obj $(OBJ_D)/x_crl.obj \
	$(OBJ_D)/x_info.obj $(OBJ_D)/x_spki.obj $(OBJ_D)/nsseq.obj \
	$(OBJ_D)/d2i_pu.obj $(OBJ_D)/d2i_pr.obj \
	$(OBJ_D)/i2d_pu.obj $(OBJ_D)/i2d_pr.obj $(OBJ_D)/t_req.obj \
	$(OBJ_D)/t_crl.obj \
	$(OBJ_D)/t_pkey.obj $(OBJ_D)/t_spki.obj $(OBJ_D)/t_bitst.obj \
	$(OBJ_D)/tasn_new.obj $(OBJ_D)/tasn_fre.obj $(OBJ_D)/tasn_enc.obj \
	$(OBJ_D)/tasn_dec.obj $(OBJ_D)/tasn_utl.obj $(OBJ_D)/tasn_typ.obj \
	$(OBJ_D)/tasn_prn.obj $(OBJ_D)/ameth_lib.obj $(OBJ_D)/f_int.obj \
	$(OBJ_D)/f_string.obj $(OBJ_D)/n_pkey.obj \
	$(OBJ_D)/x_pkey.obj $(OBJ_D)/x_exten.obj \
	$(OBJ_D)/bio_asn1.obj $(OBJ_D)/bio_ndef.obj $(OBJ_D)/asn_mime.obj \
	$(OBJ_D)/asn1_gen.obj $(OBJ_D)/asn1_par.obj $(OBJ_D)/asn1_lib.obj \
	$(OBJ_D)/asn1_err.obj $(OBJ_D)/a_strnid.obj \
	$(OBJ_D)/evp_asn1.obj $(OBJ_D)/asn_pack.obj $(OBJ_D)/p5_pbe.obj \
	$(OBJ_D)/p5_pbev2.obj $(OBJ_D)/p8_pkey.obj $(OBJ_D)/asn_moid.obj \
	$(OBJ_D)/pem_sign.obj $(OBJ_D)/pem_info.obj \
	$(OBJ_D)/pem_lib.obj $(OBJ_D)/pem_all.obj $(OBJ_D)/pem_err.obj \
	$(OBJ_D)/pem_x509.obj $(OBJ_D)/pem_xaux.obj $(OBJ_D)/pem_oth.obj \
	$(OBJ_D)/pem_pk8.obj $(OBJ_D)/pem_pkey.obj $(OBJ_D)/pvkfmt.obj \
	$(OBJ_D)/x509_def.obj $(OBJ_D)/x509_d2.obj $(OBJ_D)/x509_r2x.obj \
	$(OBJ_D)/x509_cmp.obj $(OBJ_D)/x509_obj.obj $(OBJ_D)/x509_req.obj \
	$(OBJ_D)/x509spki.obj $(OBJ_D)/x509_vfy.obj $(OBJ_D)/x509_set.obj \
	$(OBJ_D)/x509cset.obj $(OBJ_D)/x509rset.obj $(OBJ_D)/x509_err.obj \
	$(OBJ_D)/x509name.obj $(OBJ_D)/x509_v3.obj $(OBJ_D)/x509_ext.obj \
	$(OBJ_D)/x509_att.obj $(OBJ_D)/x509type.obj $(OBJ_D)/x509_lu.obj \
	$(OBJ_D)/x_all.obj $(OBJ_D)/x509_txt.obj $(OBJ_D)/x509_trs.obj \
	$(OBJ_D)/by_file.obj $(OBJ_D)/by_dir.obj $(OBJ_D)/x509_vpm.obj \
	$(OBJ_D)/v3_bcons.obj $(OBJ_D)/v3_bitst.obj $(OBJ_D)/v3_conf.obj \
	$(OBJ_D)/v3_extku.obj $(OBJ_D)/v3_ia5.obj $(OBJ_D)/v3_lib.obj \
	$(OBJ_D)/v3_prn.obj $(OBJ_D)/v3_utl.obj $(OBJ_D)/v3err.obj \
	$(OBJ_D)/v3_genn.obj $(OBJ_D)/v3_alt.obj $(OBJ_D)/v3_skey.obj \
	$(OBJ_D)/v3_akey.obj $(OBJ_D)/v3_pku.obj $(OBJ_D)/v3_int.obj \
	$(OBJ_D)/v3_enum.obj $(OBJ_D)/v3_sxnet.obj $(OBJ_D)/v3_cpols.obj \
	$(OBJ_D)/v3_crld.obj $(OBJ_D)/v3_purp.obj $(OBJ_D)/v3_info.obj \
	$(OBJ_D)/v3_ocsp.obj $(OBJ_D)/v3_akeya.obj $(OBJ_D)/v3_pmaps.obj \
	$(OBJ_D)/v3_pcons.obj $(OBJ_D)/v3_ncons.obj $(OBJ_D)/v3_pcia.obj \
	$(OBJ_D)/v3_pci.obj $(OBJ_D)/pcy_cache.obj $(OBJ_D)/pcy_node.obj \
	$(OBJ_D)/pcy_data.obj $(OBJ_D)/pcy_map.obj $(OBJ_D)/pcy_tree.obj \
	$(OBJ_D)/pcy_lib.obj $(OBJ_D)/v3_asid.obj $(OBJ_D)/v3_addr.obj \
	$(OBJ_D)/cms_lib.obj $(OBJ_D)/cms_asn1.obj \
	$(OBJ_D)/cms_att.obj $(OBJ_D)/cms_io.obj $(OBJ_D)/cms_smime.obj \
	$(OBJ_D)/cms_err.obj $(OBJ_D)/cms_sd.obj $(OBJ_D)/cms_dd.obj \
	$(OBJ_D)/cms_cd.obj $(OBJ_D)/cms_env.obj $(OBJ_D)/cms_enc.obj \
	$(OBJ_D)/cms_ess.obj $(OBJ_D)/cms_pwri.obj $(OBJ_D)/cms_kari.obj \
	$(OBJ_D)/conf_err.obj $(OBJ_D)/conf_lib.obj $(OBJ_D)/conf_api.obj \
	$(OBJ_D)/conf_def.obj $(OBJ_D)/conf_mod.obj $(OBJ_D)/conf_mall.obj \
	$(OBJ_D)/conf_sap.obj $(OBJ_D)/txt_db.obj $(OBJ_D)/pk7_asn1.obj \
	$(OBJ_D)/pk7_lib.obj $(OBJ_D)/pkcs7err.obj $(OBJ_D)/pk7_doit.obj \
	$(OBJ_D)/pk7_smime.obj $(OBJ_D)/pk7_attr.obj $(OBJ_D)/pk7_mime.obj \
	$(OBJ_D)/bio_pk7.obj $(OBJ_D)/p12_add.obj $(OBJ_D)/p12_asn.obj \
	$(OBJ_D)/p12_attr.obj $(OBJ_D)/p12_crpt.obj $(OBJ_D)/p12_crt.obj \
	$(OBJ_D)/p12_decr.obj $(OBJ_D)/p12_init.obj $(OBJ_D)/p12_key.obj \
	$(OBJ_D)/p12_kiss.obj $(OBJ_D)/p12_mutl.obj $(OBJ_D)/p12_utl.obj \
	$(OBJ_D)/p12_npas.obj $(OBJ_D)/pk12err.obj $(OBJ_D)/p12_p8d.obj \
	$(OBJ_D)/p12_p8e.obj $(OBJ_D)/comp_lib.obj $(OBJ_D)/comp_err.obj \
	$(OBJ_D)/c_zlib.obj $(OBJ_D)/ocsp_asn.obj \
	$(OBJ_D)/ocsp_ext.obj $(OBJ_D)/ocsp_ht.obj $(OBJ_D)/ocsp_lib.obj \
	$(OBJ_D)/ocsp_cl.obj $(OBJ_D)/ocsp_srv.obj $(OBJ_D)/ocsp_prn.obj \
	$(OBJ_D)/ocsp_vfy.obj $(OBJ_D)/ocsp_err.obj $(OBJ_D)/ui_err.obj \
	$(OBJ_D)/ui_lib.obj $(OBJ_D)/ui_openssl.obj $(OBJ_D)/ui_util.obj \
	$(OBJ_D)/pqueue.obj \
	$(OBJ_D)/ts_err.obj $(OBJ_D)/ts_req_utils.obj $(OBJ_D)/ts_req_print.obj \
	$(OBJ_D)/ts_rsp_utils.obj $(OBJ_D)/ts_rsp_print.obj $(OBJ_D)/ts_rsp_sign.obj \
	$(OBJ_D)/ts_rsp_verify.obj $(OBJ_D)/ts_verify_ctx.obj $(OBJ_D)/ts_lib.obj \
	$(OBJ_D)/ts_conf.obj $(OBJ_D)/ts_asn1.obj $(OBJ_D)/srp_lib.obj \
	$(OBJ_D)/srp_vfy.obj \
	$(OBJ_D)/sha1-x86_64.obj $(OBJ_D)/sha256-x86_64.obj $(OBJ_D)/sha512-x86_64.obj \
	$(OBJ_D)/sha1-mb-x86_64.obj $(OBJ_D)/sha256-mb-x86_64.obj \
	$(OBJ_D)/poly1305-x86_64.obj $(OBJ_D)/poly1305_pmeth.obj $(OBJ_D)/poly1305_ameth.obj \
	$(OBJ_D)/drbg_ctr.obj $(OBJ_D)/drbg_lib.obj \
	$(OBJ_D)/asn1_item_list.obj $(OBJ_D)/ui_null.obj $(OBJ_D)/bio_meth.obj \
	$(OBJ_D)/store_lib.obj $(OBJ_D)/store_init.obj $(OBJ_D)/loader_file.obj \
	$(OBJ_D)/store_strings.obj $(OBJ_D)/store_register.obj $(OBJ_D)/store_err.obj \
	$(OBJ_D)/conf_ssl.obj \
	$(OBJ_D)/ctype.obj \
	$(OBJ_D)/aria.obj $(OBJ_D)/e_aria.obj \
	$(OBJ_D)/sm2_sign.obj $(OBJ_D)/sm2_pmeth.obj $(OBJ_D)/sm2_crypt.obj $(OBJ_D)/sm2_err.obj \
	$(OBJ_D)/sm3.obj $(OBJ_D)/m_sm3.obj \
	$(OBJ_D)/sm4.obj $(OBJ_D)/e_sm4.obj \
	$(OBJ_D)/siphash.obj $(OBJ_D)/siphash_pmeth.obj $(OBJ_D)/siphash_ameth.obj \
 	$(OBJ_D)/pbe_scrypt.obj  \
	$(OBJ_D)/f_generic.obj $(OBJ_D)/f_impl.obj \
	$(OBJ_D)/eddsa.obj $(OBJ_D)/scalar.obj $(OBJ_D)/curve448.obj $(OBJ_D)/curve448_tables.obj \
	$(OBJ_D)/x25519-x86_64.obj \
	$(OBJ_D)/keccak1600-x86_64.obj \
	$(OBJ_D)/dh_rfc7919.obj \
	$(OBJ_D)/rsa_mp.obj \
	$(OBJ_D)/v3_admis.obj \
	$(OBJ_D)/uplink.obj \
	$(OBJ_D)/uplink-x86_64.obj \
	$(OBJ_D)/x_int64.obj \
	$(OBJ_D)/getenv.obj \
	$(OBJ_D)/aes_core.obj $(OBJ_D)/aes_cbc.obj



