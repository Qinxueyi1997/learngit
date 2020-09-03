#
# Component Makefile
#
#
COMPONENT_ADD_INCLUDEDIRS := ahu_sleep/include ahu_sleep/gsl-1.15/include
COMPONENT_ADD_LDFLAGS += $(COMPONENT_PATH)/ahu_sleep/libsleep_monitoring.a

# if build src, open the following
#COMPONENT_ADD_INCLUDEDIRS := include gsl-1.15/include
#COMPONENT_ADD_LDFLAGS := -lsleep_monitoring

#COMPONENT_SRCDIRS := . gsl-1.15
#COMPONENT_SUBMODULES := 

#COMPONENT_OBJS := ahu_utils.o ahu_statistics.o ahu_respirate.o ahu_dsp.o ahu_wavelet.o emd.o eemd.o gsl-1.15/dd.o gsl-1.15/error.o gsl-1.15/gauss.o gsl-1.15/init.o gsl-1.15/poly.o gsl-1.15/rng.o gsl-1.15/strerror.o gsl-1.15/tridiag.o gsl-1.15/view.o gsl-1.15/stream.o gsl-1.15/mean.o gsl-1.15/mt.o gsl-1.15/variance.o

