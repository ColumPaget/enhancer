AC_INIT(hooks.c)

AC_PROG_CC
AC_LANG_C
AC_PROG_MAKE_SET
AC_HEADER_STDC
AC_SYS_LARGEFILE

AC_C_CONST
AC_C_RESTRICT
AC_C_VOLATILE
AC_C_INLINE

AC_ARG_ENABLE(x11, [  --enable-x11    use libx11 hooks], cf_x11=$enableval)
AC_ARG_WITH(x-includes, [  --with-x-includes       path to X11 includes directory], X11INC="-I$withval" )
AC_ARG_WITH(x-libraries, [  --with-x-libraries     path to X11 libraries directory], X11LIB="-L$withval" )



if test "$cf_x11" = "yes"
then
AC_CHECK_FILE([/usr/X11/include/X11/X.h], [X11ROOT="/usr/X11"],
							AC_CHECK_FILE([/usr/X11R7/include/X11/X.h], [X11ROOT="/usr/X11R7"],
							AC_CHECK_FILE([/usr/X11R6/include/X11/X.h], [X11ROOT="/usr/X11R6"])
							)
						)
	

if test "X11ROOT" != ""
then
	X11INC="-I$X11ROOT/include"
	X11LIB="-L$X11ROOT/lib"
fi

LDFLAGS="$LDFLAGS $X11LIB"
CFLAGS="$CFLAGS $X11INC"

AC_CHECK_LIB(X11,XOpenDisplay,,)
cf_have_x11=$ac_cv_lib_X11_XOpenDisplay
if test "$cf_have_x11" = "yes"
then
AC_DEFINE([HAVE_X11])
AC_SUBST([X11_HOOKS_OBJ], "x11_hooks.o")
fi
fi


AC_CHECK_LIB(c,unshare,,)
cf_have_unshare=$ac_cv_lib_c_unshare
if test "$cf_have_unshare" = "yes"
then
    AC_DEFINE([HAVE_UNSHARE])
fi



dnl read Makefile.in and write Makefile 
AC_OUTPUT(Makefile)


