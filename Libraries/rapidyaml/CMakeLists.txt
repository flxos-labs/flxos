# rapidyaml single-header package exposed as an IDF component.
# The implementation is compiled once here so consuming components only need
# the public header and a standard `REQUIRES rapidyaml`.
idf_component_register(
    SRCS "rapidyaml.cpp"
    INCLUDE_DIRS "."
)
