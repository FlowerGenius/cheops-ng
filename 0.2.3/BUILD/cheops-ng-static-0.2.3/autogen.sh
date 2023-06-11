#!/bin/sh

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have autoconf installed to compile cheops-ng";
	echo;
	exit;
}

# Thanks decklin
if test -f configure.ac ; then
	if autoconf --version | grep '2\.[01]' > /dev/null 2>&1 ; then
		mv configure.ac configure.2.1x;
		echo "configure.ac has been moved to configure.2.1x to retain compatibility with autoconf 2.1x"
		echo "Future versions of cheops-ng will not support autoconf versions older than 2.50"

	fi
fi

echo "Generating configuration files for cheops-ng, please wait...."
echo;

aclocal --acdir=m4 || exit;
autoconf || exit;

