QT += widgets network multimedia webkitwidgets multimediawidgets printsupport
CONFIG += warn_off
CONFIG += app_bundle
INCLUDEPATH += /Users/eduard/Documents/qt_depend/include
INCLUDEPATH += ./tomcrypt
INCLUDEPATH += /usr/local/include
INCLUDEPATH += /usr/local/include/opus
LIBPATH += /Users/eduard/Documents/qt_depend/lib
LIBPATH += /usr/lib /usr/local/lib
LIBS += -ldl
LIBS += -lcrypto -lssl
LIBS += -lspeex -lopus
ICON = APPL.icns
QMAKE_CXXFLAGS += -DAPP_PATH=\"\\\"/usr/bin/\\\"\" -DCOMPONENTS_PATH=\"\\\"/usr/libexec/ConceptClient3/components/\\\"\" -D__WITH_TOMCRYPT

HEADERS += \
    AES/aes.h \
    bn_openssl097beta1/bn.h \
    bn_openssl097beta1/bn_lcl.h \
    bn_openssl097beta1/bn_prime.h \
    bn_openssl097beta1/hmf_prime.h \
    bn_openssl097beta1/hmf_rand.h \
    PropertiesBox/qteditorfactory.h \
    PropertiesBox/qtpropertybrowser.h \
    PropertiesBox/qtpropertybrowserutils_p.h \
    PropertiesBox/qtpropertymanager.h \
    PropertiesBox/qttreepropertybrowser.h \
    PropertiesBox/qtvariantproperty.h \
    rsa/rsa.h \
    AnsiList.h \
    AnsiString.h \
    BaseWidget.h \
    BasicMessages.h \
    callback.h \
    CARoot.h \
    components_library.h \
    ConceptClient.h \
    connectdialog.h \
    consolewindow.h \
    CustomButton.h \
    EventAwareWidget.h \
    MacOSXApplication.h \
    EventHandler.h \
    ForwardLabel.h \
    GTKControl.h \
    httpmessaging.h \
    interface.h \
    INTERFACE_TO_AES.h \
    ListDelegate.h \
    logindialog.h \
    ManagedPage.h \
    ManagedRequest.h \
    md5.h \
    md5_global.h \
    messages.h \
    mtrand.h \
    resources.h \
    semhh.h \
    sha1.h \
    sha256.h \
    stock_constants.h \
    ToWriteText.h \
    TreeColumn.h \
    TreeDelegate.h \
    ui_connectdialog.h \
    ui_consolewindow.h \
    ui_logindialog.h \
    unzip.h \
    verticallabel.h \
    CompensationParser.h \
    Codes.h \
    worker.h \
    cameraframegrabber.h \
    AudioObject.h \
    tomcrypt/tomcrypt.h tomcrypt/tomcrypt_argchk.h tomcrypt/tomcrypt_cfg.h tomcrypt/tomcrypt_cipher.h tomcrypt/tomcrypt_custom.h tomcrypt/tomcrypt_hash.h tomcrypt/tomcrypt_mac.h tomcrypt/tomcrypt_macros.h tomcrypt/tomcrypt_math.h tomcrypt/tomcrypt_misc.h tomcrypt/tomcrypt_pk.h tomcrypt/tomcrypt_pkcs.h tomcrypt/tomcrypt_prng.h tomcrypt/tommath.h tomcrypt/tommath_class.h tomcrypt/tommath_private.h tomcrypt/tommath_superclass.h

