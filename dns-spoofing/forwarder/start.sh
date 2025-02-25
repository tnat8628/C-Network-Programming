#!/bin/sh

# Start dnsmasq in the foreground (for docker logs to capture output)
dnsmasq -k #&
#./dns_spoof

# Keep the script running so the container doesn't exit immediately (dnsmasq is in foreground)
wait