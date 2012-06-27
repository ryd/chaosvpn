all: installer

chaosvpn.exe: ../chaosvpn
	cp ../chaosvpn chaosvpn.exe	
	strip chaosvpn.exe

LICENSE.txt: ../LICENCE
	cp ../LICENCE LICENSE.txt
	todos LICENSE.txt

README.txt: ../README
	cp ../README README.txt
	todos README.txt

chaosvpn.1.html: ../man/chaosvpn.1
	w3mman2html $< > $@

chaosvpn.conf.5.html: ../man/chaosvpn.conf.5
	w3mman2html $< > $@

installer: chaosvpn.nsi chaosvpn.exe chaosvpn.conf LICENSE.txt README.txt chaosvpn.1.html chaosvpn.conf.5.html
	makensis $<

clean:
	rm -f *.txt *.exe *.html

.PHONY: installer clean
