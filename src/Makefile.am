SUBDIRS=Implicit rsMath PixelCity

INCLUDES = -I$(top_srcdir)/src
AM_LDFLAGS = @GLEW@

bin_PROGRAMS = biof busyspheres colorfire cyclone drempels euphoria feedback fieldlines flocks flux helios hufo_smoke hufo_tunnel hyperspace lattice lorenz matrixview pixelcity plasma skyrocket solarwinds spirographx sundancer2

biof_SOURCES = driver.c biof.c
busyspheres_SOURCES = driver.c busyspheres.c
colorfire_SOURCES = driver.c colorfire.cpp colorfire_textures.h colorfire_textures.cpp
colorfire_LDADD = @BZIP2@
cyclone_SOURCES = driver.c cyclone.cpp rgbhsl.h rgbhsl.cpp rsRand.h
cyclone_LDADD = -LrsMath -lrsMath
drempels_SOURCES = driver.h driver.c drempels.cpp gpoly.h gpoly.cpp TexMgr.h TexMgr.cpp rgbhsl.h rgbhsl.cpp noise1234.h noise1234.c
drempels_CXXFLAGS = @Wand_CFLAGS@
drempels_LDADD = @PTHREAD_LIBS@ @Wand_LIBS@
euphoria_SOURCES = driver.h driver.c euphoria.cpp euphoria_textures.h euphoria_textures.c rgbhsl.h rgbhsl.cpp
euphoria_LDADD = -LrsMath -lrsMath @BZIP2@
feedback_SOURCES = driver.c feedback.cpp rgbhsl.h rgbhsl.cpp
feedback_LDADD = -LrsMath -lrsMath
fieldlines_SOURCES = driver.c fieldlines.cpp rsDefines.h rsRand.h
flocks_SOURCES = driver.c flocks.cpp rgbhsl.h rgbhsl.cpp rsDefines.h rsRand.h
flux_SOURCES = driver.c flux.cpp rgbhsl.h rgbhsl.cpp rsDefines.h rsRand.h
helios_SOURCES = driver.c helios.cpp spheremap.cpp rgbhsl.h rgbhsl.cpp rsDefines.h rsRand.h
helios_LDADD = @BZIP2@ -LImplicit -lImplicit -LrsMath -lrsMath
hufo_smoke_SOURCES = driver.c hufo_smoke.cpp FMotion.h FMotion.cpp
hufo_smoke_LDADD = -LrsMath -lrsMath
hufo_tunnel_SOURCES = driver.c hufo_tunnel.cpp hufo_tunnel.h hufo_tunnel_textures.h hufo_tunnel_textures.cpp
hufo_tunnel_LDADD = @BZIP2@ -LrsMath -lrsMath
hyperspace_SOURCES = driver.c hyperspace.cpp causticTextures.cpp flare.cpp goo.cpp nebulamap.cpp splinePath.cpp starBurst.cpp stretchedParticle.cpp tunnel.cpp wavyNormalCubeMaps.cpp rgbhsl.h rgbhsl.cpp
hyperspace_LDADD = @BZIP2@ -LImplicit -lImplicit -LrsMath -lrsMath
lattice_SOURCES = driver.h driver.c lattice.c lattice_textures.h lattice_textures.c rsMath.h rsMath.c
lattice_LDADD = @BZIP2@
lorenz_SOURCES = driver.c lorenz.cpp
lorenz_LDADD = -LrsMath -lrsMath
matrixview_SOURCES = driver.c matrixview.c matrixview_textures.c
matrixview_CFLAGS = @Wand_CFLAGS@
matrixview_LDADD = @BZIP2@ @PTHREAD_LIBS@ @Wand_LIBS@
plasma_SOURCES = driver.c plasma.cpp rsDefines.h rsRand.h
pixelcity_SOURCES = driver.c
pixelcity_LDADD = @quesoglc_LIBS@ -LPixelCity -lPixelCity
skyrocket_SOURCES = driver.c skyrocket.cpp skyrocket_flare.cpp skyrocket_particle.cpp skyrocket_shockwave.cpp skyrocket_smoke.cpp skyrocket_sound.cpp skyrocket_world.cpp skyrocket_textures.cpp rsDefines.h rsRand.h
solarwinds_SOURCES = driver.c solarwinds.cpp rsDefines.h rsRand.h
spirographx_SOURCES = driver.c spirographx.c
spirographx_LDADD = -LrsMath -lrsMath
sundancer2_SOURCES = driver.c sundancer2.c

if INSTALL_XSCREENSAVER_CONFIG_FILES
dist_config_DATA = $(bin_PROGRAMS:%=%.xml)
endif

if INSTALL_KDE_CONFIG_FILES
dist_kdessconfig_DATA = $(bin_PROGRAMS:%=%.desktop)
endif

man1_MANS = $(bin_PROGRAMS:%=%.1)

EXTRA_DIST = $(man1_MANS)

################################################################################
# Textures

noinst_PROGRAMS = stringify
stringify_LDADD = @BZIP2@

