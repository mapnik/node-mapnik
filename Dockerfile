FROM ghcr.io/mathisloge/mapnik:docker
ARG DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y nodejs npm git && mkdir /nodemapnik
COPY . /nodemapnik

WORKDIR /
RUN git clone https://github.com/mapbox/mapnik-vector-tile.git mapnik-vector-tile \
    && cd mapnik-vector-tile \
    && git checkout proj6
 
WORKDIR /nodemapnik
RUN npm install
RUN npm run test && rm -rf build
