# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# Version: MPL 1.1 / GPLv3+ / LGPLv3+
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License or as specified alternatively below. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# Major Contributor(s):
# Copyright (C) 2012 Matúš Kukan <matus.kukan@gmail.com> (initial developer)
#
# All Rights Reserved.
#
# For minor contributions see the git repository.
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 3 or later (the "GPLv3+"), or
# the GNU Lesser General Public License Version 3 or later (the "LGPLv3+"),
# in which case the provisions of the GPLv3+ or the LGPLv3+ are applicable
# instead of those above.

$(eval $(call gb_Package_Package,unotest_inc,$(SRCDIR)/unotest/inc/unotest))

$(eval $(call gb_Package_add_file,unotest_inc,inc/unotest/bootstrapfixturebase.hxx,bootstrapfixturebase.hxx))
$(eval $(call gb_Package_add_file,unotest_inc,inc/unotest/filters-test.hxx,filters-test.hxx))
$(eval $(call gb_Package_add_file,unotest_inc,inc/unotest/macros_test.hxx,macros_test.hxx))
$(eval $(call gb_Package_add_file,unotest_inc,inc/unotest/gettestargument.hxx,gettestargument.hxx))
$(eval $(call gb_Package_add_file,unotest_inc,inc/unotest/officeconnection.hxx,officeconnection.hxx))
$(eval $(call gb_Package_add_file,unotest_inc,inc/unotest/toabsolutefileurl.hxx,toabsolutefileurl.hxx))
$(eval $(call gb_Package_add_file,unotest_inc,inc/unotest/uniquepipename.hxx,uniquepipename.hxx))
$(eval $(call gb_Package_add_file,unotest_inc,inc/unotest/detail/unotestdllapi.hxx,detail/unotestdllapi.hxx))

# vim: set noet sw=4 ts=4: