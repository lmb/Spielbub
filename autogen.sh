#!/bin/sh

AC_SEARCH_OPTS=""

# This is to make life easier for people who installed pkg-config in /usr/local
# but have autoconf/make/etc in /usr/. AKA most mac users
if [ -d "/usr/local/share/aclocal" ]
then
    AC_SEARCH_OPTS="-I /usr/local/share/aclocal"
fi

aclocal --force $AC_SEARCH_OPTS && \
autoconf -f && \
autoheader -f && \
automake -f --add-missing
