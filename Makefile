all:

vbcc:
	make -C external/vbcc DESTDIR=$(PWD)/toolchain

clean:
