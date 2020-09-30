FROM esydev/esy:nightly-alpine-latest

RUN apk add nodejs npm linux-headers emacs
COPY . /app
WORKDIR /app
RUN esy --cache-tarballs-path=esy-sources
RUN esy release
WORKDIR /app/_release
RUN npm pack
