FROM ghcr.io/mathisloge/mapnik:docker
ARG DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y nodejs npm git libprotobuf-dev protobuf-compiler && mkdir /nodemapnik

WORKDIR /nodemapnik
COPY . .
RUN npm install && rm -rf build*
RUN npm run test
