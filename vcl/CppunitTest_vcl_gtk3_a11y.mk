# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# hack plugging into the CppunitTest machinery yet using a xvfb-wrapper GTK3 run
$(call gb_CppunitTest_get_target,vcl_gtk3_a11y) : gb_TEST_ENV_VARS += SAL_USE_VCLPLUGIN=gtk3
ifeq (,$(VCL_GTK3_TESTS_NO_XVFB))
$(call gb_CppunitTest_get_target,vcl_gtk3_a11y) : \
	ICECREAM_RUN += $(XVFB_RUN) $(DBUS_LAUNCH) --exit-with-session
endif

$(eval $(call gb_CppunitTest_CppunitTest,vcl_gtk3_a11y))

$(eval $(call gb_CppunitTest_add_exception_objects,vcl_gtk3_a11y, \
	vcl/qa/cppunit/a11y/atspi2/atspiwrapper \
	vcl/qa/cppunit/a11y/atspi2/atspi2 \
	vcl/qa/cppunit/a11y/atspi2/atspi2text \
))

$(eval $(call gb_CppunitTest_set_include,vcl_gtk3_a11y,\
	$$(INCLUDE) \
	$$(ATSPI2_CFLAGS) \
))

$(eval $(call gb_CppunitTest_add_libs,vcl_gtk3_a11y,\
	$$(ATSPI2_LIBS) \
))

$(eval $(call gb_CppunitTest_use_libraries,vcl_gtk3_a11y, \
	sal \
	cppu \
	subsequenttest \
	test \
	i18nlangtag \
	unotest \
	vcl \
))

$(eval $(call gb_CppunitTest_use_externals,vcl_gtk3_a11y,\
	boost_headers \
))

$(eval $(call gb_CppunitTest_use_api,vcl_gtk3_a11y,\
	offapi \
	udkapi \
))

$(eval $(call gb_CppunitTest_use_sdk_api,vcl_gtk3_a11y))
$(eval $(call gb_CppunitTest_use_rdb,vcl_gtk3_a11y,services))
$(eval $(call gb_CppunitTest_use_ure,vcl_gtk3_a11y))
$(eval $(call gb_CppunitTest_use_vcl,vcl_gtk3_a11y))

$(eval $(call gb_CppunitTest_use_instdir_configuration,vcl_gtk3_a11y))
$(eval $(call gb_CppunitTest_use_common_configuration,vcl_gtk3_a11y))

# vim: set noet sw=4 ts=4:
