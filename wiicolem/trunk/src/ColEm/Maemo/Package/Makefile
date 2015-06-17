all:
	make --directory=.. all

build:
	make --directory=.. all

clean:
	make --directory=.. clean

install:
	install -m 755 -D ../colem $(DESTDIR)/usr/bin/colem
	install -m 755 -D run-colem $(DESTDIR)/usr/bin/run-colem
	install -m 644 -D COLECO.ROM $(DESTDIR)/usr/share/colem/COLECO.ROM
	install -m 644 -D colem-26x26.png $(DESTDIR)/usr/share/icons/hicolor/26x26/hildon/colem.png
	install -m 644 -D colem-40x40.png $(DESTDIR)/usr/share/icons/hicolor/40x40/hildon/colem.png
	install -m 644 -D colem-64x64.png $(DESTDIR)/usr/share/icons/hicolor/scalable/hildon/colem.png
	install -m 644 -D colem.desktop $(DESTDIR)/usr/share/applications/hildon/colem.desktop
	install -m 644 -D colem.service $(DESTDIR)/usr/share/dbus-1/services/colem.service
	install -m 644 -D colem-mime.xml $(DESTDIR)/usr/share/mime/packages/colem-mime.xml

uninstall:
	rm -rf $(DESTDIR)/usr/share/colem
	rm -f $(DESTDIR)/usr/bin/colem
	rm -f $(DESTDIR)/usr/bin/run-colem
	rm -f $(DESTDIR)/usr/share/applications/hildon/colem.desktop
	rm -f $(DESTDIR)/usr/share/dbus-1/services/colem.service
	rm -f $(DESTDIR)/usr/share/icons/hicolor/26x26/hildon/colem.png
	rm -f $(DESTDIR)/usr/share/icons/hicolor/40x40/hildon/colem.png
	rm -f $(DESTDIR)/usr/share/icons/hicolor/scalable/hildon/colem.png
