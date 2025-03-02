FROM alpine:latest


RUN apk add --no-cache openssl build-base libssh-dev

RUN adduser -h /home/seed -s /bin/sh -D seed
RUN echo -n 'seed:dees' | chpasswd
WORKDIR /app

EXPOSE 9443
#COPY https_server.c .


CMD ["tail", "-f", "/dev/null"]