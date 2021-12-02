# -*- Mode: makefile-gmake -*-

.PHONY: clean all

all:
%:
	@$(MAKE) -C unit_client $*
	@$(MAKE) -C unit_instance $*
	@$(MAKE) -C unit_registry $*
	@$(MAKE) -C unit_util $*

clean: unitclean
	rm -f coverage/*.gcov
	rm -fr coverage/report