SOURCES += \
    AES/aes.cpp \
    PropertiesBox/qteditorfactory.cpp \
    PropertiesBox/qtpropertybrowser.cpp \
    PropertiesBox/qtpropertybrowserutils.cpp \
    PropertiesBox/qtpropertymanager.cpp \
    PropertiesBox/qttreepropertybrowser.cpp \
    PropertiesBox/qtvariantproperty.cpp \
    AnsiList.cpp \
    AnsiString.cpp \
    callback.cpp \
    ConceptClient.cpp \
    ConClientResources.cpp \
    connectdialog.cpp \
    consolewindow.cpp \
    EventHandler.cpp \
    httpmessaging.cpp \
    interface.cpp \
    logindialog.cpp \
    main.cpp \
    messages.cpp \
    mtrand.cpp \
    UriCodec.cpp \
    verticallabel.cpp \
    worker.cpp \
    bn_openssl097beta1/bn_add.c \
    bn_openssl097beta1/bn_asm.c \
    bn_openssl097beta1/bn_ctx.c \
    bn_openssl097beta1/bn_div.c \
    bn_openssl097beta1/bn_exp.c \
    bn_openssl097beta1/bn_exp2.c \
    bn_openssl097beta1/bn_gcd.c \
    bn_openssl097beta1/bn_lib.c \
    bn_openssl097beta1/bn_mod.c \
    bn_openssl097beta1/bn_mont.c \
    bn_openssl097beta1/bn_mpi.c \
    bn_openssl097beta1/bn_mul.c \
    bn_openssl097beta1/bn_prime.c \
    bn_openssl097beta1/bn_rand_hmf.c \
    bn_openssl097beta1/bn_recp.c \
    bn_openssl097beta1/bn_shift.c \
    bn_openssl097beta1/bn_sqr.c \
    bn_openssl097beta1/bn_word.c \
    bn_openssl097beta1/hmf_rand.c \
    rsa/rsa_convert.c \
    rsa/rsa_encode.c \
    rsa/rsa_keys.c \
    rsa/rsa_lib.c \
    rsa/rsa_pem.c \
    md5c.c \
    sha1.c \
    sha256.c \
    unzip.cpp \
    Codes.cpp \
    cameraframegrabber.cpp \
    AudioObject.cpp \
    tomcrypt/bn_error.c tomcrypt/bn_fast_mp_invmod.c tomcrypt/bn_fast_mp_montgomery_reduce.c tomcrypt/bn_fast_s_mp_mul_digs.c tomcrypt/bn_fast_s_mp_mul_high_digs.c tomcrypt/bn_fast_s_mp_sqr.c tomcrypt/bn_mp_2expt.c tomcrypt/bn_mp_abs.c tomcrypt/bn_mp_add.c tomcrypt/bn_mp_add_d.c tomcrypt/bn_mp_addmod.c tomcrypt/bn_mp_and.c tomcrypt/bn_mp_clamp.c tomcrypt/bn_mp_clear.c tomcrypt/bn_mp_clear_multi.c tomcrypt/bn_mp_cmp.c tomcrypt/bn_mp_cmp_d.c tomcrypt/bn_mp_cmp_mag.c tomcrypt/bn_mp_cnt_lsb.c tomcrypt/bn_mp_copy.c tomcrypt/bn_mp_count_bits.c tomcrypt/bn_mp_div.c tomcrypt/bn_mp_div_2.c tomcrypt/bn_mp_div_2d.c tomcrypt/bn_mp_div_3.c tomcrypt/bn_mp_div_d.c tomcrypt/bn_mp_dr_is_modulus.c tomcrypt/bn_mp_dr_reduce.c tomcrypt/bn_mp_dr_setup.c tomcrypt/bn_mp_exch.c tomcrypt/bn_mp_export.c tomcrypt/bn_mp_expt_d.c tomcrypt/bn_mp_expt_d_ex.c tomcrypt/bn_mp_exptmod.c tomcrypt/bn_mp_exptmod_fast.c tomcrypt/bn_mp_exteuclid.c tomcrypt/bn_mp_fread.c tomcrypt/bn_mp_fwrite.c tomcrypt/bn_mp_gcd.c tomcrypt/bn_mp_get_int.c tomcrypt/bn_mp_get_long.c tomcrypt/bn_mp_get_long_long.c tomcrypt/bn_mp_grow.c tomcrypt/bn_mp_import.c tomcrypt/bn_mp_init.c tomcrypt/bn_mp_init_copy.c tomcrypt/bn_mp_init_multi.c tomcrypt/bn_mp_init_set.c tomcrypt/bn_mp_init_set_int.c tomcrypt/bn_mp_init_size.c tomcrypt/bn_mp_invmod.c tomcrypt/bn_mp_invmod_slow.c tomcrypt/bn_mp_is_square.c tomcrypt/bn_mp_jacobi.c tomcrypt/bn_mp_karatsuba_mul.c tomcrypt/bn_mp_karatsuba_sqr.c tomcrypt/bn_mp_lcm.c tomcrypt/bn_mp_lshd.c tomcrypt/bn_mp_mod.c tomcrypt/bn_mp_mod_2d.c tomcrypt/bn_mp_mod_d.c tomcrypt/bn_mp_montgomery_calc_normalization.c tomcrypt/bn_mp_montgomery_reduce.c tomcrypt/bn_mp_montgomery_setup.c tomcrypt/bn_mp_mul.c tomcrypt/bn_mp_mul_2.c tomcrypt/bn_mp_mul_2d.c tomcrypt/bn_mp_mul_d.c tomcrypt/bn_mp_mulmod.c tomcrypt/bn_mp_n_root.c tomcrypt/bn_mp_n_root_ex.c tomcrypt/bn_mp_neg.c tomcrypt/bn_mp_or.c tomcrypt/bn_mp_prime_fermat.c tomcrypt/bn_mp_prime_is_divisible.c tomcrypt/bn_mp_prime_is_prime.c tomcrypt/bn_mp_prime_miller_rabin.c tomcrypt/bn_mp_prime_next_prime.c tomcrypt/bn_mp_prime_rabin_miller_trials.c tomcrypt/bn_mp_prime_random_ex.c tomcrypt/bn_mp_radix_size.c tomcrypt/bn_mp_radix_smap.c tomcrypt/bn_mp_rand.c tomcrypt/bn_mp_read_radix.c tomcrypt/bn_mp_read_signed_bin.c tomcrypt/bn_mp_read_unsigned_bin.c tomcrypt/bn_mp_reduce.c tomcrypt/bn_mp_reduce_2k.c tomcrypt/bn_mp_reduce_2k_l.c tomcrypt/bn_mp_reduce_2k_setup.c tomcrypt/bn_mp_reduce_2k_setup_l.c tomcrypt/bn_mp_reduce_is_2k.c tomcrypt/bn_mp_reduce_is_2k_l.c tomcrypt/bn_mp_reduce_setup.c tomcrypt/bn_mp_rshd.c tomcrypt/bn_mp_set.c tomcrypt/bn_mp_set_int.c tomcrypt/bn_mp_set_long.c tomcrypt/bn_mp_set_long_long.c tomcrypt/bn_mp_shrink.c tomcrypt/bn_mp_signed_bin_size.c tomcrypt/bn_mp_sqr.c tomcrypt/bn_mp_sqrmod.c tomcrypt/bn_mp_sqrt.c tomcrypt/bn_mp_sqrtmod_prime.c tomcrypt/bn_mp_sub.c tomcrypt/bn_mp_sub_d.c tomcrypt/bn_mp_submod.c tomcrypt/bn_mp_to_signed_bin.c tomcrypt/bn_mp_to_signed_bin_n.c tomcrypt/bn_mp_to_unsigned_bin.c tomcrypt/bn_mp_to_unsigned_bin_n.c tomcrypt/bn_mp_toom_mul.c tomcrypt/bn_mp_toom_sqr.c tomcrypt/bn_mp_toradix.c tomcrypt/bn_mp_toradix_n.c tomcrypt/bn_mp_unsigned_bin_size.c tomcrypt/bn_mp_xor.c tomcrypt/bn_mp_zero.c tomcrypt/bn_prime_tab.c tomcrypt/bn_reverse.c tomcrypt/bn_s_mp_add.c tomcrypt/bn_s_mp_exptmod.c tomcrypt/bn_s_mp_mul_digs.c tomcrypt/bn_s_mp_mul_high_digs.c tomcrypt/bn_s_mp_sqr.c tomcrypt/bn_s_mp_sub.c tomcrypt/bncore.c tomcrypt/crypt.c tomcrypt/crypt_argchk.c tomcrypt/crypt_cipher_descriptor.c tomcrypt/crypt_cipher_is_valid.c tomcrypt/crypt_find_cipher.c tomcrypt/crypt_find_cipher_any.c tomcrypt/crypt_find_cipher_id.c tomcrypt/crypt_find_hash.c tomcrypt/crypt_find_hash_any.c tomcrypt/crypt_find_hash_id.c tomcrypt/crypt_find_hash_oid.c tomcrypt/crypt_find_prng.c tomcrypt/crypt_fsa.c tomcrypt/crypt_hash_descriptor.c tomcrypt/crypt_hash_is_valid.c tomcrypt/crypt_ltc_mp_descriptor.c tomcrypt/crypt_prng_descriptor.c tomcrypt/crypt_prng_is_valid.c tomcrypt/crypt_register_cipher.c tomcrypt/crypt_register_hash.c tomcrypt/crypt_register_prng.c tomcrypt/crypt_unregister_cipher.c tomcrypt/crypt_unregister_hash.c tomcrypt/crypt_unregister_prng.c tomcrypt/der_decode_bit_string.c tomcrypt/der_decode_boolean.c tomcrypt/der_decode_choice.c tomcrypt/der_decode_ia5_string.c tomcrypt/der_decode_integer.c tomcrypt/der_decode_object_identifier.c tomcrypt/der_decode_octet_string.c tomcrypt/der_decode_printable_string.c tomcrypt/der_decode_sequence_ex.c tomcrypt/der_decode_sequence_flexi.c tomcrypt/der_decode_sequence_multi.c tomcrypt/der_decode_short_integer.c tomcrypt/der_decode_utctime.c tomcrypt/der_decode_utf8_string.c tomcrypt/der_encode_bit_string.c tomcrypt/der_encode_boolean.c tomcrypt/der_encode_ia5_string.c tomcrypt/der_encode_integer.c tomcrypt/der_encode_object_identifier.c tomcrypt/der_encode_octet_string.c tomcrypt/der_encode_printable_string.c tomcrypt/der_encode_sequence_ex.c tomcrypt/der_encode_sequence_multi.c tomcrypt/der_encode_set.c tomcrypt/der_encode_setof.c tomcrypt/der_encode_short_integer.c tomcrypt/der_encode_utctime.c tomcrypt/der_encode_utf8_string.c tomcrypt/der_length_bit_string.c tomcrypt/der_length_boolean.c tomcrypt/der_length_ia5_string.c tomcrypt/der_length_integer.c tomcrypt/der_length_object_identifier.c tomcrypt/der_length_octet_string.c tomcrypt/der_length_printable_string.c tomcrypt/der_length_sequence.c tomcrypt/der_length_short_integer.c tomcrypt/der_length_utctime.c tomcrypt/der_length_utf8_string.c tomcrypt/der_sequence_free.c tomcrypt/ecc.c tomcrypt/ecc_ansi_x963_export.c tomcrypt/ecc_ansi_x963_import.c tomcrypt/ecc_decrypt_key.c tomcrypt/ecc_encrypt_key.c tomcrypt/ecc_export.c tomcrypt/ecc_free.c tomcrypt/ecc_get_size.c tomcrypt/ecc_import.c tomcrypt/ecc_make_key.c tomcrypt/ecc_shared_secret.c tomcrypt/ecc_sign_hash.c tomcrypt/ecc_sizes.c tomcrypt/ecc_test.c tomcrypt/ecc_verify_hash.c tomcrypt/error_to_string.c tomcrypt/gmp_desc.c tomcrypt/hash_file.c tomcrypt/hash_filehandle.c tomcrypt/hash_memory.c tomcrypt/hash_memory_multi.c tomcrypt/ltc_ecc_is_valid_idx.c tomcrypt/ltc_ecc_map.c tomcrypt/ltc_ecc_mul2add.c tomcrypt/ltc_ecc_mulmod.c tomcrypt/ltc_ecc_mulmod_timing.c tomcrypt/ltc_ecc_points.c tomcrypt/ltc_ecc_projective_add_point.c tomcrypt/ltc_ecc_projective_dbl_point.c tomcrypt/ltm_desc.c tomcrypt/multi.c tomcrypt/pkcs_1_i2osp.c tomcrypt/pkcs_1_mgf1.c tomcrypt/pkcs_1_oaep_decode.c tomcrypt/pkcs_1_oaep_encode.c tomcrypt/pkcs_1_os2ip.c tomcrypt/pkcs_1_pss_decode.c tomcrypt/pkcs_1_pss_encode.c tomcrypt/pkcs_1_v1_5_decode.c tomcrypt/pkcs_1_v1_5_encode.c tomcrypt/rand_prime.c tomcrypt/rc4.c tomcrypt/rng_get_bytes.c tomcrypt/rng_make_prng.c tomcrypt/rsa_decrypt_key.c tomcrypt/rsa_encrypt_key.c tomcrypt/rsa_export.c tomcrypt/rsa_exptmod.c tomcrypt/rsa_free.c tomcrypt/rsa_import.c tomcrypt/rsa_make_key.c tomcrypt/rsa_sign_hash.c tomcrypt/rsa_verify_hash.c tomcrypt/sha1tc.c tomcrypt/sprng.c tomcrypt/tfm_desc.c tomcrypt/zeromem.c

mac {
    ICON = APPL.icns
    QMAKE_INFO_PLIST = Info_mac.plist
    TARGET = ConceptClient
}

DISTFILES += android/AndroidManifest.xml

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
