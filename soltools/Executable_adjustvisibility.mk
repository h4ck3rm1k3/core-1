# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Executable_Executable,adjustvisibility))

ifeq ($(COM),MSC)
$(eval $(call gb_Executable_use_packages,adjustvisibility,\
    soltools_inc \
))
endif

$(eval $(call gb_Executable_add_exception_objects,adjustvisibility,\
    soltools/adjustvisibility/adjustvisibility \
))

# vim:set shiftwidth=4 softtabstop=4 expandtab: