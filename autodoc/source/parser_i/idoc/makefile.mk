#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This file incorporates work covered by the following license notice:
#
#   Licensed to the Apache Software Foundation (ASF) under one or more
#   contributor license agreements. See the NOTICE file distributed
#   with this work for additional information regarding copyright
#   ownership. The ASF licenses this file to you under the Apache
#   License, Version 2.0 (the "License"); you may not use this file
#   except in compliance with the License. You may obtain a copy of
#   the License at http://www.apache.org/licenses/LICENSE-2.0 .
#

PRJ=..$/..$/..

PRJNAME=autodoc
TARGET=parser2_s2_dsapi


# --- Settings -----------------------------------------------------

ENABLE_EXCEPTIONS=true
PRJINC=$(PRJ)$/source

.INCLUDE :  settings.mk
.INCLUDE : $(PRJ)$/source$/mkinc$/fullcpp.mk



# --- Files --------------------------------------------------------

OBJFILES= \
    $(OBJ)$/cx_docu2.obj	\
    $(OBJ)$/cx_dsapi.obj	\
    $(OBJ)$/docu_pe2.obj	\
    $(OBJ)$/tk_atag2.obj  	\
    $(OBJ)$/tk_docw2.obj   	\
    $(OBJ)$/tk_html.obj   	\
    $(OBJ)$/tk_xml.obj



# --- Targets ------------------------------------------------------

.INCLUDE :  target.mk


