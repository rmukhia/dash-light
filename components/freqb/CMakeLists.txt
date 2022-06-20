set(srcdirs "src")
set(includedirs "include")
set(requires freertos esp-dsp)


idf_component_register(SRC_DIRS ${srcdirs}
                       INCLUDE_DIRS ${includedirs}
                       REQUIRES ${requires})