#
# $Id: Makefile.am,v 1.2 2006/06/15 09:53:23 richter Exp $
#
#***********************************************************************
#
# FeeClient library
# Copyright (c) 2006
#
# This file is property of and copyright by the Experimental Nuclear 
# Physics Group, Dep. of Physics and Technology
# University of Bergen, Norway, 2004
# This file has been written by Matthias Richter,
# Matthias.Richter@ift.uib.no
#
# Permission to use, copy, modify and distribute this software and its  
# documentation strictly for non-commercial purposes is hereby granted  
# without fee, provided that the above copyright notice appears in all  
# copies and that both the copyright notice and this permission notice  
# appear in the supporting documentation. The authors make no claims    
# about the suitability of this software for any purpose. It is         
# provided "as is" without express or implied warranty.                 
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
# MA 02111-1307  USA  
#
#***********************************************************************/

EXTRA_DIST		= mainpage.c
DOC_SRC			= src/mrshellprim.c \
			  src/mrshellprim.h

LATEX_DIR		= latex
PDF_SRC			= refman.pdf
DISTCLEANFILES		= doxy.log $(PACKAGE).tags
if HAVE_DOXYGEN
HTML			= html/index.html 
TAGS			= $(PACKAGE).tags
else
HTML			=
TAGS			= 
endif
if CREATE_PDF_DOC
PDF			= pdf
else
PDF			=
endif

%.png:%.fig
	fig2dev -Lpng $< 

all-local:$(HTML) $(PDF)

clean-local: 
	rm -rf *~ html $(LATEX_DIR) man $(PACKAGE)-doc.pdf

html/index.html $(PACKAGE).tags: doxygen.conf mainpage.c $(DOC_SRC:%=$(top_srcdir)/%)	
	$(DOXYGEN) $<  
	for i in `find @srcdir@ -name '*.png' -or -name '*.gif'`; \
		do cp -f $$i html/pic_`basename $$i`; done

$(LATEX_DIR)/$(PDF_SRC): $(HTML)
	[ -e $(LATEX_DIR)/Makefile ] && $(MAKE) -C $(LATEX_DIR)

pdf: $(LATEX_DIR)/$(PDF_SRC)
	[ -e $(LATEX_DIR)/$(PDF_SRC) ] && cp $< $(PACKAGE)-doc.pdf

tar-ball:$(HTML)
	if test -f html/index.html ; then \
	  mv html $(PACKAGE) ;	\
	  tar -czvf ../$(PACKAGE)-$(VERSION)-doc.tar.gz $(PACKAGE) ; \
	  mv $(PACKAGE) html ; fi 


#
# EOF
#
