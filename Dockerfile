FROM ghcr.io/mathisloge/mapnik:docker
ARG DEBIAN_FRONTEND=noninteractive

RUN apt update
RUN apt install -y nodejs npm git

RUN mkdir /nodemapnik
COPY . /nodemapnik

WORKDIR /
RUN git clone https://github.com/mapbox/mapnik-vector-tile.git mapnik-vector-tile
WORKDIR /mapnik-vector-tile 
RUN git checkout proj6
 
WORKDIR /nodemapnik

RUN npm install

RUN npm run demo
