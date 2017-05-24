#
# A simple static wrapper for a number of standard Makefile targets,
# mostly just forwarding to build/Makefile. This is provided only for
# convenience and supports only a subset of what CMake's Makefile
# to offer. For more, execute that one directly.
#

BUILD=build
REPO=`basename \`git config --get remote.origin.url\``
VERSION_FULL=$(REPO)-`cat VERSION`
VERSION_MIN=$(REPO)-`cat VERSION`-minimal
HAVE_MODULES=git submodule | grep -v cmake >/dev/null

all: configured
	$(MAKE) -C $(BUILD) $@

install: configured
	$(MAKE) -C $(BUILD) $@

clean: configured
	$(MAKE) -C $(BUILD) $@

dist:
	@rm -rf $(VERSION_FULL) $(VERSION_FULL).tgz
	@rm -rf $(VERSION_MIN) $(VERSION_MIN).tgz
	@mkdir $(VERSION_FULL)
	@tar --exclude=$(VERSION_FULL)* --exclude=$(VERSION_MIN)* --exclude=.git -cf - . | ( cd $(VERSION_FULL) && tar -xpf - )
	@( cd $(VERSION_FULL) && cp -R ../.git . && git reset -q --hard HEAD && git clean -xdfq && rm -rf .git )
	@tar -czf $(VERSION_FULL).tgz $(VERSION_FULL) && echo Package: $(VERSION_FULL).tgz && rm -rf $(VERSION_FULL)
	@$(HAVE_MODULES) && mkdir $(VERSION_MIN) || exit 0
	@$(HAVE_MODULES) && tar --exclude=$(VERSION_FULL)* --exclude=$(VERSION_MIN)* --exclude=.git `git submodule | awk '{print "--exclude="$$2}' | grep -v cmake | tr '\n' ' '` -cf - . | ( cd $(VERSION_MIN) && tar -xpf - ) || exit 0
	@$(HAVE_MODULES) && ( cd $(VERSION_MIN) && cp -R ../.git . && git reset -q --hard HEAD && git clean -xdfq && rm -rf .git ) || exit 0
	@$(HAVE_MODULES) && tar -czf $(VERSION_MIN).tgz $(VERSION_MIN) && echo Package: $(VERSION_MIN).tgz && rm -rf $(VERSION_MIN) || exit 0

distclean:
	rm -rf $(BUILD)

.PHONY : doc
doc:
	cd doc && make

.PHONY : configured
configured:
	@test -d $(BUILD) || ( echo "Error: No build/ directory found. Did you run configure?" && exit 1 )
	@test -e $(BUILD)/Makefile || ( echo "Error: No build/Makefile found. Did you run configure?" && exit 1 )
