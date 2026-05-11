# Measurer container — analyzes ARM binaries for memory footprint
# Minimal image (~50MB): only binutils for size/nm/objdump analysis
#
# Expects /artifacts/ volume mounted from builder with:
#   bare-metal-demo.elf, .a libraries

FROM alpine:3.20

RUN apk add --no-cache binutils-arm-none-eabi file

WORKDIR /analysis

COPY measure.sh .
RUN chmod +x measure.sh

ENTRYPOINT ["./measure.sh"]