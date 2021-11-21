FROM ghcr.io/mathisloge/mapnik:docker
ARG DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y nodejs npm git libprotobuf-dev && mkdir /nodemapnik

WORKDIR /
RUN git clone https://github.com/mapbox/mapnik-vector-tile.git mapnik-vector-tile \
    && cd mapnik-vector-tile \
    && git checkout proj6
 

WORKDIR /nodemapnik
COPY . .
RUN npm install && rm -rf build*
RUN npm run test
