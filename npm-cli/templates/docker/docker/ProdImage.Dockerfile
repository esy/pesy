FROM node:current-alpine
COPY _container_release /app/_container_release
WORKDIR /app/_container_release
RUN yarn global --prefix /usr/local add $PWD

ENTRYPOINT FooApp

# To use: docker run --rm -it --name foo-c foo