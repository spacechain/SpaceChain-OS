COMPONENTS = \
libsylixos \
libcextern \
libVxWorks \
libreadline \
liblua \
libsqlite3 \
pciutils \
libzmodem \
libexpat \
libluaplugin \
libsunrpc \
unfsd \
libsalsa \
libpcap \
openssl

all: $(COMPONENTS)
	@for target in $(COMPONENTS); do make -C $$target all; done

clean: $(COMPONENTS)
	@for target in $(COMPONENTS); do make -C $$target clean; done
