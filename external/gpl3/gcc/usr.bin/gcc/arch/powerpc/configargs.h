/* This file is automatically generated.  DO NOT EDIT! */
/* Generated from: 	NetBSD: mknative-gcc,v 1.61 2011/07/03 12:26:02 mrg Exp  */
/* Generated from: NetBSD: mknative.common,v 1.9 2007/02/05 18:26:01 apb Exp  */

/* Generated automatically. */
#ifdef _SOFT_FLOAT
static const char configuration_arguments[] = "/usr/src2/tools/gcc/../../external/gpl3/gcc/dist/configure --target=powerpc--netbsd --enable-long-long --enable-threads --with-bugurl=http://www.NetBSD.org/Misc/send-pr.html --with-pkgversion='NetBSD nb1 20110620' --enable-__cxa_atexit --with-mpc=/var/obj/ofppc/usr/src2/destdir.ofppc/usr --with-mpfr=/var/obj/ofppc/usr/src2/destdir.ofppc/usr --with-gmp=/var/obj/ofppc/usr/src2/destdir.ofppc/usr --disable-multilib --disable-symvers --disable-libstdcxx-pch --build=x86_64-unknown-netbsd5.99.54 --host=powerpc--netbsd -with-float=soft";
#else
static const char configuration_arguments[] = "/usr/src2/tools/gcc/../../external/gpl3/gcc/dist/configure --target=powerpc--netbsd --enable-long-long --enable-threads --with-bugurl=http://www.NetBSD.org/Misc/send-pr.html --with-pkgversion='NetBSD nb1 20110620' --enable-__cxa_atexit --with-mpc=/var/obj/ofppc/usr/src2/destdir.ofppc/usr --with-mpfr=/var/obj/ofppc/usr/src2/destdir.ofppc/usr --with-gmp=/var/obj/ofppc/usr/src2/destdir.ofppc/usr --disable-multilib --disable-symvers --disable-libstdcxx-pch --build=x86_64-unknown-netbsd5.99.54 --host=powerpc--netbsd";
#endif
static const char thread_model[] = "posix";

static const struct {
  const char *name, *value;
} configure_default_options[] = { 
#ifdef _SOFT_FLOAT
  { "float", "soft" },
#else
  { NULL, NULL }
#endif
};
