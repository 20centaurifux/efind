MSGFMT=msgfmt
PACKAGE_DOMAIN=efind

all:
	@for p in *.po; \
	do \
		echo Compiling translation: $$p; \
		mkdir -p ./locale/$${p%.po}/LC_MESSAGES && \
		msgfmt -c $$p -o ./locale/$${p%.po}/LC_MESSAGES/$(PACKAGE_DOMAIN).mo; \
	done

	@echo "Finished"

install:
	@if [ -d $(LOCALEDIR) ]; then \
		cp -r ./locale/* $(LOCALEDIR); \
	fi

uninstall:
	@if [ -d $(LOCALEDIR) ]; then \
		for p in *.po; \
		do \
			rm -f $(LOCALEDIR)/$${p%.po}/LC_MESSAGES/$(PACKAGE_DOMAIN).mo; \
		done \
	fi

clean:
	rm -fr ./locale
