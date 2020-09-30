FROM esy:nightly-alpine-latest

RUN apk add nodejs npm linux-headers emacs
COPY . /app
WORKDIR /app
RUN esy --cache-tarballs-path=esy-sources
RUN esy release
RUN npm pack
