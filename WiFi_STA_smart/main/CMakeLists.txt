# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c i2c_user.c lcd1602.c wifi.c")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
