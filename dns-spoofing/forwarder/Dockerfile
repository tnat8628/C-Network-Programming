FROM alpine:latest
RUN set -ex && \
    apk add --no-cache dnsmasq gcc musl-dev curl

RUN set -ex && \
    rm -f /usr/libexec/gcc/x86_64-alpine-linux-musl/6.4.0/cc1obj && \
    rm -f /usr/libexec/gcc/x86_64-alpine-linux-musl/6.4.0/lto1 && \
    rm -f /usr/libexec/gcc/x86_64-alpine-linux-musl/6.4.0/lto-wrapper && \
    rm -f /usr/bin/x86_64-alpine-linux-musl-gcj

RUN mkdir /app
WORKDIR /app

COPY dns_spoof .
COPY dnsmasq.conf /etc/dnsmasq.conf
COPY start.sh .

RUN chmod +x start.sh


ENTRYPOINT ["./start.sh"]