colorfire_textures.cpp: stringify colorfire_textures/ripplemap colorfire_textures/smokemap
	./stringify colorfire_textures/ripplemap colorfire_textures/smokemap > colorfire_textures.cpp

euphoria_textures.c: stringify euphoria_textures/linesmap euphoria_textures/plasmamap euphoria_textures/stringymap
	./stringify euphoria_textures/linesmap euphoria_textures/plasmamap euphoria_textures/stringymap > euphoria_textures.c

hufo_tunnel_textures.cpp: stringify hufo_tunnel_textures/marblemap hufo_tunnel_textures/swirlmap
	./stringify hufo_tunnel_textures/marblemap hufo_tunnel_textures/swirlmap > hufo_tunnel_textures.cpp

lattice_textures.c: stringify lattice_textures/brasstex lattice_textures/chrometex lattice_textures/circuittex lattice_textures/crystex lattice_textures/doughtex lattice_textures/ghostlytex lattice_textures/indtex1 lattice_textures/indtex2 lattice_textures/shinytex
	./stringify lattice_textures/brasstex lattice_textures/chrometex lattice_textures/circuittex lattice_textures/crystex lattice_textures/doughtex lattice_textures/ghostlytex lattice_textures/indtex1 lattice_textures/indtex2 lattice_textures/shinytex > lattice_textures.c

matrixview_textures.c: stringify matrixview_textures/cfont matrixview_textures/cpics
	./stringify matrixview_textures/cfont matrixview_textures/cpics > matrixview_textures.c

skyrocket_textures.cpp: stringify skyrocket_textures/cloudmap skyrocket_textures/earthnearmap skyrocket_textures/earthfarmap skyrocket_textures/earthlightmap skyrocket_textures/moontint skyrocket_textures/moonalpha skyrocket_textures/presmoke1 skyrocket_textures/presmoke2 skyrocket_textures/presmoke3 skyrocket_textures/presmoke4 skyrocket_textures/presmoke5
	./stringify skyrocket_textures/cloudmap skyrocket_textures/earthnearmap skyrocket_textures/earthfarmap skyrocket_textures/earthlightmap skyrocket_textures/moontint skyrocket_textures/moonalpha skyrocket_textures/presmoke1 skyrocket_textures/presmoke2 skyrocket_textures/presmoke3 skyrocket_textures/presmoke4 skyrocket_textures/presmoke5 > skyrocket_textures.cpp

spheremap.cpp: stringify helios_textures/spheremap
	./stringify helios_textures/spheremap > spheremap.cpp

nebulamap.cpp: stringify hyperspace_textures/nebulamap
	./stringify hyperspace_textures/nebulamap > nebulamap.cpp

################################################################################
# Sounds

if WITHSOUND
skyrocket_CXXFLAGS = @freealut_CFLAGS@
skyrocket_LDADD = @freealut_LIBS@ skyrocket_boomsound.o skyrocket_launchsound.o skyrocket_nukesound.o skyrocket_poppersound.o skyrocket_sucksound.o skyrocket_whistlesound.o @BZIP2@ -LrsMath -lrsMath

skyrocket_boomsound.cpp: stringify skyrocket_sounds/boom1SoundData skyrocket_sounds/boom2SoundData skyrocket_sounds/boom3SoundData skyrocket_sounds/boom4SoundData
	./stringify skyrocket_sounds/boom1SoundData skyrocket_sounds/boom2SoundData skyrocket_sounds/boom3SoundData skyrocket_sounds/boom4SoundData > skyrocket_boomsound.cpp

skyrocket_launchsound.cpp: stringify skyrocket_sounds/launch1SoundData skyrocket_sounds/launch2SoundData
	./stringify skyrocket_sounds/launch1SoundData skyrocket_sounds/launch2SoundData > skyrocket_launchsound.cpp

skyrocket_nukesound.cpp: stringify skyrocket_sounds/nukeSoundData
	./stringify skyrocket_sounds/nukeSoundData > skyrocket_nukesound.cpp

skyrocket_poppersound.cpp: stringify skyrocket_sounds/popperSoundData
	./stringify skyrocket_sounds/popperSoundData > skyrocket_poppersound.cpp

skyrocket_sucksound.cpp: stringify skyrocket_sounds/suckSoundData
	./stringify skyrocket_sounds/suckSoundData > skyrocket_sucksound.cpp

skyrocket_whistlesound.cpp: stringify skyrocket_sounds/whistleSoundData
	./stringify skyrocket_sounds/whistleSoundData > skyrocket_whistlesound.cpp
else
skyrocket_LDADD = @BZIP2@ -LrsMath -lrsMath
endif

CLEANFILES = colorfire_textures.cpp euphoria_textures.c hufo_tunnel_textures.cpp lattice_textures.c matrixview_textures.c skyrocket_textures.cpp skyrocket_boomsound.cpp skyrocket_launchsound.cpp skyrocket_nukesound.cpp skyrocket_poppersound.cpp skyrocket_sucksound.cpp skyrocket_whistlesound.cpp spheremap.cpp nebulamap.cpp
