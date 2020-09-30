FROM node:current-alpine

COPY _container_release /app/_container_release
WORKDIR /app_container_release
RUN npm i -g . 