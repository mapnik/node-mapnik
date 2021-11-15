add_library(mapbox-geometry INTERFACE)
target_include_directories(mapbox-geometry INTERFACE geometry/include)

add_library(mapbox-wagyu INTERFACE)
target_include_directories(mapbox-wagyu INTERFACE wagyu/include)

add_library(mapbox-protozero INTERFACE)
target_include_directories(mapbox-protozero INTERFACE protozero/include)
