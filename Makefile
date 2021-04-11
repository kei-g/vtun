.DEFAULT_GOAL := all
BUILD_DIR = build
RM = rm -fr
TARGET_BINARY = $(BUILD_DIR)/bin/vtun

.NOTMAIN: clean disasm install nm readelf

.PHONY: all clean disasm install nm readelf

all: $(TARGET_BINARY)

clean:
	$(RM) $(BUILD_DIR)

disasm: $(TARGET_BINARY)
	@llvm-objdump --disassemble-all $(TARGET_BINARY) | less

install: $(TARGET_BINARY)
	cat rc > /usr/local/etc/rc.d/vtun
	chmod 555 /usr/local/etc/rc.d/vtun
	cp $(TARGET_BINARY) /usr/local/sbin/
	ln -f /usr/local/sbin/vtun /usr/local/sbin/vtun-keygen
	@make clean

nm: $(TARGET_BINARY)
	@nm -gP $(TARGET_BINARY)

readelf: $(TARGET_BINARY)
	@readelf --all $(TARGET_BINARY) | less

$(TARGET_BINARY):
	@[ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && env CC=clang CXX=clang LD=clang cmake .. && cmake --build . -j8
