FROM gcc:12 AS build
WORKDIR /app
COPY afrih_server.c .
RUN gcc -O2 -o afrih_server afrih_server.c -lpthread

FROM debian:bookworm-slim
WORKDIR /app
COPY --from=build /app/afrih_server .
COPY public/ ./public/
EXPOSE 8080
CMD ["./afrih_server", "8080"]
