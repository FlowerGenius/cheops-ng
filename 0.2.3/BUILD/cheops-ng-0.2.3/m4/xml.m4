# Test for libxml by Haavard Kvaalen <havardk@xmms.org>
# Based on GLIB test by Owen Taylor

dnl AM_PATH_LIBXML([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for libxml, and define XML_CFLAGS and XML_LIBS
dnl
AC_DEFUN(AM_PATH_LIBXML,
[dnl 
dnl Get the cflags and libraries from the xml-config script
dnl
AC_ARG_WITH(libxml-prefix, [  --with-libxml-prefix=PFX   Prefix where libxml is installed (optional)],
            libxml_prefix="$withval", libxml_prefix="")
AC_ARG_ENABLE(libxmltest, [  --disable-libxmltest       Do not try to compile and run a test libxml program],
		    , enable_libxmltest=yes)

  if test x$libxml_config_prefix != x ; then
     libxml_config_args="$libxml_config_args --prefix=$libxml_config_prefix"
     if test x${XML_CONFIG+set} != xset ; then
        XML_CONFIG=$libxml_config_prefix/bin/xml-config
     fi
  fi

  AC_PATH_PROG(XML_CONFIG, xml-config, no)
  min_libxml_version=ifelse([$1], ,1.0.0, $1)
  AC_MSG_CHECKING(for libxml - version >= $min_libxml_version)
  no_libxml=""
  if test "$XML_CONFIG" = "no" ; then
    no_libxml=yes
  else
    XML_CFLAGS=`$XML_CONFIG $libxml_config_args --cflags`
    XML_LIBS=`$XML_CONFIG $libxml_config_args --libs`
    libxml_major_version=`$XML_CONFIG $libxml_config_args --version | \
           sed 's/[[^0-9]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)[[^0-9]]*/\1/'`
    libxml_minor_version=`$XML_CONFIG $libxml_config_args --version | \
           sed 's/[[^0-9]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)[[^0-9]]*/\2/'`
    libxml_micro_version=`$XML_CONFIG $libxml_config_args --version | \
           sed 's/[[^0-9]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)[[^0-9]]*/\3/'`
    if test "x$enable_libxmltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $XML_CFLAGS"
      LIBS="$XML_LIBS $LIBS"
dnl
dnl Now check if the installed libxml is sufficiently new. (Also sanity
dnl checks the results of xml-config to some extent
dnl
      rm -f conf.libxmltest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <parser.h>

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
  {
      new_str = malloc((strlen (str) + 1) * sizeof(char));
      strcpy(new_str, str);
  }
  else
    new_str = NULL;
  
  return new_str;
}

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.libxmltest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_libxml_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_libxml_version");
     exit(1);
  }

  if (($libxml_major_version > major) ||
      (($libxml_major_version == major) && ($libxml_minor_version > minor)) ||
      (($libxml_major_version == major) && ($libxml_minor_version == minor) && ($libxml_micro_version >= micro)))
  {
        return 0;
  }
  else
  {
        printf("\n*** An old version of libxml (%d.%d.%d) was found.\n",
               $libxml_major_version, $libxml_minor_version, $libxml_micro_version);
        printf("*** You need a version of libxml newer than %d.%d.%d.\n",
	       major, minor, micro);
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the xml-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of libxml, but you can also set the XML_CONFIG environment to point to the\n");
        printf("*** correct copy of xml-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
  }
  return 1;
}
],, no_libxml=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_libxml" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$XML_CONFIG" = "no" ; then
	:
     else
       if test -f conf.libxmltest ; then
        :
       else
          echo "*** Could not run LIBXML test program, checking why..."
          CFLAGS="$CFLAGS $XML_CFLAGS"
          LIBS="$LIBS $XML_LIBS"
          AC_TRY_LINK([
#include <parser.h>
#include <stdio.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding libxml or finding the"
          echo "*** wrong version of libxml. If it is not finding libxml, you will"
          echo "*** need to set your LD_LIBRARY_PATH environment variable, or"
          echo "*** edit /etc/ld.so.conf to point to the installed location  Also,"
          echo "*** make sure you have run ldconfig if that is required on your system"],
        [ echo "*** The test program failed to compile or link. See the file config.log"
          echo "*** for the exact error that occured. This usually means libxml was"
          echo "*** incorrectly installed or that you have moved libxml since it was"
          echo "*** installed."])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     XML_CFLAGS=""
     XML_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(XML_CFLAGS)
  AC_SUBST(XML_LIBS)
  rm -f conf.libxmltest
])

