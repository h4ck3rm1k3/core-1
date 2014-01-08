#	-DSAL_UNX  
inlc:
	g++ -save-temps \
	-DUNX  \
	-DLINUX \
	-D_LITTLE_ENDIAN \
	-latk-1.0 \
	-lgobject-2.0 \
	-lglib-2.0 \
	-I ./accessibility/inc \
	-I /usr/lib/x86_64-linux-gnu/glib-2.0/include/ \
	-I /usr/include/glib-2.0/ \
	-I /usr/include/atk-1.0/ \
	-I ./ \
	-I ./include \
	-I ./config_host \
	-I ./sw/source/core/access/ \
	-I ./workdir/UnoApiHeadersTarget/udkapi/comprehensive/ \
	-I ./workdir/UnoApiHeadersTarget/offapi/comprehensive/ \
	-I ./include/vcl/ \
	-I  ./sw/inc/ \
        -I ./sw/source/core/inc \
	fakeinclude/allinc.cxx

#        -I ./starmath/inc/ \
