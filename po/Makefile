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
	mkdir -p $(DESTDIR)$(LOCALEDIR)
	cp -r ./locale/* $(DESTDIR)$(LOCALEDIR); \

uninstall:
	for p in *.po; \
	do \
		rm -f $(DESTDIR)$(LOCALEDIR)/$${p%.po}/LC_MESSAGES/$(PACKAGE_DOMAIN).mo; \
	done

clean:
	rm -fr ./locale
