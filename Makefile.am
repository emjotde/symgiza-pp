## Process this file with automake to produce Makefile.in
## Created by Anjuta

AM_CPPFLAGS = -Wno-deprecated -Wno-write-strings

SUBDIRS = src 

symgizadocdir = ${prefix}/doc/symgiza
symgizadoc_DATA = \
	README.md\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS

symgizascriptsdir = ${prefix}/scripts/

symgizascripts_SCRIPTS = \
				   scripts/*

EXTRA_DIST = $(symgizadoc_DATA) \
	         ${symgizascripts_SCRIPTS}
# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

