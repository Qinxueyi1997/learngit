# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
set(COMPONENT_SRCS "dd.c"
                   "error.c"
                   "gauss.c"
                   "init.c"                                                   
                   "poly.c"
                   "rng.c"
                   "strerror.c"
                   "tridiag.c"              
                   "view.c"
                   "stream.c"                           
                   "variance.c")
                
set(COMPONENT_ADD_INCLUDEDIRS include)

register_component()
