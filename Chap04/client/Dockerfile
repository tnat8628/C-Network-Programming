# Dockerfile 
FROM alpine:latest


RUN apk add --no-cache openssl curl tshark 
    #openssh libssh-dev build-base

# Set working directory
WORKDIR /app

# Copy the C source code (assuming it's in the same directory as this Dockerfile)
#COPY client.c .  


# Keep the container running (for interactive use)
CMD ["tail", "-f", "/dev/null"]