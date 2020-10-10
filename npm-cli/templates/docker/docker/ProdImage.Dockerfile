FROM node:current-alpine
COPY _container_release /app/_container_release
WORKDIR /app/_container_release
RUN yarn global --prefix /usr/local add $PWD

# Need glibc right now. TODO get rid of them
RUN wget -q -O /etc/apk/keys/sgerrand.rsa.pub https://alpine-pkgs.sgerrand.com/sgerrand.rsa.pub
RUN wget https://github.com/sgerrand/alpine-pkg-glibc/releases/download/2.31-r0/glibc-2.31-r0.apk
RUN apk add glibc-2.31-r0.apk

ENTRYPOINT FooApp

# To use: docker run --rm -it --name foo-c foo